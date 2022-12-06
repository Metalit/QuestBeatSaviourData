#include "main.hpp"
#include "stats.hpp"
#include "sprites.hpp"
#include "localdata.hpp"
#include "settings.hpp"

#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"

#include "bs-utils/shared/utils.hpp"

#include "overswing/shared/callback.hpp"

#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "custom-types/shared/register.hpp"

#include "HMUI/ViewController_AnimationType.hpp"

#include "System/DateTime.hpp"
#include "System/Threading/Tasks/Task_1.hpp"

#include "UnityEngine/Sprite.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Color.hpp"
#include "UnityEngine/Vector4.hpp"
#include "UnityEngine/Sprites/DataUtility.hpp"
#include "UnityEngine/UIVertex.hpp"
#include "UnityEngine/UI/VertexHelper.hpp"
#include "UnityEngine/Mathf.hpp"

#include "GlobalNamespace/SoloFreePlayFlowCoordinator.hpp"
#include "GlobalNamespace/PartyFreePlayFlowCoordinator.hpp"
#include "GlobalNamespace/EnterPlayerGuestNameViewController.hpp"
#include "GlobalNamespace/LevelCompletionResults.hpp"
#include "GlobalNamespace/IPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/BeatmapData.hpp"
#include "GlobalNamespace/GameplayModifiers.hpp"
#include "GlobalNamespace/SinglePlayerLevelSelectionFlowCoordinator.hpp"
#include "GlobalNamespace/MenuTransitionsHelper.hpp"
#include "GlobalNamespace/PlatformLeaderboardViewController.hpp"
#include "GlobalNamespace/LeaderboardsModel.hpp"
#include "GlobalNamespace/LocalLeaderboardViewController.hpp"
#include "GlobalNamespace/PauseMenuManager.hpp"
#include "GlobalNamespace/NoteCutInfo.hpp"
#include "GlobalNamespace/CutScoreBuffer.hpp"
#include "GlobalNamespace/SaberSwingRatingCounter.hpp"
#include "GlobalNamespace/SaberSwingRating.hpp"
#include "GlobalNamespace/GoodCutScoringElement.hpp"
#include "GlobalNamespace/BadCutScoringElement.hpp"
#include "GlobalNamespace/MissScoringElement.hpp"
#include "GlobalNamespace/SaberMovementData.hpp"
#include "GlobalNamespace/ScoreModel_NoteScoreDefinition.hpp"
#include "GlobalNamespace/ScoreController.hpp"
#include "GlobalNamespace/BladeMovementDataElement.hpp"
#include "GlobalNamespace/NoteData.hpp"

#include <sstream>

using namespace GlobalNamespace;
using namespace BeatSaviorData;

const std::unordered_set<int> allowedNoteTypes = {
    NoteData::ScoringType::Normal,
    NoteData::ScoringType::SliderHead,
    NoteData::ScoringType::SliderTail
};

LevelStats* levelStatsView = nullptr;
ScoreGraph* scoreGraphView = nullptr;
UnityEngine::UI::Button* detailsButton;

ModInfo modInfo;
IDifficultyBeatmap* lastBeatmap = nullptr;
LevelCompletionResults* lastCompletionResults;
std::vector<std::pair<float, float>> percents;
SinglePlayerLevelSelectionFlowCoordinator* levelSelectCoordinator;

Logger& getLogger() {
    static Logger* logger = new Logger(modInfo);
    return *logger;
}

std::string GetConfigPath() {
    static std::string configPath = Configuration::getConfigFilePath(modInfo);
    return configPath;
}

std::string GetDataPath() {
    static std::string dataPath = getDataDir(modInfo) + "data.json";
    return dataPath;
}

inline UnityEngine::Color32 colorToc32(UnityEngine::Color color) {
    return UnityEngine::Color32(color.r * 255, color.g * 255, color.b * 255, color.a * 255);
}
inline UnityEngine::Vector2 v3tov2(UnityEngine::Vector3 v) {
    return UnityEngine::Vector2(v.x, v.y);
}

void disableDetailsButton() {
    if(detailsButton)
        detailsButton->get_gameObject()->set_active(false);
}

