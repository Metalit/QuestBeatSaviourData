#pragma once

#include "TypeMacros.hpp"

#include "GlobalNamespace/IDifficultyBeatmap.hpp"

DECLARE_JSON_CLASS(BeatSaviorData, Tracker,
    int score, notes, combo, pauses, misses, l_notes, r_notes;
    float min_pct, max_pct, song_time,
    l_cut, l_beforeCut, l_accuracy, l_afterCut, l_speed, l_distance, l_preSwing, l_postSwing,
    r_cut, r_beforeCut, r_accuracy, r_afterCut, r_speed, r_distance, r_preSwing, r_postSwing;
    std::string date;
    std::string characteristic;
    int difficulty;
)

DECLARE_JSON_CLASS(BeatSaviorData, Level,
    std::vector<Tracker> maps{};
)

extern BeatSaviorData::Tracker currentTracker;

void loadData();
void saveMap(GlobalNamespace::IDifficultyBeatmap* beatmap, bool alwaysOverride = false);
void loadMap(GlobalNamespace::IDifficultyBeatmap* beatmap);
void deleteMap(GlobalNamespace::IDifficultyBeatmap* beatmap);
bool mapSaved(GlobalNamespace::IDifficultyBeatmap* beatmap);
