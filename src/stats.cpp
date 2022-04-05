#include "main.hpp"
#include "config.hpp"
#include "stats.hpp"
#include "localdata.hpp"
#include "sprites.hpp"

#include "questui/shared/BeatSaberUI.hpp"

#include "HMUI/ViewController_AnimationType.hpp"

#include "GlobalNamespace/IPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/BeatmapDifficulty.hpp"
#include "GlobalNamespace/PlayerDataModel.hpp"
#include "GlobalNamespace/PlayerData.hpp"
#include "GlobalNamespace/ColorSchemesSettings.hpp"
#include "GlobalNamespace/ColorScheme.hpp"
#include "GlobalNamespace/LeaderboardViewController.hpp"

#include "System/Threading/CancellationToken.hpp"
#include "System/Threading/Tasks/Task_1.hpp"

#include "UnityEngine/UI/Image_Origin360.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Color.hpp"
#include "UnityEngine/Material.hpp"

#include <iomanip>
#include <sstream>

DEFINE_TYPE(BeatSaviorData, LevelStats);
DEFINE_TYPE(BeatSaviorData, ScoreGraph);

using namespace BeatSaviorData;
using namespace QuestUI;
using namespace GlobalNamespace;

#pragma region helpers
#define ANCHOR(component, xmin, ymin, xmax, ymax) auto component##_rect = reinterpret_cast<UnityEngine::RectTransform*>(component->get_transform()); \
component##_rect->set_anchorMin({xmin, ymin}); \
component##_rect->set_anchorMax({xmax, ymax});

int calculateOldMaxScore(int blockCount) {
    int maxScore;
    if(blockCount < 14) {
        if (blockCount == 1) {
            maxScore = 115;
        } else if (blockCount < 5) {
            maxScore = (blockCount - 1) * 230 + 115;
        } else {
            maxScore = (blockCount - 5) * 460 + 1035;
        }
    } else {
        maxScore = (blockCount - 13) * 920 + 4715;
    }
    return maxScore;
}

inline float safeDiv(float numerator, float denominator) {
    if(denominator > 0)
        return numerator / denominator;
    return 0;
}

std::string Round(float num, std::string_view extra = "") {
    std::stringstream out;
    out << std::fixed << std::setprecision(globalConfig.NumDecimals) << num;
    out << extra;

    std::string s = out.str();
    if(globalConfig.UseCommas) {
        getLogger().debug("yo wtf is this crashing");
        int i = s.find('.');
        getLogger().debug("i: %i", i);
        if(i > 0)
            s[i] = ',';
        getLogger().debug("it even made it past here");
    }
    return s;
}

HMUI::ImageView* anchorImage(UnityEngine::Transform* parent, UnityEngine::Sprite* sprite, float xmin, float ymin, float xmax, float ymax) {
    HMUI::ImageView* ret = BeatSaberUI::CreateImage(parent, sprite, {0, 0}, {0, 0});
    ANCHOR(ret, xmin, ymin, xmax, ymax)
    return ret;
}

TMPro::TextMeshProUGUI* anchorText(UnityEngine::Transform* parent, std::string text, float xmin, float ymin, float xmax, float ymax) {
    TMPro::TextMeshProUGUI* ret = BeatSaberUI::CreateText(parent, to_utf16(text));
    ANCHOR(ret, xmin, ymin, xmax, ymax)
    ret->set_alignment(TMPro::TextAlignmentOptions::Center);
    return ret;
}

UnityEngine::GameObject* anchorContainer(UnityEngine::Transform* parent, float xmin, float ymin, float xmax, float ymax) {
    static ConstString name("BeatSaviorDataContainer");
    auto go = UnityEngine::GameObject::New_ctor(name);
    go->AddComponent<UnityEngine::UI::ContentSizeFitter*>();
    
    auto rect = go->GetComponent<UnityEngine::RectTransform*>();
    rect->SetParent(parent, false);
    rect->set_anchorMin({xmin, ymin});
    rect->set_anchorMax({xmax, ymax});
    rect->set_sizeDelta({0, 0});

    go->AddComponent<UnityEngine::UI::LayoutElement*>();
    return go;
}