std::optional<std::string> modifierString(GameplayModifiers* modifiers, bool failed = false) {
    if(modifiers->IsWithoutModifiers())
        return std::nullopt;
    std::stringstream ss;
    if(modifiers->disappearingArrows) ss << "DA, ";
    if(modifiers->songSpeed == GameplayModifiers::SongSpeed::Faster) ss << "FS, ";
    if(modifiers->songSpeed == GameplayModifiers::SongSpeed::Slower) ss << "SS, ";
    if(modifiers->songSpeed == GameplayModifiers::SongSpeed::SuperFast) ss << "SFS, ";
    if(modifiers->strictAngles) ss << "SA, ";
    if(modifiers->proMode) ss << "PM, ";
    if(modifiers->smallCubes) ss << "SC, ";
    if(modifiers->failOnSaberClash) ss << "SC, ";
    if(modifiers->ghostNotes) ss << "GN, ";
    if(modifiers->noArrows) ss << "NA, ";
    if(modifiers->noBombs) ss << "NB, ";
    if(modifiers->noFailOn0Energy && failed) ss << "NF, ";
    if(modifiers->enabledObstacleType == GameplayModifiers::EnabledObstacleType::NoObstacles) ss << "NO, ";
    ss.seekp(-2, std::ios_base::end);
    return ss.str();
}

// function to avoid duplicate code
void processResults(SinglePlayerLevelSelectionFlowCoordinator* self, LevelCompletionResults* levelCompletionResults, IDifficultyBeatmap* difficultyBeatmap, bool practice) {
    if(!levelStatsView)
        levelStatsView = QuestUI::BeatSaberUI::CreateViewController<LevelStats*>();
    if(!scoreGraphView)
        scoreGraphView = QuestUI::BeatSaberUI::CreateViewController<ScoreGraph*>();

    bool failed = levelCompletionResults->levelEndStateType == LevelCompletionResults::LevelEndStateType::Failed;

    // exit through pause or something
    if(levelCompletionResults->levelEndStateType == LevelCompletionResults::LevelEndStateType::Incomplete)
        return;

    if(!getConfig().ShowOnPass.GetValue() && levelCompletionResults->levelEndStateType == LevelCompletionResults::LevelEndStateType::Cleared)
        return;

    if(!getConfig().ShowOnFail.GetValue() && failed)
        return;

    if(levelCompletionResults->multipliedScore == 0)
        return;

    self->SetLeftScreenViewController(levelStatsView, HMUI::ViewController::AnimationType::None);

    if(getConfig().ShowGraph.GetValue())
        self->SetRightScreenViewController(scoreGraphView, HMUI::ViewController::AnimationType::None);

    // add completion results to currentTracker
    currentTracker.l_distance = levelCompletionResults->leftSaberMovementDistance;
    currentTracker.r_distance = levelCompletionResults->rightSaberMovementDistance;
    // check note counts
    currentTracker.combo = levelCompletionResults->maxCombo;
    currentTracker.score = levelCompletionResults->multipliedScore;
    currentTracker.modifiers = modifierString(levelCompletionResults->gameplayModifiers, failed);

    auto time = System::DateTime::get_Now().ToLocalTime();
    currentTracker.date = std::string(time.ToString("dddd, d MMMM yyyy h:mm tt"));

    levelStatsView->setText(difficultyBeatmap);

    // don't save on practice or fails
    if(getConfig().SaveLocally.GetValue() && !practice && !failed && bs_utils::Submission::getEnabled()) {
        SaveMap(difficultyBeatmap);
        if(detailsButton)
            detailsButton->get_gameObject()->set_active(true);
    }
}

