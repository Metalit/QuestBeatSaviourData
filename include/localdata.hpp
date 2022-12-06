#pragma once

#include "TypeMacros.hpp"

#include "rapidjson-macros/shared/macros.hpp"

#include "GlobalNamespace/IDifficultyBeatmap.hpp"

namespace BeatSaviorData {
    // notes refers to all notes, but l_notes and r_notes are only counted notes (see allowedNoteTypes in main.cpp)
    // score also refers to the total score, while fullNotesScore is the score of only the counted notes (both without modifiers as of v2)
    DECLARE_JSON_CLASS(Tracker,
        VALUE(int, score)
        VALUE(int, maxScore)
        VALUE(int, fullNotesScore)
        VALUE(int, notes)
        VALUE(int, combo)
        VALUE(int, pauses)
        VALUE(int, misses)
        VALUE(int, l_notes)
        VALUE(int, r_notes)
        VALUE(float, min_pct)
        VALUE(float, max_pct)
        VALUE(float, song_time)
        VALUE(float, l_cut)
        VALUE(float, l_beforeCut)
        VALUE(float, l_accuracy)
        VALUE(float, l_afterCut)
        VALUE(float, l_speed)
        VALUE(float, l_distance)
        VALUE(float, l_preSwing)
        VALUE(float, l_postSwing)
        VALUE(float, r_cut)
        VALUE(float, r_beforeCut)
        VALUE(float, r_accuracy)
        VALUE(float, r_afterCut)
        VALUE(float, r_speed)
        VALUE(float, r_distance)
        VALUE(float, r_preSwing)
        VALUE(float, r_postSwing)
        VALUE(std::string, date)
        VALUE(std::string, characteristic)
        VALUE_OPTIONAL(std::string, modifiers)
        VALUE(int, difficulty)
        VALUE_DEFAULT(int, trackerVersion, 2)
    )

    DECLARE_JSON_CLASS(Level,
        VECTOR_DEFAULT(Tracker, maps, {});
    )
}

extern BeatSaviorData::Tracker currentTracker;

void LoadData();
void SaveMap(GlobalNamespace::IDifficultyBeatmap* beatmap, bool alwaysOverride = false);
void LoadMap(GlobalNamespace::IDifficultyBeatmap* beatmap);
void DeleteMap(GlobalNamespace::IDifficultyBeatmap* beatmap);
bool MapSaved(GlobalNamespace::IDifficultyBeatmap* beatmap);