#define graphScale 66
#define graphLength 1.7
UnityEngine::UI::Image* createLine(UnityEngine::Transform* parent, UnityEngine::Vector2 a, UnityEngine::Vector2 b) {
    a.x *= graphLength;
    b.x *= graphLength;

    float dist = UnityEngine::Vector2::Distance(a, b);
    UnityEngine::Vector2 dir = (b - a).get_normalized();
    
    UnityEngine::UI::Image* image = BeatSaberUI::CreateImage(parent, WhiteSprite(), (a + dir * dist * 0.5)*graphScale, {dist*graphScale, 0.3});
    ANCHOR(image, 0, 0, 0, 0);

    // UnityEngine::RectTransform* rect = reinterpret_cast<UnityEngine::RectTransform*>(image->get_transform());
    image->get_transform()->set_localEulerAngles({0, 0, (float)(atan2(dir.y, dir.x) * 57.29578)}); // radians to degrees

    return image;
}
#pragma endregion

void LevelStats::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    if(!firstActivation) return;

    getLogger().info("Constructing level stats menu");

    #pragma region songInfo
    auto song = anchorContainer(get_transform(), 0, 0.75, 1, 1);
    
    songCover = anchorImage(song->get_transform(), WhiteSprite(), 0.01, 0.05, 0.175, 0.95);
    auto songInfo = anchorContainer(song->get_transform(), 0.44, 0.05, 1, 0.95);
    songName = anchorText(songInfo->get_transform(), "Song Name", 0, 0.55, 1, 1.1);
    songName->set_fontSize(8);
    // songName->set_overflowMode(TMPro::TextOverflowModes::Ellipsis); doesn't work
    songName->set_alignment(TMPro::TextAlignmentOptions::MidlineLeft);
    songAuthor = anchorText(songInfo->get_transform(), "Song Author", 0, 0.49, 1, 0.6);
    songAuthor->set_fontSize(4);
    // songAuthor->set_overflowMode(TMPro::TextOverflowModes::Ellipsis);
    songAuthor->set_alignment(TMPro::TextAlignmentOptions::MidlineLeft);
    songMapper = anchorText(songInfo->get_transform(), "Song Mapper", 0, 0.29, 1, 0.425);
    songMapper->set_fontSize(4);
    // songMapper->set_overflowMode(TMPro::TextOverflowModes::Ellipsis);
    songMapper->set_alignment(TMPro::TextAlignmentOptions::MidlineLeft);
    songDifficulty = anchorText(songInfo->get_transform(), "Song Difficulty", 0, 0, 1, 0.25);
    songDifficulty->set_fontSize(6);
    // songDifficulty->set_overflowMode(TMPro::TextOverflowModes::Ellipsis);
    songDifficulty->set_alignment(TMPro::TextAlignmentOptions::MidlineLeft);
    #pragma endregion

    #pragma region playDetailsMenu
    detailsSpecifics = anchorContainer(get_transform(), 0.1, 0.77, 0.9, 1);
    // find round background
    auto sprite = ArrayUtil::First(UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Sprite*>(), [](UnityEngine::Sprite* x){ 
        return x->get_name() == "RoundRect10";
    });
    auto img = anchorImage(detailsSpecifics->get_transform(), sprite, 0.28, 0.565, 0.9, 1);
    img->skew = 0.18;
    img->gradient = true;
    img->set_color0({1, 1, 1, 1});
    img->set_color1({1, 1, 1, 0});
    img->set_color({1, 1, 1, 0.2});
    img->set_type(1);

    anchorText(detailsSpecifics->get_transform(), "SCORE DETAILS", 0.3, 0.53, 0.7, 1)->set_fontSize(5.65);

    date = anchorText(detailsSpecifics->get_transform(), "Date Played - N/A", 0, 0, 1, 0.304);
    date->set_fontSize(5);
    
    auto backButton = BeatSaberUI::CreateUIButton(detailsSpecifics->get_transform(), "", "BackButton", [this](){
        levelSelectCoordinator->SetRightScreenViewController(levelSelectCoordinator->get_leaderboardViewController(), HMUI::ViewController::AnimationType::In);
        this->get_gameObject()->set_active(false);
    });
    ANCHOR(backButton, 0.58, 0.565, 0.58, 1)
    reinterpret_cast<UnityEngine::RectTransform*>(backButton->get_transform())->set_sizeDelta({12, 2.5});
    backButton->get_gameObject()->set_name("BeatSaviorDataBackButton");
    #pragma endregion

    #pragma region overcomplicatedDeleteButton
    // delete button, mostly copied from songloader, should be in questui as some sort of createIconButton
    deleteButton = BeatSaberUI::CreateUIButton(get_transform(), "", "PracticeButton", {55, 36}, {10, 10}, [this](){
        if(lastBeatmap)
            deleteMap(lastBeatmap);
        disableDetailsButton();
        // close menu
        levelSelectCoordinator->SetRightScreenViewController(levelSelectCoordinator->get_leaderboardViewController(), HMUI::ViewController::AnimationType::In);
        get_gameObject()->set_active(false);
    });
    // as fern would say, yeet the text (and a few other things)
    auto contentTransform = deleteButton->get_transform()->Find("Content");
    Object::Destroy(contentTransform->Find("Text")->get_gameObject());
    Object::Destroy(contentTransform->GetComponent<UnityEngine::UI::LayoutElement*>());
    Object::Destroy(deleteButton->get_transform()->Find("Underline")->get_gameObject());
    // add icon
    auto iconGameObject = UnityEngine::GameObject::New_ctor("Icon");
    auto imageView = iconGameObject->AddComponent<HMUI::ImageView*>();
    auto iconTransform = imageView->get_rectTransform();
    iconTransform->SetParent(contentTransform, false);
    imageView->set_material(ArrayUtil::First(UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Material*>(), [](UnityEngine::Material* x) { return x->get_name() == "UINoGlow"; }));
    imageView->set_sprite(DeleteSprite());
    imageView->set_preserveAspect(true);
    iconTransform->set_localScale({1.7, 1.7, 1.7});
    #pragma endregion

    #pragma region globalStats
    getLogger().info("Constructing global stats");

    auto globalStats = anchorContainer(get_transform(), 0, 0.61, 1, 0.74);
    top_line = anchorImage(globalStats->get_transform(), WhiteSprite(), 0, 0.97, 1, 1);
    bottom_line = anchorImage(globalStats->get_transform(), WhiteSprite(), 0, 0, 1, 0.03);

    anchorText(globalStats->get_transform(), "Rank", 0, 0.5, 0.2, 0.9)->set_fontSize(3);
    rank = anchorText(globalStats->get_transform(), "N/A", 0, 0.1, 0.2, 0.5);
    rank->set_fontSize(5);
    anchorText(globalStats->get_transform(), "Score", 0.2, 0.5, 0.4, 0.9)->set_fontSize(3);
    percent = anchorText(globalStats->get_transform(), "N/A", 0.2, 0.1, 0.4, 0.5);
    percent->set_fontSize(5);
    anchorText(globalStats->get_transform(), "Combo", 0.4, 0.5, 0.6, 0.9)->set_fontSize(3);
    combo = anchorText(globalStats->get_transform(), "N/A", 0.4, 0.1, 0.6, 0.5);
    combo->set_fontSize(5);
    anchorText(globalStats->get_transform(), "Misses", 0.6, 0.5, 0.8, 0.9)->set_fontSize(3);
    misses = anchorText(globalStats->get_transform(), "N/A", 0.6, 0.1, 0.8, 0.5);
    misses->set_fontSize(5);
    anchorText(globalStats->get_transform(), "Pauses", 0.8, 0.5, 1, 0.9)->set_fontSize(3);
    pauses = anchorText(globalStats->get_transform(), "N/A", 0.8, 0.1, 1, 0.5);
    pauses->set_fontSize(5);
    #pragma endregion

    #pragma region leftSaber
    auto leftSaber = anchorContainer(get_transform(), 0.07, 0, 0.47, 0.6);

    l_circle = BeatSaberUI::CreateImage(leftSaber->get_transform(), CircleSprite(), {0, 0}, {0, 0});
    l_circle->set_preserveAspect(true);
    l_circle->get_transform()->set_localScale(l_circle->get_transform()->get_localScale() * 1.55);
    l_circle->set_type(UnityEngine::UI::Image::Type::Filled);
    l_circle->set_fillOrigin(UnityEngine::UI::Image::Origin360::Top);
    ANCHOR(l_circle, 0.1, 0.52, 0.4, 0.92)

    l_cut = anchorText(leftSaber->get_transform(), "N/A", 0.1, 0.5, 0.4, 0.9);
    l_cut->set_fontSize(7);
    anchorText(leftSaber->get_transform(), "Before Cut", 0.6, 0.89, 0.9, 0.94)->set_fontSize(3);
    l_beforeCut = anchorText(leftSaber->get_transform(), "N/A", 0.6, 0.79, 0.9, 0.89);
    l_beforeCut->set_fontSize(5);
    anchorText(leftSaber->get_transform(), "Accuracy", 0.6, 0.73, 0.9, 0.78)->set_fontSize(3);
    l_accuracy = anchorText(leftSaber->get_transform(), "N/A", 0.6, 0.63, 0.9, 0.73);
    l_accuracy->set_fontSize(5);
    anchorText(leftSaber->get_transform(), "After Cut", 0.6, 0.57, 0.9, 0.62)->set_fontSize(3);
    l_afterCut = anchorText(leftSaber->get_transform(), "N/A", 0.6, 0.47, 0.9, 0.57);
    l_afterCut->set_fontSize(5);
    anchorText(leftSaber->get_transform(), "Distance", 0.1, 0.35, 0.5, 0.4)->set_fontSize(3);
    l_distance = anchorText(leftSaber->get_transform(), "N/A", 0.1, 0.25, 0.5, 0.35);
    l_distance->set_fontSize(5);
    anchorText(leftSaber->get_transform(), "Speed", 0.1, 0.19, 0.5, 0.24)->set_fontSize(3);
    l_speed = anchorText(leftSaber->get_transform(), "N/A", 0.1, 0.09, 0.5, 0.19);
    l_speed->set_fontSize(5);
    anchorText(leftSaber->get_transform(), "Pre-swing", 0.5, 0.35, 0.9, 0.4)->set_fontSize(3);
    l_preSwing = anchorText(leftSaber->get_transform(), "N/A", 0.5, 0.25, 0.9, 0.35);
    l_preSwing->set_fontSize(5);
    anchorText(leftSaber->get_transform(), "Post-swing", 0.5, 0.19, 0.9, 0.24)->set_fontSize(3);
    l_postSwing = anchorText(leftSaber->get_transform(), "N/A", 0.5, 0.09, 0.9, 0.19);
    l_postSwing->set_fontSize(5);
    #pragma endregion
    
    anchorImage(get_transform(), WhiteSprite(), 0.498, 0.05, 0.502, 0.6);

    #pragma region rightSaber
    auto rightSaber = anchorContainer(get_transform(), 0.53, 0, 0.93, 0.6);

    r_circle = BeatSaberUI::CreateImage(rightSaber->get_transform(), CircleSprite(), {0, 0}, {0, 0});
    r_circle->set_preserveAspect(true);
    r_circle->get_transform()->set_localScale(r_circle->get_transform()->get_localScale() * 1.55);
    r_circle->set_type(UnityEngine::UI::Image::Type::Filled);
    r_circle->set_fillOrigin(UnityEngine::UI::Image::Origin360::Top);
    ANCHOR(r_circle, 0.6, 0.52, 0.9, 0.92)

    r_cut = anchorText(rightSaber->get_transform(), "N/A", 0.6, 0.5, 0.9, 0.9);
    r_cut->set_fontSize(7);
    anchorText(rightSaber->get_transform(), "Before Cut", 0.1, 0.89, 0.4, 0.94)->set_fontSize(3);
    r_beforeCut = anchorText(rightSaber->get_transform(), "N/A", 0.1, 0.79, 0.4, 0.89);
    r_beforeCut->set_fontSize(5);
    anchorText(rightSaber->get_transform(), "Accuracy", 0.1, 0.73, 0.4, 0.78)->set_fontSize(3);
    r_accuracy = anchorText(rightSaber->get_transform(), "N/A", 0.1, 0.63, 0.4, 0.73);
    r_accuracy->set_fontSize(5);
    anchorText(rightSaber->get_transform(), "After Cut", 0.1, 0.57, 0.4, 0.62)->set_fontSize(3);
    r_afterCut = anchorText(rightSaber->get_transform(), "N/A", 0.1, 0.47, 0.4, 0.57);
    r_afterCut->set_fontSize(5);
    anchorText(rightSaber->get_transform(), "Distance", 0.5, 0.35, 0.9, 0.4)->set_fontSize(3);
    r_distance = anchorText(rightSaber->get_transform(), "N/A", 0.5, 0.25, 0.9, 0.35);
    r_distance->set_fontSize(5);
    anchorText(rightSaber->get_transform(), "Speed", 0.5, 0.19, 0.9, 0.24)->set_fontSize(3);
    r_speed = anchorText(rightSaber->get_transform(), "N/A", 0.5, 0.09, 0.9, 0.19);
    r_speed->set_fontSize(5);
    anchorText(rightSaber->get_transform(), "Pre-swing", 0.1, 0.35, 0.5, 0.4)->set_fontSize(3);
    r_preSwing = anchorText(rightSaber->get_transform(), "N/A", 0.1, 0.25, 0.5, 0.35);
    r_preSwing->set_fontSize(5);
    anchorText(rightSaber->get_transform(), "Post-swing", 0.1, 0.19, 0.5, 0.24)->set_fontSize(3);
    r_postSwing = anchorText(rightSaber->get_transform(), "N/A", 0.1, 0.09, 0.5, 0.19);
    r_postSwing->set_fontSize(5);
    #pragma endregion

    getLogger().info("Level stats finished");
}