// another function for solo/party overlap
bool flashed = false;
void primeLevelStats(LeaderboardViewController* self, IDifficultyBeatmap* difficultyBeatmap) {
    static ConstString buttonName("BeatSaviorDataDetailsButton");

    // use button tranform as a marker to construct everything after a soft restart
    auto buttonTransform = self->get_transform()->Find(buttonName);
    if(!buttonTransform) {
        LOG_INFO("Creating details button");
        detailsButton = QuestUI::BeatSaberUI::CreateUIButton(self->get_transform(), "...", [](){
            // LOG_INFO("Displaying level stats from file");
            if(!lastBeatmap)
                return;
            levelSelectCoordinator->SetRightScreenViewController(levelStatsView, HMUI::ViewController::AnimationType::In);
            LoadMap(lastBeatmap);
            levelStatsView->setText(lastBeatmap, false);
        });
        ((UnityEngine::RectTransform*) detailsButton->get_transform())->set_sizeDelta({10, 10});
        detailsButton->GetComponentInChildren<TMPro::TextMeshProUGUI*>()->set_margin({-14.5, 0, 0, 0});
        detailsButton->get_gameObject()->set_name(buttonName);
    } else
        detailsButton = buttonTransform->GetComponent<UnityEngine::UI::Button*>();

    if(!levelStatsView || !buttonTransform) {
        levelStatsView = QuestUI::BeatSaberUI::CreateViewController<LevelStats*>();
        scoreGraphView = QuestUI::BeatSaberUI::CreateViewController<ScoreGraph*>();
        flashed = false;
    }
    levelStatsView->get_gameObject()->set_active(false);

    // if someone could please give me a better solution for the back button, it would be much appreciated
    if(!flashed) {
        levelStatsView->get_transform()->set_localScale({0, 0, 0});
        levelSelectCoordinator->SetRightScreenViewController(levelStatsView, HMUI::ViewController::AnimationType::None);
        flashed = true;
    }

    lastBeatmap = difficultyBeatmap;
}

// show results when finishing a level in solo mode
MAKE_HOOK_MATCH(ProcessResultsSolo, &SoloFreePlayFlowCoordinator::ProcessLevelCompletionResultsAfterLevelDidFinish, void, SoloFreePlayFlowCoordinator* self, LevelCompletionResults* levelCompletionResults, IReadonlyBeatmapData* transformedBeatmapData, IDifficultyBeatmap* difficultyBeatmap, GameplayModifiers* gameplayModifiers, bool practice) {
    ProcessResultsSolo(self, levelCompletionResults, transformedBeatmapData, difficultyBeatmap, gameplayModifiers, practice);
    processResults(self, levelCompletionResults, difficultyBeatmap, practice);
}

// show results when finishing a level in party mode
MAKE_HOOK_MATCH(ProcessResultsParty, &PartyFreePlayFlowCoordinator::ProcessLevelCompletionResultsAfterLevelDidFinish, void, PartyFreePlayFlowCoordinator* self, LevelCompletionResults* levelCompletionResults, IReadonlyBeatmapData* transformedBeatmapData, IDifficultyBeatmap* difficultyBeatmap, GameplayModifiers* gameplayModifiers, bool practice) {
    ProcessResultsParty(self, levelCompletionResults, transformedBeatmapData, difficultyBeatmap, gameplayModifiers, practice);
    // don't show if the enter name view will be displayed
    if(self->WillScoreGoToLeaderboard(levelCompletionResults, LeaderboardsModel::GetLeaderboardID(difficultyBeatmap), practice)) {
        lastBeatmap = difficultyBeatmap;
        lastCompletionResults = levelCompletionResults;
    } else {
        processResults(self, levelCompletionResults, difficultyBeatmap, practice);
    }
}

// make sure results show after entering a name
MAKE_HOOK_MATCH(PostNameResultsParty, &EnterPlayerGuestNameViewController::OkButtonPressed, void, EnterPlayerGuestNameViewController* self) {
    PostNameResultsParty(self);
    processResults(levelSelectCoordinator, lastCompletionResults, lastBeatmap, false); // guaranteed to be cleared and not in practice mode
}

