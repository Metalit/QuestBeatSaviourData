#include "main.hpp"
#include "localdata.hpp"

#include "GlobalNamespace/IPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/IDifficultyBeatmapSet.hpp"
#include "GlobalNamespace/BeatmapDifficulty.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"

#define DESERIALIZE_VALUE_NAME(name, jsonType) DESERIALIZE_VALUE(name, name, jsonType)
DESERIALIZE_METHOD(BeatSaviorData, Tracker,
    DESERIALIZE_VALUE_NAME(score, Int);
    DESERIALIZE_VALUE_NAME(notes, Int);
    DESERIALIZE_VALUE_NAME(combo, Int);
    DESERIALIZE_VALUE_NAME(pauses, Int);
    DESERIALIZE_VALUE_NAME(misses, Int);
    DESERIALIZE_VALUE_NAME(l_notes, Int);
    DESERIALIZE_VALUE_NAME(r_notes, Int);
    DESERIALIZE_VALUE_NAME(l_cut, Float);
    DESERIALIZE_VALUE_NAME(l_beforeCut, Float);
    DESERIALIZE_VALUE_NAME(l_accuracy, Float);
    DESERIALIZE_VALUE_NAME(l_afterCut, Float);
    DESERIALIZE_VALUE_NAME(l_speed, Float);
    DESERIALIZE_VALUE_NAME(l_distance, Float);
    DESERIALIZE_VALUE_NAME(l_preSwing, Float);
    DESERIALIZE_VALUE_NAME(l_postSwing, Float);
    DESERIALIZE_VALUE_NAME(r_cut, Float);
    DESERIALIZE_VALUE_NAME(r_beforeCut, Float);
    DESERIALIZE_VALUE_NAME(r_accuracy, Float);
    DESERIALIZE_VALUE_NAME(r_afterCut, Float);
    DESERIALIZE_VALUE_NAME(r_speed, Float);
    DESERIALIZE_VALUE_NAME(r_distance, Float);
    DESERIALIZE_VALUE_NAME(r_preSwing, Float);
    DESERIALIZE_VALUE_NAME(r_postSwing, Float);
    DESERIALIZE_VALUE_NAME(date, String);
    DESERIALIZE_VALUE_NAME(characteristic, String);
    DESERIALIZE_VALUE_NAME(difficulty, Int);
)

#define SERIALIZE_VALUE_NAME(name) SERIALIZE_VALUE(name, name)
SERIALIZE_METHOD(BeatSaviorData, Tracker,
    SERIALIZE_VALUE_NAME(score);
    SERIALIZE_VALUE_NAME(notes);
    SERIALIZE_VALUE_NAME(combo);
    SERIALIZE_VALUE_NAME(pauses);
    SERIALIZE_VALUE_NAME(misses);
    SERIALIZE_VALUE_NAME(l_notes);
    SERIALIZE_VALUE_NAME(r_notes);
    SERIALIZE_VALUE_NAME(l_cut);
    SERIALIZE_VALUE_NAME(l_beforeCut);
    SERIALIZE_VALUE_NAME(l_accuracy);
    SERIALIZE_VALUE_NAME(l_afterCut);
    SERIALIZE_VALUE_NAME(l_speed);
    SERIALIZE_VALUE_NAME(l_distance);
    SERIALIZE_VALUE_NAME(l_preSwing);
    SERIALIZE_VALUE_NAME(l_postSwing);
    SERIALIZE_VALUE_NAME(r_cut);
    SERIALIZE_VALUE_NAME(r_beforeCut);
    SERIALIZE_VALUE_NAME(r_accuracy);
    SERIALIZE_VALUE_NAME(r_afterCut);
    SERIALIZE_VALUE_NAME(r_speed);
    SERIALIZE_VALUE_NAME(r_distance);
    SERIALIZE_VALUE_NAME(r_preSwing);
    SERIALIZE_VALUE_NAME(r_postSwing);
    SERIALIZE_VALUE_NAME(date);
    SERIALIZE_VALUE_NAME(characteristic);
    SERIALIZE_VALUE_NAME(difficulty);
)

