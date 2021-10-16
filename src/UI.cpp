#include "ModConfig.hpp"
#include "UI.hpp"

#include "questui/shared/BeatSaberUI.hpp"

#include "UnityEngine/UI/Image_origin360.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Color.hpp"
#include "UnityEngine/Material.hpp"

DEFINE_TYPE(BSDUI, LevelStats);
DEFINE_TYPE(BSDUI, ScoreGraph);

using namespace BSDUI;
using namespace QuestUI;

#define ANCHOR(component, xmin, ymin, xmax, ymax) \
component->get_rectTransform()->set_anchorMin({xmin, ymin}); \
component->get_rectTransform()->set_anchorMax({xmax, ymax});

static UnityEngine::Sprite* WhiteSprite() {
    static auto whiteSprite = QuestUI::BeatSaberUI::Base64ToSprite("/9j/4AAQSkZJRgABAQEAwADAAAD/4QAiRXhpZgAATU0AKgAAAAgAAQESAAMAAAABAAEAAAAAAAD/2wBDAAIBAQIBAQICAgICAgICAwUDAwMDAwYEBAMFBwYHBwcGBwcICQsJCAgKCAcHCg0KCgsMDAwMBwkODw0MDgsMDAz/2wBDAQICAgMDAwYDAwYMCAcIDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAwMDAz/wAARCAABAAEDASIAAhEBAxEB/8QAHwAAAQUBAQEBAQEAAAAAAAAAAAECAwQFBgcICQoL/8QAtRAAAgEDAwIEAwUFBAQAAAF9AQIDAAQRBRIhMUEGE1FhByJxFDKBkaEII0KxwRVS0fAkM2JyggkKFhcYGRolJicoKSo0NTY3ODk6Q0RFRkdISUpTVFVWV1hZWmNkZWZnaGlqc3R1dnd4eXqDhIWGh4iJipKTlJWWl5iZmqKjpKWmp6ipqrKztLW2t7i5usLDxMXGx8jJytLT1NXW19jZ2uHi4+Tl5ufo6erx8vP09fb3+Pn6/8QAHwEAAwEBAQEBAQEBAQAAAAAAAAECAwQFBgcICQoL/8QAtREAAgECBAQDBAcFBAQAAQJ3AAECAxEEBSExBhJBUQdhcRMiMoEIFEKRobHBCSMzUvAVYnLRChYkNOEl8RcYGRomJygpKjU2Nzg5OkNERUZHSElKU1RVVldYWVpjZGVmZ2hpanN0dXZ3eHl6goOEhYaHiImKkpOUlZaXmJmaoqOkpaanqKmqsrO0tba3uLm6wsPExcbHyMnK0tPU1dbX2Nna4uPk5ebn6Onq8vP09fb3+Pn6/9oADAMBAAIRAxEAPwD9/KKKKAP/2Q==");
    return whiteSprite;
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
    static auto name = il2cpp_utils::createcsstr("BSDUIContainer");
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

void LevelStats::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    if(!firstActivation) return;

    getLogger().info("Constructing level stats menu");

    auto rect = reinterpret_cast<HMUI::ViewController*>(this)->get_rectTransform();

    auto circleSprite = ArrayUtil::First(UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Sprite*>(), [](UnityEngine::Sprite* x){
        return to_utf8(csstrtostr(x->get_name())) == "Circle";
    });

    #pragma region songInfo
    auto song = anchorContainer(rect, 0, 0.75, 1, 1);
    
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

    #pragma region globalStats
    getLogger().info("Constructing global stats");

    auto globalStats = anchorContainer(rect, 0, 0.61, 1, 0.74);
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
    auto leftSaber = anchorContainer(rect, 0.07, 0, 0.47, 0.6);

    l_circle = BeatSaberUI::CreateImage(leftSaber->get_transform(), circleSprite, {0, 0}, {0, 0});
    l_circle->set_overrideSprite(circleSprite);
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
    
    anchorImage(rect, WhiteSprite(), 0.498, 0.05, 0.502, 0.6);

    #pragma region rightSaber
    auto rightSaber = anchorContainer(rect, 0.53, 0, 0.93, 0.6);

    r_circle = BeatSaberUI::CreateImage(rightSaber->get_transform(), circleSprite, {0, 0}, {0, 0});
    r_circle->set_overrideSprite(circleSprite);
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

    getLogger().info("Menu finished");
}

void LevelStats::setColors(UnityEngine::Color leftColor, UnityEngine::Color rightColor) {
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

static const float graphScale = 66;
static const float graphLength = 1.7;

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

void ScoreGraph::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    getLogger().info("Min percent: %.2f, max percent: %.2f", tracker.min_pct, tracker.max_pct);

    if(graphContainer)
        UnityEngine::Object::Destroy(graphContainer);

    auto rect = reinterpret_cast<HMUI::ViewController*>(this)->get_rectTransform();

    graphContainer = anchorContainer(rect, 0.05, 0.05, 0.95, 0.87);
    
    int minPct = 0;
    if(getModConfig().GraphRange.GetValue()) {
        minPct = 9;
        while(minPct > 0) {
            if(tracker.min_pct*10 - minPct >= 10 - tracker.max_pct*10)
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
        time = i->first / tracker.song_time;
        pct = (i->second - pctOffset) / (1 - pctOffset);

        // avoid having too many lines (at most 1000)
        if(time - lastTime < 0.001)
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