// manages stats view on the solo leaderboard
MAKE_HOOK_MATCH(LevelLeaderboardSolo, &PlatformLeaderboardViewController::SetData, void, PlatformLeaderboardViewController* self, IDifficultyBeatmap* difficultyBeatmap) {
    LevelLeaderboardSolo(self, difficultyBeatmap);

    auto flowCoordinatorArr = UnityEngine::Resources::FindObjectsOfTypeAll<SoloFreePlayFlowCoordinator*>();
    if(flowCoordinatorArr.Length() < 1)
        return;
    levelSelectCoordinator = flowCoordinatorArr[0];

    primeLevelStats(self, difficultyBeatmap);

    // ui is a war
    detailsButton->get_transform()->SetParent(self->get_transform(), false);
    auto rect = reinterpret_cast<UnityEngine::RectTransform*>(detailsButton->get_transform());
    rect->set_anchoredPosition({-39.5, -36});
    detailsButton->get_gameObject()->set_active(MapSaved(difficultyBeatmap));
}

// manages stats view on the party leaderboard
MAKE_HOOK_MATCH(LevelLeaderboardParty, &LocalLeaderboardViewController::SetData, void, LocalLeaderboardViewController* self, IDifficultyBeatmap* difficultyBeatmap) {
    LevelLeaderboardParty(self, difficultyBeatmap);

    auto flowCoordinatorArr = UnityEngine::Resources::FindObjectsOfTypeAll<PartyFreePlayFlowCoordinator*>();
    if(flowCoordinatorArr.Length() < 1)
        return;
    levelSelectCoordinator = flowCoordinatorArr[0];

    primeLevelStats(self, difficultyBeatmap);

    // ui is a war
    detailsButton->get_transform()->SetParent(self->get_transform(), false);
    auto rect = reinterpret_cast<UnityEngine::RectTransform*>(detailsButton->get_transform());
    rect->set_anchoredPosition({-39.5, 23});
    detailsButton->get_gameObject()->set_active(MapSaved(difficultyBeatmap));
}

// resets trackers when starting a level
MAKE_HOOK_MATCH(LevelPlay, static_cast<void(MenuTransitionsHelper::*)(StringW, IDifficultyBeatmap*, IPreviewBeatmapLevel*, OverrideEnvironmentSettings*, ColorScheme*, GameplayModifiers*, PlayerSpecificSettings*, PracticeSettings*, StringW, bool, bool, System::Action*, System::Action_1<Zenject::DiContainer*>*, System::Action_2<StandardLevelScenesTransitionSetupDataSO*, LevelCompletionResults*>*, System::Action_2<LevelScenesTransitionSetupDataSO*, LevelCompletionResults*>*)>(&MenuTransitionsHelper::StartStandardLevel),
        void, MenuTransitionsHelper* self, StringW f1, IDifficultyBeatmap* f2, IPreviewBeatmapLevel* f3, OverrideEnvironmentSettings* f4, ColorScheme* f5, GameplayModifiers* f6, PlayerSpecificSettings* f7, PracticeSettings* f8, StringW f9, bool f10, bool f11, System::Action* f12, System::Action_1<Zenject::DiContainer*>* f13, System::Action_2<StandardLevelScenesTransitionSetupDataSO*, LevelCompletionResults*>* f14, System::Action_2<LevelScenesTransitionSetupDataSO*, LevelCompletionResults*>* f15) {
    LevelPlay(self, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15);
    // reset current tracker
    currentTracker = {};
    currentTracker.min_pct = 1;
    currentTracker.song_time = f3->get_songDuration();
    percents.clear();
    // disable score view in level select if needed
    if(levelSelectCoordinator)
        levelSelectCoordinator->SetRightScreenViewController(levelSelectCoordinator->get_leaderboardViewController(), HMUI::ViewController::AnimationType::In);
    if(levelStatsView)
        levelStatsView->get_gameObject()->set_active(false);
}

// tracks pauses
MAKE_HOOK_MATCH(LevelPause, &PauseMenuManager::ShowMenu, void, PauseMenuManager* self) {
    LevelPause(self);
    currentTracker.pauses++;
}