DESERIALIZE_METHOD(BeatSaviorData, Level,
    if(!jsonValue.IsArray())
        return;
    for(auto& item : jsonValue.GetArray()) {
        maps.emplace_back(Tracker());
        maps.back().Deserialize(item);
    }
)

rapidjson::Value BeatSaviorData::Level::Serialize(rapidjson::Document::AllocatorType& allocator) {
    rapidjson::Value jsonObject(rapidjson::kArrayType);
    for(auto& map : maps) {
        jsonObject.GetArray().PushBack(map.Serialize(allocator), allocator);
    }
    return jsonObject;
}

using namespace GlobalNamespace;
using namespace BeatSaviorData;

rapidjson::Document globalDoc;
std::unordered_map<std::string, BeatSaviorData::Level> id_levels;

Tracker currentTracker{};

void loadData() {
    auto json = readfile(GetDataPath());

    globalDoc.Parse(json);
    if(globalDoc.HasParseError()) {
        getLogger().error("Could not parse json!");
        return;
    }
    if(!globalDoc.IsObject())
        globalDoc.SetObject();

    id_levels.clear();

    // each level is stored globally, which I regret
    for(auto& member : globalDoc.GetObject()) {
        // add blank level at location
        std::string id = member.name.GetString();
        id_levels.insert({id, Level()});
        // get level and deserialize it
        auto& level = id_levels.find(id)->second;
        level.Deserialize(member.value);
    }
}

void saveData() {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    globalDoc.Accept(writer);
    std::string_view s = buffer.GetString();

    writefile(GetDataPath(), s);
}

void saveLevel(std::string const& id) {
    auto level = globalDoc.FindMember(id);
    if(level == globalDoc.MemberEnd())
        return;
    
    auto& levelObj = id_levels.find(id)->second;
    level->value = levelObj.Serialize(globalDoc.GetAllocator());

    saveData();
}

void saveMap(IDifficultyBeatmap* beatmap, bool alwaysOverride) {
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
            saveLevel(id);
        } else if(mapTracker->score < currentTracker.score || alwaysOverride) {
            *mapTracker = currentTracker;
            saveLevel(id);
        }
    } else {
        auto& allocator = globalDoc.GetAllocator();
        // create new level
        id_levels.insert({id, Level()});
        auto& level = id_levels.find(id)->second;
        // add map to level
        level.maps.emplace_back(currentTracker);
        // add level to document and save
        globalDoc.AddMember(rapidjson::Value(id, allocator).Move(), level.Serialize(allocator), allocator);
        saveData();
    }
}

void loadMap(GlobalNamespace::IDifficultyBeatmap* beatmap) {
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

void deleteMap(IDifficultyBeatmap* beatmap) {
    // get identification info
    std::string id = ((IPreviewBeatmapLevel*) beatmap->get_level())->get_levelID();
    std::string characteristic = beatmap->get_parentDifficultyBeatmapSet()->get_beatmapCharacteristic()->get_serializedName();
    int difficulty = beatmap->get_difficulty();
    
    if(!id_levels.contains(id))
        return;
    auto& level = id_levels.find(id)->second;
    for(auto iter = level.maps.begin(); iter != level.maps.end(); iter++) {
        if(iter->characteristic == characteristic && iter->difficulty == difficulty) {
            level.maps.erase(iter);
            break;
        }
    }
    
    saveLevel(id);
}

bool mapSaved(IDifficultyBeatmap* beatmap) {
    // get identification info
    std::string id = ((IPreviewBeatmapLevel*) beatmap->get_level())->get_levelID();
    std::string characteristic = beatmap->get_parentDifficultyBeatmapSet()->get_beatmapCharacteristic()->get_serializedName();
    int difficulty = beatmap->get_difficulty();
    
    if(!id_levels.contains(id))
        return false;
    auto& level = id_levels.find(id)->second;
    for(auto& map : level.maps) {
        if(map.characteristic == characteristic && map.difficulty == difficulty)
            return true;
    }
    return false;
}