void LevelStats::setColors(UnityEngine::Color leftColor, UnityEngine::Color rightColor) {
    // getLogger().info("Setting UI colors");
    l_cut->set_color(leftColor);
    l_beforeCut->set_color(leftColor);
    l_afterCut->set_color(leftColor);
    l_accuracy->set_color(leftColor);
    l_distance->set_color(leftColor);
    l_speed->set_color(leftColor);
    l_preSwing->set_color(leftColor);
    l_postSwing->set_color(leftColor);
    l_circle->set_color(leftColor);
    
    r_cut->set_color(rightColor);
    r_beforeCut->set_color(rightColor);
    r_afterCut->set_color(rightColor);
    r_accuracy->set_color(rightColor);
    r_distance->set_color(rightColor);
    r_speed->set_color(rightColor);
    r_preSwing->set_color(rightColor);
    r_postSwing->set_color(rightColor);
    r_circle->set_color(rightColor);
}

const UnityEngine::Color expertPlus = UnityEngine::Color(0.5607843399, 0.2823529541, 0.8588235378, 1),
expert = UnityEngine::Color(0.7490196228, 0.7490196228, 0.2588235438, 1),
hard = UnityEngine::Color(1, 0.6470588446, 0, 1),
normal = UnityEngine::Color(0.3490196168, 0.6901960969, 0.9568627477, 1),
easy = UnityEngine::Color(0.2352941185, 0.7019608021, 0.4431372583, 1),
gold = UnityEngine::Color(0.9294117689, 0.9294117689, 0.4039215744, 1);