// adds note events to the tracker (cuts, bad cuts, misses)
MAKE_HOOK_MATCH(NoteEventFinish, &ScoreController::DespawnScoringElement, void, ScoreController* self, ScoringElement* scoringElement) {
    NoteEventFinish(self, scoringElement);

    bool isBomb = scoringElement->get_noteData()->colorType == ColorType::None;

    if(!isBomb) {
        currentTracker.notes++;
        currentTracker.maxScore = self->immediateMaxPossibleMultipliedScore;
    }

    if(auto goodElement = il2cpp_utils::try_cast<GoodCutScoringElement>(scoringElement)) {
        if(!allowedNoteTypes.contains(scoringElement->get_noteData()->scoringType))
            return;

        auto buffer = goodElement.value()->cutScoreBuffer;

        currentTracker.fullNotesScore += scoringElement->get_cutScore() * scoringElement->get_multiplier();

        if(buffer->noteCutInfo.saberType == SaberType::SaberA) {
            currentTracker.l_notes++;
            currentTracker.l_cut += buffer->get_cutScore();
            currentTracker.l_beforeCut += buffer->get_beforeCutScore();
            currentTracker.l_afterCut += buffer->get_afterCutScore();
            currentTracker.l_accuracy += buffer->get_centerDistanceCutScore();
            currentTracker.l_speed += buffer->noteCutInfo.saberSpeed;
        } else {
            currentTracker.r_notes++;
            currentTracker.r_cut += buffer->get_cutScore();
            currentTracker.r_beforeCut += buffer->get_beforeCutScore();
            currentTracker.r_afterCut += buffer->get_afterCutScore();
            currentTracker.r_accuracy += buffer->get_centerDistanceCutScore();
            currentTracker.r_speed += buffer->noteCutInfo.saberSpeed;
        }
    } else if(il2cpp_utils::try_cast<BadCutScoringElement>(scoringElement)) {
        currentTracker.misses++;
    } else if(il2cpp_utils::try_cast<MissScoringElement>(scoringElement)) {
        if(!isBomb)
            currentTracker.misses++;
    }

    float pct = self->multipliedScore / (float) self->immediateMaxPossibleMultipliedScore;

    if(pct > 0 && pct < currentTracker.min_pct)
        currentTracker.min_pct = pct;
    if(pct < 1 && pct > currentTracker.max_pct)
        currentTracker.max_pct = pct;

    float time = scoringElement->get_time();

    percents.push_back(std::make_pair(time, pct));
}

