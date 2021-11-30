#include "main.hpp"

#include "beatsaber-hook/shared/config/config-utils.hpp"

#include "GlobalNamespace/IPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/IDifficultyBeatmapSet.hpp"
#include "GlobalNamespace/BeatmapDifficulty.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"

using namespace GlobalNamespace;

ConfigDocument doc;
rapidjson::Value::MemberIterator levelDiffsItr;
int levelIdx;

// 0: failed to read, 1: failed to find level, 2: failed to find map, 3: success
int getLevel(IDifficultyBeatmap* beatmap) {
    std::string dir = getDataDir(modInfo);
    static std::string file = dir + "data.json";

    if(!direxists(dir) || !fileexists(file))
        return 0;

    ConfigDocument d;
    if(!parsejsonfile(d, file)) {
        getLogger().info("Failed to read file");
        return 0;
    }
    doc.Swap(d);

    std::string characteristic = to_utf8(csstrtostr(beatmap->get_parentDifficultyBeatmapSet()->get_beatmapCharacteristic()->get_serializedName()));
    int difficulty = beatmap->get_difficulty();
    std::string levelID = to_utf8(csstrtostr(reinterpret_cast<IPreviewBeatmapLevel*>(beatmap->get_level())->get_levelID()));
    
    levelDiffsItr = doc.FindMember(levelID);
    if(levelDiffsItr == doc.MemberEnd())
        return 1;
    auto levelDiffs = levelDiffsItr->value.GetArray();
    levelIdx = -1;
    for(int i = 0; i < levelDiffs.Size(); i++) {
        if(levelDiffs[i].GetObject()["characteristic"].GetString() == characteristic && levelDiffs[i].GetObject()["difficulty"].GetInt() == difficulty) {
            levelIdx = i;
        }
    }
    if(levelIdx < 0)
        return 2;

    getLogger().info("Found level in file");
    return 3;
}

bool isInFile(IDifficultyBeatmap* beatmap) {
    if(getLevel(beatmap) != 3)
        return false;
    // use the presence of date as a marker for empty level data
    auto level = levelDiffsItr->value.GetArray()[levelIdx].GetObject();
    auto dateItr = level.FindMember("date");
    return dateItr != level.MemberEnd();
}

void writeFile() {
    static std::string file = getDataDir(modInfo) + "data.json";

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    std::string s = buffer.GetString();

    writefile(file, s);
}

#define addTrackerMember(name) v.AddMember(#name, tracker.name, allocator);
void addDataToFile(IDifficultyBeatmap* beatmap) {
    getLogger().info("Saving local data");
    static std::string dir = getDataDir(modInfo);
    static std::string file = dir + "data.json";

    if(!direxists(dir)) {
        int made = mkpath(dir);
        if(made < 0) {
            getLogger().info("Failed to make data directory");
            return;
        }
    }
    if(!fileexists(file))
        writefile(file, "{}");

    int completion = getLevel(beatmap);
    if(completion == 0)
        return;

    auto& allocator = doc.GetAllocator();

    std::string characteristic = to_utf8(csstrtostr(beatmap->get_parentDifficultyBeatmapSet()->get_beatmapCharacteristic()->get_serializedName()));
    int difficulty = beatmap->get_difficulty();
    std::string levelID = to_utf8(csstrtostr(reinterpret_cast<IPreviewBeatmapLevel*>(beatmap->get_level())->get_levelID()));

    #pragma region trackerObject
    rapidjson::Value v(rapidjson::kObjectType);
    v.AddMember("characteristic", characteristic, allocator);
    v.AddMember("difficulty", difficulty, allocator);
    addTrackerMember(score);
    addTrackerMember(notes);
    addTrackerMember(combo);
    addTrackerMember(pauses);
    addTrackerMember(misses);
    addTrackerMember(date);
    addTrackerMember(l_notes);
    addTrackerMember(r_notes);
    addTrackerMember(l_cut);
    addTrackerMember(l_beforeCut);
    addTrackerMember(l_accuracy);
    addTrackerMember(l_afterCut);
    addTrackerMember(l_speed);
    addTrackerMember(l_distance);
    addTrackerMember(l_preSwing);
    addTrackerMember(l_postSwing);
    addTrackerMember(r_cut);
    addTrackerMember(r_beforeCut);
    addTrackerMember(r_accuracy);
    addTrackerMember(r_afterCut);
    addTrackerMember(r_speed);
    addTrackerMember(r_distance);
    addTrackerMember(r_preSwing);
    addTrackerMember(r_postSwing);
    #pragma endregion

    auto levelDiffs = levelDiffsItr->value.GetArray();
    switch (completion) {
        case 1:
            getLogger().info("Adding level to file");
            doc.AddMember(rapidjson::Value(levelID, allocator).Move(), rapidjson::Value(rapidjson::kArrayType), allocator);
            doc.FindMember(levelID)->value.GetArray().PushBack(v, allocator);
            break;
        case 2:
            getLogger().info("Adding beatmap to file");
            levelDiffsItr->value.GetArray().PushBack(v, allocator);
            break;
        case 3:
            if(levelDiffs[levelIdx].GetObject()["score"].GetInt() < tracker.score) {
                levelDiffs.Erase(levelDiffs.Begin() + levelIdx);
                levelDiffs.PushBack(v, allocator);
            }
            break;
        default:
            return;
    }

    writeFile();
}

#define addMemberTracker(name) tracker.name = level[#name].GetFloat();
void getDataFromFile(IDifficultyBeatmap* beatmap) {
    if(getLevel(beatmap) != 3)
        return;

    auto level = levelDiffsItr->value.GetArray()[levelIdx].GetObject();
    #pragma region trackerObject
    addMemberTracker(score);
    addMemberTracker(notes);
    addMemberTracker(combo);
    addMemberTracker(pauses);
    addMemberTracker(misses);
    tracker.date = level["date"].GetString();
    addMemberTracker(l_notes);
    addMemberTracker(r_notes);
    addMemberTracker(l_cut);
    addMemberTracker(l_beforeCut);
    addMemberTracker(l_accuracy);
    addMemberTracker(l_afterCut);
    addMemberTracker(l_speed);
    addMemberTracker(l_distance);
    addMemberTracker(l_preSwing);
    addMemberTracker(l_postSwing);
    addMemberTracker(r_cut);
    addMemberTracker(r_beforeCut);
    addMemberTracker(r_accuracy);
    addMemberTracker(r_afterCut);
    addMemberTracker(r_speed);
    addMemberTracker(r_distance);
    addMemberTracker(r_preSwing);
    addMemberTracker(r_postSwing);
    #pragma endregion
}

void deleteDataFromFile(IDifficultyBeatmap* beatmap) {
    if(getLevel(beatmap) != 3)
        return;
    
    auto levelDiffs = levelDiffsItr->value.GetArray();
    levelDiffs.Erase(levelDiffs.Begin() + levelIdx);

    writeFile();
}