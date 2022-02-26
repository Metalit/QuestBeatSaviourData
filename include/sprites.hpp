#pragma once

#include "UnityEngine/Sprite.hpp"

UnityEngine::Sprite* WhiteSprite(bool reset = false);
UnityEngine::Sprite* CircleSprite(bool reset = false);
UnityEngine::Sprite* DeleteSprite(bool reset = false);

static void ClearSprites() {
    WhiteSprite(true);
    CircleSprite(true);
    DeleteSprite(true);
}