// override to use customAddQuad, which simply does the same thing as AddQuad which would be in its place but also accounts for curved canvases
void customAddQuad(UnityEngine::UI::VertexHelper* vertexHelper, ArrayW<UnityEngine::Vector3> quadPositions, UnityEngine::Color32 color, ArrayW<UnityEngine::Vector3> quadUVs, float curvedUIRadius) {
    int currentVertCount = vertexHelper->get_currentVertCount();
    UnityEngine::Vector2 uv = UnityEngine::Vector2(curvedUIRadius, 0);
    for (int i = 0; i < 4; i++) {
        vertexHelper->AddVert(quadPositions[i], color, v3tov2(quadUVs[i]), HMUI::ImageView::_get_kVec2Zero(), uv, HMUI::ImageView::_get_kVec2Zero(), HMUI::ImageView::_get_kVec3Zero(), HMUI::ImageView::_get_kVec4Zero());
    }
    vertexHelper->AddTriangle(currentVertCount, currentVertCount + 1, currentVertCount + 2);
    vertexHelper->AddTriangle(currentVertCount + 2, currentVertCount + 3, currentVertCount);
}
MAKE_HOOK_MATCH(FillImage, &HMUI::ImageView::GenerateFilledSprite, void, HMUI::ImageView* self, UnityEngine::UI::VertexHelper* toFill, bool preserveAspect, float curvedUIRadius) {
    toFill->Clear();
    if (self->get_fillAmount() < 0.001) {
        return;
    }
    UnityEngine::Vector4 drawingDimensions = self->GetDrawingDimensions(preserveAspect);
    UnityEngine::Vector4 obj = (self->get_overrideSprite() ? UnityEngine::Sprites::DataUtility::GetOuterUV(self->get_overrideSprite()) : UnityEngine::Vector4::get_zero());
    UnityEngine::UIVertex simpleVert = UnityEngine::UIVertex::_get_simpleVert();
    simpleVert.color = colorToc32(self->get_color());
    float num = obj.x;
    float num2 = obj.y;
    float num3 = obj.z;
    float num4 = obj.w;
    if (self->get_fillMethod() == UnityEngine::UI::Image::FillMethod::Horizontal || self->get_fillMethod() == UnityEngine::UI::Image::FillMethod::Vertical) {
        if (self->get_fillMethod() == UnityEngine::UI::Image::FillMethod::Horizontal) {
            float num5 = (num3 - num) * self->get_fillAmount();
            if (self->get_fillOrigin() == 1) {
                drawingDimensions.x = drawingDimensions.z - (drawingDimensions.z - drawingDimensions.x) * self->get_fillAmount();
                num = num3 - num5;
            } else {
                drawingDimensions.z = drawingDimensions.x + (drawingDimensions.z - drawingDimensions.x) * self->get_fillAmount();
                num3 = num + num5;
            }
        } else if (self->get_fillMethod() == UnityEngine::UI::Image::FillMethod::Vertical) {
            float num6 = (num4 - num2) * self->get_fillAmount();
            if (self->get_fillOrigin() == 1) {
                drawingDimensions.y = drawingDimensions.w - (drawingDimensions.w - drawingDimensions.y) * self->get_fillAmount();
                num2 = num4 - num6;
            } else {
                drawingDimensions.w = drawingDimensions.y + (drawingDimensions.w - drawingDimensions.y) * self->get_fillAmount();
                num4 = num2 + num6;
            }
        }
    }
    self->_get_s_Xy()[0] = UnityEngine::Vector3(drawingDimensions.x, drawingDimensions.y, 0);
    self->_get_s_Xy()[1] = UnityEngine::Vector3(drawingDimensions.x, drawingDimensions.w, 0);
    self->_get_s_Xy()[2] = UnityEngine::Vector3(drawingDimensions.z, drawingDimensions.w, 0);
    self->_get_s_Xy()[3] = UnityEngine::Vector3(drawingDimensions.z, drawingDimensions.y, 0);
    self->_get_s_Uv()[0] = UnityEngine::Vector3(num, num2, 0);
    self->_get_s_Uv()[1] = UnityEngine::Vector3(num, num4, 0);
    self->_get_s_Uv()[2] = UnityEngine::Vector3(num3, num4, 0);
    self->_get_s_Uv()[3] = UnityEngine::Vector3(num3, num2, 0);
    if (self->get_fillAmount() < 1 && self->get_fillMethod() != 0 && self->get_fillMethod() != UnityEngine::UI::Image::FillMethod::Vertical) {
        if (self->get_fillMethod() == UnityEngine::UI::Image::FillMethod::Radial90) {
            if (HMUI::ImageView::RadialCut(self->_get_s_Xy(), self->_get_s_Uv(), self->get_fillAmount(), self->get_fillClockwise(), self->get_fillOrigin())) {
                customAddQuad(toFill, self->_get_s_Xy(), colorToc32(self->get_color()), self->_get_s_Uv(), curvedUIRadius);
            }
        } else if (self->get_fillMethod() == UnityEngine::UI::Image::FillMethod::Radial180) {
            for (int i = 0; i < 2; i++) {
                int num7 = ((self->get_fillOrigin() > 1) ? 1 : 0);
                float t;
                float t2;
                float t3;
                float t4;
                if (self->get_fillOrigin() == 0 || self->get_fillOrigin() == 2) {
                    t = 0;
                    t2 = 1;
                    if (i == num7) {
                        t3 = 0;
                        t4 = 0.5;
                    } else {
                        t3 = 0.5;
                        t4 = 1;
                    }
                } else {
                    t3 = 0;
                    t4 = 1;
                    if (i == num7) {
                        t = 0.5;
                        t2 = 1;
                    } else {
                        t = 0;
                        t2 = 0.5;
                    }
                }
                self->_get_s_Xy()[0].x = UnityEngine::Mathf::Lerp(drawingDimensions.x, drawingDimensions.z, t3);
                self->_get_s_Xy()[1].x = self->_get_s_Xy()[0].x;
                self->_get_s_Xy()[2].x = UnityEngine::Mathf::Lerp(drawingDimensions.x, drawingDimensions.z, t4);
                self->_get_s_Xy()[3].x = self->_get_s_Xy()[2].x;
                self->_get_s_Xy()[0].y = UnityEngine::Mathf::Lerp(drawingDimensions.y, drawingDimensions.w, t);
                self->_get_s_Xy()[1].y = UnityEngine::Mathf::Lerp(drawingDimensions.y, drawingDimensions.w, t2);
                self->_get_s_Xy()[2].y = self->_get_s_Xy()[1].y;
                self->_get_s_Xy()[3].y = self->_get_s_Xy()[0].y;
                self->_get_s_Uv()[0].x = UnityEngine::Mathf::Lerp(num, num3, t3);
                self->_get_s_Uv()[1].x = self->_get_s_Uv()[0].x;
                self->_get_s_Uv()[2].x = UnityEngine::Mathf::Lerp(num, num3, t4);
                self->_get_s_Uv()[3].x = self->_get_s_Uv()[2].x;
                self->_get_s_Uv()[0].y = UnityEngine::Mathf::Lerp(num2, num4, t);
                self->_get_s_Uv()[1].y = UnityEngine::Mathf::Lerp(num2, num4, t2);
                self->_get_s_Uv()[2].y = self->_get_s_Uv()[1].y;
                self->_get_s_Uv()[3].y = self->_get_s_Uv()[0].y;
                float value = (self->get_fillClockwise() ? (self->get_fillAmount() * 2 - (float)i) : (self->get_fillAmount() * 2 - (float)(1 - i)));
                if (HMUI::ImageView::RadialCut(self->_get_s_Xy(), self->_get_s_Uv(), UnityEngine::Mathf::Clamp01(value), self->get_fillClockwise(), (i + self->get_fillOrigin() + 3) % 4)) {
                    customAddQuad(toFill, self->_get_s_Xy(), colorToc32(self->get_color()), self->_get_s_Uv(), curvedUIRadius);
                }
            }
        } else {
            if (self->get_fillMethod() != HMUI::ImageView::FillMethod::Radial360)
                return;
            for (int j = 0; j < 4; j++) {
                float t5;
                float t6;
                if (j < 2) {
                    t5 = 0;
                    t6 = 0.5;
                } else {
                    t5 = 0.5;
                    t6 = 1;
                }
                float t7;
                float t8;
                if (j == 0 || j == 3) {
                    t7 = 0;
                    t8 = 0.5;
                } else {
                    t7 = 0.5;
                    t8 = 1;
                }
                // calculate uvs and point borders of quarter
                self->_get_s_Xy()[0].x = UnityEngine::Mathf::Lerp(drawingDimensions.x, drawingDimensions.z, t5);
                self->_get_s_Xy()[1].x = self->_get_s_Xy()[0].x;
                self->_get_s_Xy()[2].x = UnityEngine::Mathf::Lerp(drawingDimensions.x, drawingDimensions.z, t6);
                self->_get_s_Xy()[3].x = self->_get_s_Xy()[2].x;
                self->_get_s_Xy()[0].y = UnityEngine::Mathf::Lerp(drawingDimensions.y, drawingDimensions.w, t7);
                self->_get_s_Xy()[1].y = UnityEngine::Mathf::Lerp(drawingDimensions.y, drawingDimensions.w, t8);
                self->_get_s_Xy()[2].y = self->_get_s_Xy()[1].y;
                self->_get_s_Xy()[3].y = self->_get_s_Xy()[0].y;
                self->_get_s_Uv()[0].x = UnityEngine::Mathf::Lerp(num, num3, t5);
                self->_get_s_Uv()[1].x = self->_get_s_Uv()[0].x;
                self->_get_s_Uv()[2].x = UnityEngine::Mathf::Lerp(num, num3, t6);
                self->_get_s_Uv()[3].x = self->_get_s_Uv()[2].x;
                self->_get_s_Uv()[0].y = UnityEngine::Mathf::Lerp(num2, num4, t7);
                self->_get_s_Uv()[1].y = UnityEngine::Mathf::Lerp(num2, num4, t8);
                self->_get_s_Uv()[2].y = self->_get_s_Uv()[1].y;
                self->_get_s_Uv()[3].y = self->_get_s_Uv()[0].y;
                // find angle to fill in the quarter
                float value2 = (self->get_fillClockwise() ? (self->get_fillAmount() * 4 - (float)((j + self->get_fillOrigin()) % 4)) : (self->get_fillAmount() * 4 - (float)(3 - (j + self->get_fillOrigin()) % 4)));
                // check that the quarter is filled in at all, and cut the dimensions to how they should be
                if (HMUI::ImageView::RadialCut(self->_get_s_Xy(), self->_get_s_Uv(), UnityEngine::Mathf::Clamp01(value2), self->get_fillClockwise(), (j + 2) % 4)) {
                    customAddQuad(toFill, self->_get_s_Xy(), colorToc32(self->get_color()), self->_get_s_Uv(), curvedUIRadius);
                }
            }
        }
    }
    else {
        float x = self->get_transform()->get_localScale().x;
        HMUI::ImageView::AddQuad(toFill, v3tov2(self->_get_s_Xy()[0]), v3tov2(self->_get_s_Xy()[2]), colorToc32(self->get_color()), v3tov2(self->_get_s_Uv()[0]), v3tov2(self->_get_s_Uv()[2]), x, curvedUIRadius);
    }
}

