#pragma once

#include "modloader/shared/modloader.hpp"

#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"

#include "extern/custom-types/shared/macros.hpp"
#include "extern/custom-types/shared/register.hpp"
#include "extern/custom-types/shared/types.hpp"

Logger& getLogger();

#include "HMUI/ViewController.hpp"

void DidActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling);

struct Tracker {
    int pauses, l_notes, r_notes, notes, score;
    float min_pct, max_pct, song_time,
    l_cut, l_beforeCut, l_accuracy, l_afterCut, l_speed, l_preSwing, l_postSwing,
    r_cut, r_beforeCut, r_accuracy, r_afterCut, r_speed, r_preSwing, r_postSwing;
};
extern Tracker tracker;

#include <vector>

extern std::vector<std::pair<float, float>> percents;