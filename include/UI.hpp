#pragma once

#include "main.hpp"

#include "HMUI/ViewController.hpp"
#include "HMUI/ImageView.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "UnityEngine/UI/VerticalLayoutGroup.hpp"
#include "UnityEngine/Color.hpp"

DECLARE_CLASS_CODEGEN(BSDUI, LevelStats, HMUI::ViewController,
    DECLARE_OVERRIDE_METHOD(void, DidActivate, il2cpp_utils::FindMethodUnsafe("HMUI", "ViewController", "DidActivate", 3), bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling);

    public:

    void setColors(UnityEngine::Color leftColor, UnityEngine::Color rightColor);

    HMUI::ImageView *songCover, *l_circle, *r_circle, *top_line, *bottom_line;
    TMPro::TextMeshProUGUI *songName, *songAuthor, *songMapper, *songDifficulty,
    *rank, *percent, *combo, *misses, *pauses,
    *l_cut, *l_beforeCut, *l_accuracy, *l_afterCut, *l_distance, *l_speed, *l_preSwing, *l_postSwing,
    *r_cut, *r_beforeCut, *r_accuracy, *r_afterCut, *r_distance, *r_speed, *r_preSwing, *r_postSwing;
)

DECLARE_CLASS_CODEGEN(BSDUI, ScoreGraph, HMUI::ViewController,
    DECLARE_OVERRIDE_METHOD(void, DidActivate, il2cpp_utils::FindMethodUnsafe("HMUI", "ViewController", "DidActivate", 3), bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling);

    public:
    
    UnityEngine::GameObject* graphContainer;
)