#include "HMUI/NoTransitionsButton.hpp"
#include "UnityEngine/UI/Selectable_SelectionState.hpp"

// painful hook to set the back button to gray
MAKE_HOOK_MATCH(ButtonTransition, &HMUI::NoTransitionsButton::DoStateTransition, void, HMUI::NoTransitionsButton* self, UnityEngine::UI::Selectable::SelectionState state, bool instant) {
    ButtonTransition(self, state, instant);

    if(self->get_gameObject()->get_name() != "BeatSaviorDataBackButton")
        return;

    // set colors for the back button
    auto bg = self->GetComponentsInChildren<UnityEngine::UI::Image*>()[0];

    switch (state) {
        case 0: // normal
            bg->set_color({1, 1, 1, 0.2});
            break;
        case 1: // highlighted
            bg->set_color({1, 1, 1, 0.3});
            break;
        case 2: // pressed
            bg->set_color({1, 1, 1, 0.3});
            break;
        case 3: // selected
            bg->set_color({1, 1, 1, 0.3});
            break;
        case 4: // disabled
            bg->set_color({1, 1, 1, 0.1});
            break;
        default:
            bg->set_color({1, 1, 1, 0.2});
            break;
    }
}

extern "C" void setup(ModInfo& info) {
    info.id = ID;
    info.version = VERSION;
    modInfo = info;

    getConfig().Init(modInfo);

    if(!direxists(getDataDir(modInfo)))
        mkpath(getDataDir(modInfo));

    if(!fileexists(GetDataPath()))
        writefile(GetDataPath(), "null");
    LoadData();
}

