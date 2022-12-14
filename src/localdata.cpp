#include "main.hpp"
#include "localdata.hpp"

#include "GlobalNamespace/IPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/IDifficultyBeatmapSet.hpp"
#include "GlobalNamespace/BeatmapDifficulty.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"

#include <map>
#include <filesystem>

using namespace GlobalNamespace;
using namespace BeatSaviorData;

LocalData dataInstance;

Tracker currentTracker{};

std::tuple<std::string, std::string, int> GetInfo(IDifficultyBeatmap* beatmap) {
    std::string id = ((IPreviewBeatmapLevel*) beatmap->get_level())->get_levelID();
    std::string characteristic = beatmap->get_parentDifficultyBeatmapSet()->get_beatmapCharacteristic()->get_serializedName();
    int difficulty = beatmap->get_difficulty();
    return {id, characteristic, difficulty};
}

std::optional<std::reference_wrapper<Level>> GetLevel(std::string id) {
    auto iter = dataInstance.levels.find(id);
    if(iter == dataInstance.levels.end())
        return std::nullopt;
    return iter->second;
}

std::tuple<bool, std::vector<Tracker>::iterator> GetMap(std::optional<std::reference_wrapper<Level>>& opt, std::string characteristic, int difficulty) {
    auto& level = opt->get();
    for(auto iter = level.maps.begin(); iter != level.maps.end(); iter++) {
        if(iter->characteristic == characteristic && iter->difficulty == difficulty)
            return {true, iter};
    }
    return {false, level.maps.end()};
}

void LoadData() {
    static const std::string start = "{\"levels\":";
    auto json = readfile(GetDataPath());
    if(!json.starts_with(start))
        json = start + json + "}";

    try {
        ReadFromString(json, dataInstance);
    } catch(const std::exception& e) {
        LOG_ERROR("Error parsing local data: %s", e.what());
        std::filesystem::copy_file(GetDataPath(), GetDataPath() + ".bak", std::filesystem::copy_options::overwrite_existing);
    }
}

void SaveData() {
    WriteToFile(GetDataPath(), dataInstance);
}

void SaveMap(IDifficultyBeatmap* beatmap, bool alwaysOverride) {
    auto [id, characteristic, difficulty] = GetInfo(beatmap);

    currentTracker.characteristic = characteristic;
    currentTracker.difficulty = difficulty;

    auto level = GetLevel(id);
    if(level) {
        auto [found, map] = GetMap(level, characteristic, difficulty);
        // write if it's not saved or if it should be overridden
        if(!found)
            level->get().maps.emplace_back(currentTracker);
        else if(map->score < currentTracker.score || alwaysOverride)
            *map = currentTracker;
    } else {
        auto& newLevel = dataInstance.levels.emplace(id, Level()).first->second;
        newLevel.maps.emplace_back(currentTracker);
    }
    SaveData();
}

void LoadMap(IDifficultyBeatmap* beatmap) {
    auto [id, characteristic, difficulty] = GetInfo(beatmap);
    auto level = GetLevel(id);
    if(!level)
        return;
    auto [found, map] = GetMap(level, characteristic, difficulty);
    if(found)
        currentTracker = *map;
}

void DeleteMap(IDifficultyBeatmap* beatmap) {
    auto [id, characteristic, difficulty] = GetInfo(beatmap);
    auto level = GetLevel(id);
    if(!level)
        return;
    auto [found, map] = GetMap(level, characteristic, difficulty);
    if(!found)
        return;
    level->get().maps.erase(map);
    SaveData();
}

bool MapSaved(IDifficultyBeatmap* beatmap) {
    auto [id, characteristic, difficulty] = GetInfo(beatmap);
    auto level = GetLevel(id);
    if(!level)
        return false;
    auto [found, map] = GetMap(level, characteristic, difficulty);
    return found;
}
