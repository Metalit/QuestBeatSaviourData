#pragma once

#include "TypeMacros.hpp"

#include "GlobalNamespace/IDifficultyBeatmap.hpp"

// notes refers to all notes, but l_notes and r_notes are only counted notes (see allowedNoteTypes in main.cpp)
// score also refers to the total score, while fullNotesScore is the score of only the counted notes (both without modifiers as of v2)
DECLARE_JSON_CLASS(BeatSaviorData, Tracker,
    int score, maxScore, fullNotesScore, notes, combo, pauses, misses, l_notes, r_notes;
    float min_pct, max_pct, song_time,
    l_cut, l_beforeCut, l_accuracy, l_afterCut, l_speed, l_distance, l_preSwing, l_postSwing,
    r_cut, r_beforeCut, r_accuracy, r_afterCut, r_speed, r_distance, r_preSwing, r_postSwing;
    std::string date;
    std::string characteristic;
    std::optional<std::string> modifiers;
    int difficulty;
    int trackerVersion = 2;
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