extern "C" void load() {
    LOG_INFO("Starting setup...");
    il2cpp_functions::Init();

    custom_types::Register::AutoRegister();

    QuestUI::Init();
    QuestUI::Register::RegisterModSettingsViewController(modInfo, DidActivate);

    overswingCallbacks.addCallback(*[](SwingInfo info) {
        if(info.rightSaber) {
            currentTracker.r_preSwing += info.preSwing;
            currentTracker.r_postSwing += info.postSwing;
        } else {
            currentTracker.l_preSwing += info.preSwing;
            currentTracker.l_postSwing += info.postSwing;
        }
    });

    LOG_INFO("Installing hooks...");
    LoggerContextObject logger = getLogger().WithContext("load");
    INSTALL_HOOK(logger, ProcessResultsSolo);
    INSTALL_HOOK(logger, ProcessResultsParty);
    INSTALL_HOOK(logger, PostNameResultsParty);
    INSTALL_HOOK(logger, LevelPlay);
    INSTALL_HOOK(logger, LevelPause);
    INSTALL_HOOK(logger, NoteEventFinish);
    INSTALL_HOOK_ORIG(logger, FillImage);
    INSTALL_HOOK(logger, LevelLeaderboardSolo);
    INSTALL_HOOK(logger, LevelLeaderboardParty);
    INSTALL_HOOK(logger, ButtonTransition);
    LOG_INFO("Installed all hooks!");
}