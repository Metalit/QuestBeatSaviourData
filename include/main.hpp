#pragma once

#include "modloader/shared/modloader.hpp"

#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"

Logger& getLogger();

std::string GetConfigPath();
std::string GetDataPath();

#include "HMUI/ViewController.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"

void DidActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling);

extern ModInfo modInfo;
extern GlobalNamespace::IDifficultyBeatmap* lastBeatmap;
extern std::vector<std::pair<float, float>> percents;

void disableDetailsButton();
