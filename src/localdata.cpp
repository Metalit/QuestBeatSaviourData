#include "main.hpp"
#include "localdata.hpp"

#include "GlobalNamespace/IPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/IDifficultyBeatmapSet.hpp"
#include "GlobalNamespace/BeatmapDifficulty.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"

#include <map>

using namespace GlobalNamespace;
using namespace BeatSaviorData;

rapidjson::Document globalDoc;
std::map<std::string, BeatSaviorData::Level> id_levels;

Tracker currentTracker{};

void LoadData() {
    auto json = readfile(GetDataPath());

    globalDoc.Parse(json);
    if(globalDoc.HasParseError()) {
        LOG_ERROR("Could not parse json!");
        return;
    }
    if(!globalDoc.IsObject())
        globalDoc.SetObject();

    id_levels.clear();

    // each level is stored globally, which I regret
    for(auto& member : globalDoc.GetObject()) {
        // add blank level at location
        std::string id = member.name.GetString();
        auto& level = id_levels.emplace(id, Level()).first->second;
        try {
            level.Deserialize(member.value);
        } catch (const std::exception& err) {
            LOG_ERROR("Error reading id %s from data: %s!", id.c_str(), err.what());
        }
    }
}

void SaveData() {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    globalDoc.Accept(writer);
    std::string_view s = buffer.GetString();

    writefile(GetDataPath(), s);
}

void SaveLevel(std::string const& id) {
    auto level = globalDoc.FindMember(id);
    if(level == globalDoc.MemberEnd())
        return;

    auto& levelObj = id_levels.find(id)->second;
    level->value = levelObj.Serialize(globalDoc.GetAllocator());

    SaveData();
}

void SaveMap(IDifficultyBeatmap* beatmap, bool alwaysOverride) {
    // get identification info
    std::string id = ((IPreviewBeatmapLevel*) beatmap->get_level())->get_levelID();
    std::string characteristic = beatmap->get_parentDifficultyBeatmapSet()->get_beatmapCharacteristic()->get_serializedName();
    int difficulty = beatmap->get_difficulty();

    currentTracker.characteristic = characteristic;
    currentTracker.difficulty = difficulty;

    // check if level is already saved
    if(id_levels.contains(id)) {
        auto& level = id_levels.find(id)->second;
        // check if map is already saved
        Tracker* mapTracker = nullptr;
        for(auto& map : level.maps) {
            if(map.characteristic == characteristic && map.difficulty == difficulty)
                mapTracker = &map;
        }
        // write if it's not saved or if it should be overridden
        if(!mapTracker) {
            level.maps.emplace_back(currentTracker);
            SaveLevel(id);
        } else if(mapTracker->score < currentTracker.score || alwaysOverride) {
            *mapTracker = currentTracker;
            SaveLevel(id);
        }
    } else {
        auto& allocator = globalDoc.GetAllocator();
        // create new level
        auto& level = id_levels.emplace(id, Level()).first->second;
        // add map to level
        level.maps.emplace_back(currentTracker);
        // add level to document and save
        globalDoc.AddMember(rapidjson::Value(id, allocator).Move(), level.Serialize(allocator), allocator);
        SaveData();
    }
}

void LoadMap(IDifficultyBeatmap* beatmap) {
    // get identification info
    std::string id = ((IPreviewBeatmapLevel*) beatmap->get_level())->get_levelID();
    std::string characteristic = beatmap->get_parentDifficultyBeatmapSet()->get_beatmapCharacteristic()->get_serializedName();
    int difficulty = beatmap->get_difficulty();

    if(id_levels.contains(id)) {
        auto& level = id_levels.find(id)->second;
        // find map in saved maps
        Tracker* mapTracker = nullptr;
        for(auto& map : level.maps) {
            if(map.characteristic == characteristic && map.difficulty == difficulty)
                mapTracker = &map;
        }
        // write if it's not saved or if it should be overridden
        if(mapTracker)
            currentTracker = *mapTracker;
    }
}

void DeleteMap(IDifficultyBeatmap* beatmap) {
    // get identification info
    std::string id = ((IPreviewBeatmapLevel*) beatmap->get_level())->get_levelID();
    std::string characteristic = beatmap->get_parentDifficultyBeatmapSet()->get_beatmapCharacteristic()->get_serializedName();
    int difficulty = beatmap->get_difficulty();

    if(!id_levels.contains(id))
        return;
    auto& maps = id_levels.find(id)->second.maps;
    for(auto iter = maps.begin(); iter != maps.end(); iter++) {
        if(iter->characteristic == characteristic && iter->difficulty == difficulty) {
            maps.erase(iter);
            break;
        }
    }
    SaveLevel(id);
}

bool MapSaved(IDifficultyBeatmap* beatmap) {
    // get identification info
    std::string id = ((IPreviewBeatmapLevel*) beatmap->get_level())->get_levelID();
    std::string characteristic = beatmap->get_parentDifficultyBeatmapSet()->get_beatmapCharacteristic()->get_serializedName();
    int difficulty = beatmap->get_difficulty();

    if(!id_levels.contains(id))
        return false;

    auto& maps = id_levels.find(id)->second.maps;
    for(auto& map : maps) {
        if(map.characteristic == characteristic && map.difficulty == difficulty)
            return true;
    }
    return false;
}
