#pragma once

#include "rapidjson-macros/shared/macros.hpp"

#include "GlobalNamespace/IDifficultyBeatmap.hpp"

namespace BeatSaviorData {
    // notes refers to all notes, but l_notes and r_notes are only counted notes (see allowedNoteTypes in main.cpp)
    // score also refers to the total score, while fullNotesScore is the score of only the counted notes (both without modifiers as of v2)
    DECLARE_JSON_CLASS(Tracker,
        DISCARD_EXTRA_FIELDS
        VALUE(int, score)
        VALUE_DEFAULT(int, maxScore, -1)
        VALUE_DEFAULT(int, fullNotesScore, -1)
        VALUE(int, notes)
        VALUE(int, combo)
        VALUE(int, pauses)
        VALUE(int, misses)
        VALUE(int, l_notes)
        VALUE(int, r_notes)
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
        NAMED_VALUE_DEFAULT(int, trackerVersion, 2, "version")
        float min_pct, max_pct, song_time;
    )

    DECLARE_JSON_CLASS(Level,
        DESERIALIZE_ACTION(WhyDidIMakeThingsNotHaveNames,
            if(!jsonValue.IsArray())
                throw JSONException("level was not an array");
            for(auto& item : jsonValue.GetArray()) {
                self->maps.emplace_back(Tracker());
                try {
                    self->maps.back().Deserialize(item);
                } catch(const std::exception& e) {
                    LOG_ERROR("Error parsing map: %s", e.what());
                    self->maps.pop_back();
                }
            }
        )
        SERIALIZE_ACTION(WhyDidIMakeThingsNotHaveNames,
            jsonObject.SetArray();
            for(auto& map : self->maps)
                jsonObject.GetArray().PushBack(map.Serialize(allocator), allocator);
        )
        std::vector<Tracker> maps{};
    )

    DECLARE_JSON_CLASS(LocalData,
        MAP_DEFAULT(Level, levels, {})
    )
}

extern BeatSaviorData::Tracker currentTracker;

void LoadData();
void SaveMap(GlobalNamespace::IDifficultyBeatmap* beatmap, bool alwaysOverride = false);
void LoadMap(GlobalNamespace::IDifficultyBeatmap* beatmap);
void DeleteMap(GlobalNamespace::IDifficultyBeatmap* beatmap);
bool MapSaved(GlobalNamespace::IDifficultyBeatmap* beatmap);