void LevelStats::setText(IDifficultyBeatmap* beatmap, bool resultScreen) {
    get_transform()->set_localScale({1, 1, 1});
    get_gameObject()->set_active(true);
    // get colors
    auto playerData = UnityEngine::Resources::FindObjectsOfTypeAll<PlayerDataModel*>()[0]->playerData;
    auto colors = playerData->colorSchemesSettings->GetSelectedColorScheme();

    setColors(colors->saberAColor, colors->saberBColor);

    // get cover image and other level stats
    IPreviewBeatmapLevel* levelData = reinterpret_cast<IPreviewBeatmapLevel*>(beatmap->get_level());
    auto cover = levelData->GetCoverImageAsync(System::Threading::CancellationToken::get_None());

    songCover->set_sprite(cover->get_Result());
    songName->set_text(levelData->get_songName());
    songAuthor->set_text(levelData->get_songAuthorName());
    songMapper->set_text(levelData->get_levelAuthorName());

    songCover->get_gameObject()->set_active(resultScreen);
    songName->get_gameObject()->set_active(resultScreen);
    songAuthor->get_gameObject()->set_active(resultScreen);
    songMapper->get_gameObject()->set_active(resultScreen);
    
    // parse difficulty name and corresponding color
    std::string diffName;
    UnityEngine::Color color;
    switch(beatmap->get_difficulty()) {
        case BeatmapDifficulty::Easy:
            diffName = "Easy";
            color = easy;
            break;
        case BeatmapDifficulty::Normal:
            diffName = "Normal";
            color = normal;
            break;
        case BeatmapDifficulty::Hard:
            diffName = "Hard";
            color = hard;
            break;
        case BeatmapDifficulty::Expert:
            diffName = "Expert";
            color = expert;
            break;
        case BeatmapDifficulty::ExpertPlus:
            diffName = "Expert+";
            color = expertPlus;
            break;
        default:
            diffName = "Unknown";
            color = UnityEngine::Color::get_white();
    }
    songDifficulty->set_text(diffName);
    songDifficulty->set_color(color);
    songDifficulty->get_gameObject()->set_active(resultScreen);

    date->set_text("Date Played - " + currentTracker.date);
    detailsSpecifics->set_active(!resultScreen);
    deleteButton->get_gameObject()->set_active(!resultScreen);
    
    #pragma region levelstats

    int maxScore = currentTracker.maxScore;
    // <v2 saved data won't have max score, so it will be loaded as -1
    // thankfully old data is also guaranteed to not have any new notes
    if(maxScore < 0)
        maxScore = calculateOldMaxScore(currentTracker.notes);
    float pct = currentTracker.score * 100.0 / maxScore;
    std::string rankTxt;
    if(pct >= 90)
        rankTxt = "SS";
    else if(pct >= 80)
        rankTxt = "S";
    else if(pct >= 65)
        rankTxt = "A";
    else if(pct >= 50)
        rankTxt = "B";
    else if(pct >= 35)
        rankTxt = "C";
    else if(pct >= 20)
        rankTxt = "D";
    else
        rankTxt = "E";

    rank->set_text(rankTxt);
    percent->set_text(Round(pct, "%"));
    // set colors and combo to fc if combo matches total notes
    if(currentTracker.combo == currentTracker.notes) {
        combo->set_text("FC");
        combo->set_color(gold);
        top_line->set_color(gold);
        bottom_line->set_color(gold);
    } else {
        combo->set_text(std::to_string(currentTracker.combo));
        combo->set_color(UnityEngine::Color::get_white());
        top_line->set_color(UnityEngine::Color::get_white());
        bottom_line->set_color(UnityEngine::Color::get_white());
    }
    misses->set_text(std::to_string(currentTracker.misses));
    pauses->set_text(std::to_string(currentTracker.pauses));
    #pragma endregion

    #pragma region saberpanel
    // left saber
    int l_notes = currentTracker.l_notes;
    l_cut->set_text(Round(safeDiv(currentTracker.l_cut, l_notes)));
    l_beforeCut->set_text(Round(safeDiv(currentTracker.l_beforeCut, l_notes)));
    l_afterCut->set_text(Round(safeDiv(currentTracker.l_afterCut, l_notes)));
    l_accuracy->set_text(Round(safeDiv(currentTracker.l_accuracy, l_notes)));
    l_distance->set_text(Round(currentTracker.l_distance, " m"));
    l_speed->set_text(Round(safeDiv(currentTracker.l_speed, l_notes), " Km/h"));
    l_preSwing->set_text(Round(safeDiv(currentTracker.l_preSwing * 100, l_notes), "%"));
    l_postSwing->set_text(Round(safeDiv(currentTracker.l_postSwing * 100, l_notes), "%"));
    l_circle->set_fillAmount(safeDiv(currentTracker.l_cut, l_notes) / 115);
    
    // right saber
    int r_notes = currentTracker.r_notes;
    r_cut->set_text(Round(safeDiv(currentTracker.r_cut, r_notes)));
    r_beforeCut->set_text(Round(safeDiv(currentTracker.r_beforeCut, r_notes)));
    r_afterCut->set_text(Round(safeDiv(currentTracker.r_afterCut, r_notes)));
    r_accuracy->set_text(Round(safeDiv(currentTracker.r_accuracy, r_notes)));
    r_distance->set_text(Round(currentTracker.r_distance, " m"));
    r_speed->set_text(Round(safeDiv(currentTracker.r_speed, r_notes), " Km/h"));
    r_preSwing->set_text(Round(safeDiv(currentTracker.r_preSwing * 100, r_notes), "%"));
    r_postSwing->set_text(Round(safeDiv(currentTracker.r_postSwing * 100, r_notes), "%"));
    r_circle->set_fillAmount(safeDiv(currentTracker.r_cut, r_notes) / 115);
    #pragma endregion
}

