#include "main.hpp"
#include "settings.hpp"

#include "HMUI/Touchable.hpp"

void DidActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {

    if(!firstActivation)
        return;

    self->get_gameObject()->AddComponent<HMUI::Touchable*>();
    auto vertical = QuestUI::BeatSaberUI::CreateVerticalLayoutGroup(self);
    vertical->set_childControlHeight(false);
    vertical->set_childForceExpandHeight(false);
    vertical->set_spacing(1);

    AddConfigValueToggle(vertical, getConfig().ShowOnPass);

    AddConfigValueToggle(vertical, getConfig().ShowOnFail);

    AddConfigValueToggle(vertical, getConfig().ShowGraph);

    AddConfigValueToggle(vertical, getConfig().NarrowGraphRange);

    AddConfigValueIncrementInt(vertical, getConfig().NumDecimals, 1, 0, 3);

    AddConfigValueToggle(vertical, getConfig().SaveLocally);

    AddConfigValueToggle(vertical, getConfig().UseCommas);
}