#include "sprites.hpp"
#include "assets.hpp"

#include "questui/shared/BeatSaberUI.hpp"

#define SPRITE_FUNC(name) \
UnityEngine::Sprite* name##Sprite() { \
    auto arr = ArrayW<uint8_t>(name##_png::getLength()); \
    memcpy(arr->values, name##_png::getData(), name##_png::getLength()); \
    return QuestUI::BeatSaberUI::ArrayToSprite(arr); \
}

SPRITE_FUNC(White)
SPRITE_FUNC(Circle)
SPRITE_FUNC(Delete)