void ScoreGraph::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    getLogger().info("Min percent: %.2f, max percent: %.2f", currentTracker.min_pct, currentTracker.max_pct);

    if(graphContainer)
        UnityEngine::Object::Destroy(graphContainer);

    auto rect = get_rectTransform();

    graphContainer = anchorContainer(rect, 0.05, 0.05, 0.95, 0.87);
    
    int minPct = 0;
    if(globalConfig.NarrowGraphRange) {
        minPct = 9;
        while(minPct > 0) {
            if(currentTracker.min_pct*10 - minPct >= 10 - currentTracker.max_pct*10)
                break;
            minPct--;
        }
    }
    float step = 1.0 / (10 - minPct);
    int pctText = minPct * 10;
    for(float y = 0; pctText <= 100; pctText += 10) {
        // create a horizontal line on the graph, labeled with percentage
        anchorText(graphContainer->get_transform(), std::to_string(pctText)+"%", -0.1, y - 0.1, 0, y + 0.1);
        createLine(graphContainer->get_transform(), {0, y}, {1, y})->set_color({0.4, 0.4, 0.4, 1});
        y += step;
    }
    // extra lines if needed
    if(minPct > 4) {
        pctText = minPct*10 + 5;
        for(float y = step/2; pctText <= 100; pctText += 10) {
            anchorText(graphContainer->get_transform(), std::to_string(pctText)+"%", -0.1, y - 0.1, 0, y + 0.1)->set_color({0.6, 0.6, 0.6, 1});
            createLine(graphContainer->get_transform(), {0, y}, {1, y})->set_color({0.25, 0.25, 0.25, 1});
            y += step;
        }
    } // quarters
    if(minPct > 7) {
        pctText = minPct*10 + 2;
        step /= 2;
        for(float y = step/2; pctText < 100; pctText += 5) {
            anchorText(graphContainer->get_transform(), std::to_string(pctText)+".5%", -0.1, y - 0.1, 0, y + 0.1)->set_color({0.6, 0.6, 0.6, 1});
            createLine(graphContainer->get_transform(), {0, y}, {1, y})->set_color({0.25, 0.25, 0.25, 1});
            y += step;
        }
    }

    float pctOffset = minPct / 10.0;
    float pct, time, lastPct, lastTime = -1;
    std::sort(percents.begin(), percents.end());
    for(auto i = percents.begin(); i != percents.end(); ++i) {
        time = i->first / currentTracker.song_time;
        pct = (i->second - pctOffset) / (1 - pctOffset);

        // avoid having too many lines (at most 200)
        if(time - lastTime < 0.005)
            continue;
        // idk why this happens sometimes
        if(pct > 1 || pct < 0)
            continue;
        
        if(!(lastTime < 0))
            createLine(graphContainer->get_transform(), {lastTime, lastPct}, {time, pct});

        lastTime = time;
        lastPct = pct;
    }
    
    getLogger().info("Graph finished");
}