#include "main.hpp"
#include "config.hpp"

#include "HMUI/Touchable.hpp"
#include "questui/shared/BeatSaberUI.hpp"

using namespace QuestUI;

void DidActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    if(!firstActivation)
        return;
    
    self->get_gameObject()->AddComponent<HMUI::Touchable*>();
    UnityEngine::GameObject* container = BeatSaberUI::CreateScrollableSettingsContainer(self->get_transform());
    
    BeatSaberUI::AddHoverHint(BeatSaberUI::CreateToggle(container, "Show On Passes", globalConfig.ShowOnPass, [](bool enabled) {
        globalConfig.ShowOnPass = enabled;
        WriteToFile(GetConfigPath(), globalConfig);
    }), "Show the stats after passing a level");
    
    BeatSaberUI::AddHoverHint(BeatSaberUI::CreateToggle(container, "Show On Fails", globalConfig.ShowOnFail, [](bool enabled) {
        globalConfig.ShowOnFail = enabled;
        WriteToFile(GetConfigPath(), globalConfig);
    }), "Show the stats after failing a level");
    
    BeatSaberUI::AddHoverHint(BeatSaberUI::CreateToggle(container, "Show Graph", globalConfig.ShowGraph, [](bool enabled) {
        globalConfig.ShowGraph = enabled;
        WriteToFile(GetConfigPath(), globalConfig);
    }), "Show a performance graph");
    
    BeatSaberUI::AddHoverHint(BeatSaberUI::CreateToggle(container, "Narrow Graph Range", globalConfig.NarrowGraphRange, [](bool enabled) {
        globalConfig.NarrowGraphRange = enabled;
        WriteToFile(GetConfigPath(), globalConfig);
    }), "Automatically set the bounds of the performance graph to fit its contents");

    BeatSaberUI::AddHoverHint(BeatSaberUI::CreateIncrementSetting(container, "Decimal Precision", 0, 1, globalConfig.NumDecimals, 0, 3, [](float newValue) {
        globalConfig.NumDecimals = newValue;
        WriteToFile(GetConfigPath(), globalConfig);
    }), "The decimal precision to display");
    
    BeatSaberUI::AddHoverHint(BeatSaberUI::CreateToggle(container, "Save Data Locally", globalConfig.SaveLocally, [](bool enabled) {
        globalConfig.SaveLocally = enabled;
        WriteToFile(GetConfigPath(), globalConfig);
    }), "Store all results information in a local file");
    
    BeatSaberUI::AddHoverHint(BeatSaberUI::CreateToggle(container, "Use Commas", globalConfig.UseCommas, [](bool enabled) {
        globalConfig.UseCommas = enabled;
        WriteToFile(GetConfigPath(), globalConfig);
    }), "Commas instead of decimal points. Why do people do this?");
}