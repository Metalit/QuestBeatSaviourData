#include "ModConfig.hpp"
#include "UI.hpp"

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/utils/utils.h"
#include "beatsaber-hook/shared/utils/hooking.hpp"

#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "config-utils/shared/config-utils.hpp"
#include "custom-types/shared/register.hpp"

#include "System/Threading/CancellationToken.hpp"
#include "System/Threading/Tasks/Task_1.hpp"

#include "HMUI/ViewController_AnimationType.hpp"

#include "UnityEngine/Sprite.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Color.hpp"
#include "UnityEngine/Vector4.hpp"
#include "UnityEngine/Sprites/DataUtility.hpp"
#include "UnityEngine/UIVertex.hpp"
#include "UnityEngine/UI/VertexHelper.hpp"
#include "UnityEngine/Mathf.hpp"

#include "GlobalNamespace/SoloFreePlayFlowCoordinator.hpp"
#include "GlobalNamespace/LevelCompletionResults.hpp"
#include "GlobalNamespace/LevelCompletionResultsHelper.hpp"
#include "GlobalNamespace/IBeatmapLevel.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"
#include "GlobalNamespace/BeatmapDifficulty.hpp"
#include "GlobalNamespace/BeatmapData.hpp"
#include "GlobalNamespace/GameplayModifiers.hpp"
#include "GlobalNamespace/SinglePlayerLevelSelectionFlowCoordinator.hpp"
#include "GlobalNamespace/PauseMenuManager.hpp"
#include "GlobalNamespace/NoteCutInfo.hpp"
#include "GlobalNamespace/GameNoteController.hpp"
#include "GlobalNamespace/SaberSwingRatingCounter.hpp"
#include "GlobalNamespace/SaberSwingRating.hpp"
#include "GlobalNamespace/SaberMovementData.hpp"
#include "GlobalNamespace/ScoreModel.hpp"
#include "GlobalNamespace/ScoreController.hpp"
#include "GlobalNamespace/BladeMovementDataElement.hpp"
#include "GlobalNamespace/PlayerDataModel.hpp"
#include "GlobalNamespace/PlayerData.hpp"
#include "GlobalNamespace/ColorSchemesSettings.hpp"
#include "GlobalNamespace/ColorScheme.hpp"
#include "GlobalNamespace/NoteData.hpp"
#include "GlobalNamespace/AudioTimeSyncController.hpp"

#include "unordered_map"
#include <iomanip>
#include <sstream>

using namespace GlobalNamespace;

static ModInfo modInfo;
DEFINE_CONFIG(ModConfig);

static BSDUI::LevelStats* levelStatsView = nullptr;
static BSDUI::ScoreGraph* scoreGraphView = nullptr;
static std::unordered_map<SaberSwingRatingCounter*, NoteCutInfo> swingMap;
int realRightSaberDistance;

Tracker tracker;
std::vector<std::pair<float, float>> percents;

static const UnityEngine::Color expertPlus = UnityEngine::Color(0.5607843399, 0.2823529541, 0.8588235378, 1),
expert = UnityEngine::Color(0.7490196228, 0.7490196228, 0.2588235438, 1),
hard = UnityEngine::Color(1, 0.6470588446, 0, 1),
normal = UnityEngine::Color(0.3490196168, 0.6901960969, 0.9568627477, 1),
easy = UnityEngine::Color(0.2352941185, 0.7019608021, 0.4431372583, 1),
gold = UnityEngine::Color(0.9294117689, 0.9294117689, 0.4039215744, 1);

Logger& getLogger() {
    static Logger* logger = new Logger(modInfo);
    return *logger;
}

int calculateMaxScore(int blockCount) {
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

Il2CppString* Round(float num, std::string_view extra = "") {
    int precision = getModConfig().Decimals.GetValue();

    std::stringstream out;
    out << std::fixed << std::setprecision(precision) << num;
    out << extra;

    std::string s = out.str();
    if(getModConfig().Commas.GetValue()) {
        int i = s.find('.');
        if(i > 0)
            s[i] = ',';
    }
    return il2cpp_utils::createcsstr(s);
}

static UnityEngine::Color c32toColor(UnityEngine::Color32 color) {
    return UnityEngine::Color(color.r / 255.0, color.g / 255.0, color.b / 255.0, color.a / 255.0);
}
static UnityEngine::Color32 colorToc32(UnityEngine::Color color) {
    return UnityEngine::Color32(color.r * 255, color.g * 255, color.b * 255, color.a * 255);
}
static UnityEngine::Vector2 v3tov2(UnityEngine::Vector3 v) {
    return UnityEngine::Vector2(v.x, v.y);
}

// Hooks
MAKE_HOOK_MATCH(ProcessResults, &SoloFreePlayFlowCoordinator::ProcessLevelCompletionResultsAfterLevelDidFinish, void, SoloFreePlayFlowCoordinator* self, LevelCompletionResults* levelCompletionResults, IDifficultyBeatmap* difficultyBeatmap, GameplayModifiers* gameplayModifiers, bool practice) {
    ProcessResults(self, levelCompletionResults, difficultyBeatmap, gameplayModifiers, practice);

    if(!levelStatsView)
        levelStatsView = QuestUI::BeatSaberUI::CreateViewController<BSDUI::LevelStats*>();
    if(!scoreGraphView)
        scoreGraphView = QuestUI::BeatSaberUI::CreateViewController<BSDUI::ScoreGraph*>();
    
    // exit through pause or something
    if(levelCompletionResults->levelEndStateType == LevelCompletionResults::LevelEndStateType::None)
        return;

    if(!getModConfig().ShowPass.GetValue() && levelCompletionResults->levelEndStateType == LevelCompletionResults::LevelEndStateType::Cleared)
        return;
    
    if(!getModConfig().ShowFail.GetValue() && levelCompletionResults->levelEndStateType == LevelCompletionResults::LevelEndStateType::Failed)
        return;

    self->SetLeftScreenViewController(levelStatsView, HMUI::ViewController::AnimationType().None);
    
    if(getModConfig().ShowGraph.GetValue())
        self->SetRightScreenViewController(scoreGraphView, HMUI::ViewController::AnimationType().None);

    #pragma region saberpanel
    // get colors
    auto playerData = QuestUI::ArrayUtil::First(UnityEngine::Resources::FindObjectsOfTypeAll<PlayerDataModel*>())->playerData;
    auto colors = playerData->colorSchemesSettings->GetSelectedColorScheme();

    levelStatsView->setColors(colors->saberAColor, colors->saberBColor);
        
    getLogger().info("Setting results to UI");

    IPreviewBeatmapLevel* levelData = reinterpret_cast<IPreviewBeatmapLevel*>(difficultyBeatmap->get_level());

    getLogger().info("Setting song info");

    auto cover = levelData->GetCoverImageAsync(System::Threading::CancellationToken::get_None());

    levelStatsView->songCover->set_sprite(cover->get_Result());
    levelStatsView->songName->set_text(levelData->get_songName());
    levelStatsView->songAuthor->set_text(levelData->get_songAuthorName());
    levelStatsView->songMapper->set_text(levelData->get_levelAuthorName());
    std::string diffName;
    UnityEngine::Color color;
    switch(difficultyBeatmap->get_difficulty()) {
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
    levelStatsView->songDifficulty->set_text(il2cpp_utils::createcsstr(diffName));
    levelStatsView->songDifficulty->set_color(color);

    // get various stats
    int totalNotes = levelCompletionResults->goodCutsCount + levelCompletionResults->badCutsCount + levelCompletionResults->missedCount;
    int maxScore = calculateMaxScore(totalNotes);
    int l_notes = tracker.l_notes;
    if(l_notes < 1)
        l_notes = 1;
    int r_notes = tracker.r_notes;
    if(r_notes < 1)
        r_notes = 1;
    
    getLogger().info("Setting level stats");

    levelStatsView->rank->set_text(RankModel::GetRankName(levelCompletionResults->rank));
    levelStatsView->percent->set_text(Round(levelCompletionResults->rawScore * 100.0 / maxScore, "%"));
    if(levelCompletionResults->fullCombo) {
        levelStatsView->combo->set_text(il2cpp_utils::createcsstr("FC"));
        levelStatsView->combo->set_color(gold);
        levelStatsView->top_line->set_color(gold);
        levelStatsView->bottom_line->set_color(gold);
    } else {
        levelStatsView->combo->set_text(il2cpp_utils::createcsstr(std::to_string(levelCompletionResults->maxCombo)));
        levelStatsView->combo->set_color(UnityEngine::Color::get_white());
        levelStatsView->top_line->set_color(UnityEngine::Color::get_white());
        levelStatsView->bottom_line->set_color(UnityEngine::Color::get_white());
    }
    levelStatsView->misses->set_text(il2cpp_utils::createcsstr(std::to_string(totalNotes - levelCompletionResults->goodCutsCount)));
    levelStatsView->pauses->set_text(il2cpp_utils::createcsstr(std::to_string(tracker.pauses)));
    if(l_notes+r_notes == 0) {
        tracker = {0};
    }

    getLogger().info("Setting left saber stats");

    levelStatsView->l_cut->set_text(Round(tracker.l_cut / l_notes));
    levelStatsView->l_beforeCut->set_text(Round(tracker.l_beforeCut / l_notes));
    levelStatsView->l_afterCut->set_text(Round(tracker.l_afterCut / l_notes));
    levelStatsView->l_accuracy->set_text(Round(tracker.l_accuracy / l_notes));
    levelStatsView->l_distance->set_text(Round(levelCompletionResults->leftSaberMovementDistance, " m"));
    levelStatsView->l_speed->set_text(Round(tracker.l_speed / l_notes, " Km/h"));
    levelStatsView->l_preSwing->set_text(Round(tracker.l_preSwing*100 / l_notes, "%"));
    levelStatsView->l_postSwing->set_text(Round(tracker.l_postSwing*100 / l_notes, "%"));
    levelStatsView->l_circle->set_fillAmount((tracker.l_cut / l_notes) / 115);

    getLogger().info("Setting right saber stats");
    
    levelStatsView->r_cut->set_text(Round(tracker.r_cut / r_notes));
    levelStatsView->r_beforeCut->set_text(Round(tracker.r_beforeCut / r_notes));
    levelStatsView->r_afterCut->set_text(Round(tracker.r_afterCut / r_notes));
    levelStatsView->r_accuracy->set_text(Round(tracker.r_accuracy / r_notes));
    levelStatsView->r_distance->set_text(Round(realRightSaberDistance, " m"));
    levelStatsView->r_speed->set_text(Round(tracker.r_speed / r_notes, " Km/h"));
    levelStatsView->r_preSwing->set_text(Round(tracker.r_preSwing*100 / r_notes, "%"));
    levelStatsView->r_postSwing->set_text(Round(tracker.r_postSwing*100 / r_notes, "%"));
    levelStatsView->r_circle->set_fillAmount((tracker.r_cut / r_notes) / 115);
    #pragma endregion

    #pragma region graph

    #pragma endregion
}

MAKE_HOOK_MATCH(SongStart, &AudioTimeSyncController::Start, void, AudioTimeSyncController* self) {
    SongStart(self);

    tracker.song_time = self->get_songLength();
    // getLogger().info("Song length: %.2f", tracker.song_time);
}

MAKE_HOOK_MATCH(LevelPlay, &SinglePlayerLevelSelectionFlowCoordinator::StartLevel, void, SinglePlayerLevelSelectionFlowCoordinator* self, System::Action* beforeSceneSwitchCallback, bool practice) {
    LevelPlay(self, beforeSceneSwitchCallback, practice);
    // getLogger().info("Level started");
    tracker = {0};
    tracker.min_pct = 1;
    // idk, a few less resizes I guess
    percents.clear();
    percents.reserve(self->get_selectedDifficultyBeatmap()->get_beatmapData()->cuttableNotesCount);
    swingMap.clear();
}

MAKE_HOOK_MATCH(LevelPause, &PauseMenuManager::ShowMenu, void, PauseMenuManager* self) {
    LevelPause(self);
    tracker.pauses++;
}

MAKE_HOOK_MATCH(NoteCut, &ScoreController::HandleNoteWasCut, void, ScoreController* self, NoteController* noteController, ByRef<NoteCutInfo> noteCutInfo) {
    NoteCut(self, noteController, noteCutInfo);
    
    auto cutInfo = noteCutInfo.heldRef;

    // track bad cuts
    if(!cutInfo.get_allIsOK() && noteController->noteData->colorType != -1) {
        float time = self->audioTimeSyncController->get_songTime();

        tracker.notes++;
        float maxScore = calculateMaxScore(tracker.notes);
        float pct = self->baseRawScore / maxScore;
        
        if(pct < tracker.min_pct)
            tracker.min_pct = pct;
        if(pct > tracker.max_pct)
            tracker.max_pct = pct;
        getLogger().info("Score:%i, notes: %i, percent: %.2f, time: %.2f", self->baseRawScore, tracker.notes, pct, time);

        percents.push_back(std::make_pair(time, pct));

        return;
    }

    // add cutInfo to swing map
    SaberSwingRatingCounter* swing = reinterpret_cast<SaberSwingRatingCounter*>(cutInfo.swingRatingCounter);
    if(!swing)
        return;
    
    swingMap[swing] = cutInfo;
    
    // track before cut ratings
    float beforeCutRating = swing->beforeCutRating;
    if(cutInfo.saberType == SaberType::SaberA)
        tracker.l_preSwing += beforeCutRating;
    else
        tracker.r_preSwing += beforeCutRating;
}

MAKE_HOOK_MATCH(NoteMiss, &ScoreController::HandleNoteWasMissed, void, ScoreController* self, NoteController* noteController) {
    NoteMiss(self, noteController);

    if(noteController->noteData->colorType == -1)
        return;
        
    float time = self->audioTimeSyncController->get_songTime();

    tracker.notes++;
    float maxScore = calculateMaxScore(tracker.notes);
    float pct = self->baseRawScore / maxScore;

    if(pct < tracker.min_pct)
        tracker.min_pct = pct;
    if(pct > tracker.max_pct)
        tracker.max_pct = pct;
    getLogger().info("Score:%i, notes: %i, percent: %.2f, time: %.2f", self->baseRawScore, tracker.notes, pct, time);

    percents.push_back(std::make_pair(time, pct));
}

MAKE_HOOK_MATCH(AddScore, &ScoreController::HandleCutScoreBufferDidFinish, void, ScoreController* self, CutScoreBuffer* cutScoreBuffer) {    
    int cutScore = cutScoreBuffer->get_scoreWithMultiplier() / cutScoreBuffer->get_multiplier();
    
    AddScore(self, cutScoreBuffer);
    
    // float time = reinterpret_cast<SaberSwingRatingCounter*>(cutScoreBuffer->saberSwingRatingCounter)->cutTime;
    float time = self->audioTimeSyncController->get_songTime();

    tracker.notes++;
    float maxScore = calculateMaxScore(tracker.notes);
    float pct = self->baseRawScore / maxScore;

    if(pct < tracker.min_pct)
        tracker.min_pct = pct;
    if(pct > tracker.max_pct)
        tracker.max_pct = pct;
    getLogger().info("Score:%i, notes: %i, percent: %.2f, time: %.2f", self->baseRawScore, tracker.notes, pct, time);

    percents.push_back(std::make_pair(time, pct));
}

MAKE_HOOK_MATCH(AngleData, &SaberSwingRatingCounter::ProcessNewData, void, SaberSwingRatingCounter* self, BladeMovementDataElement newData, BladeMovementDataElement prevData, bool prevDataAreValid) {
    bool alreadyCut = self->notePlaneWasCut;
    
    AngleData(self, newData, prevData, prevDataAreValid);

    if(!self->notePlaneWasCut)
        return;

    if(swingMap.count(self) == 0)
        return;
    
    auto cutInfo = swingMap[self];
    bool leftSaber = cutInfo.saberType == SaberType::SaberA;

    float num = UnityEngine::Vector3::Angle(newData.segmentNormal, self->cutPlaneNormal);
    
    if(!alreadyCut) {
        float postAngle = UnityEngine::Vector3::Angle(self->cutTopPos - self->cutBottomPos, self->afterCutTopPos - self->afterCutBottomPos);
        if(leftSaber)
            tracker.l_postSwing += SaberSwingRating::AfterCutStepRating(postAngle, 0);
        else
            tracker.r_postSwing += SaberSwingRating::AfterCutStepRating(postAngle, 0);
    } else {
        if(leftSaber)
            tracker.l_postSwing += SaberSwingRating::AfterCutStepRating(newData.segmentAngle, num);
        else
            tracker.r_postSwing += SaberSwingRating::AfterCutStepRating(newData.segmentAngle, num);
    }

    // run only when finishing
    if(self->finished) {
        int before, after, accuracy;
        ScoreModel::RawScoreWithoutMultiplier(reinterpret_cast<ISaberSwingRatingCounter*>(self), cutInfo.cutDistanceToCenter, byref(before), byref(after), byref(accuracy));

        if(leftSaber) {
            // getLogger().info("Left swing finishing");
            tracker.l_notes++;
            tracker.l_cut += before + after + accuracy;
            tracker.l_beforeCut += before;
            tracker.l_afterCut += after;
            tracker.l_accuracy += accuracy;
            tracker.l_speed += cutInfo.saberSpeed;
        } else {
            // getLogger().info("Right swing finishing");
            tracker.r_notes++;
            tracker.r_cut += before + after + accuracy;
            tracker.r_beforeCut += before;
            tracker.r_afterCut += after;
            tracker.r_accuracy += accuracy;
            tracker.r_speed += cutInfo.saberSpeed;
        }
        swingMap.erase(self);
    }
    // correct for change in ComputeSwingRating
    if(self->beforeCutRating > 1)
        self->beforeCutRating = 1;
}

// honestly why is this so complicated
// override #1
MAKE_HOOK_MATCH(PreSwingCalc, static_cast<float (SaberMovementData::*)(bool, float)>(&SaberMovementData::ComputeSwingRating), float, SaberMovementData* self, bool overrideSegmenAngle, float overrideValue) {
    if (self->validCount < 2)
        return 0;

    int num = self->data->Length();
    int num2 = self->nextAddIndex - 1;
    if (num2 < 0)
        num2 += num;

    float time = self->data->get(num2).time;
    float num3 = time;
    float num4 = 0;
    UnityEngine::Vector3 segmentNormal = self->data->get(num2).segmentNormal;
    float angleDiff = (overrideSegmenAngle ? overrideValue : self->data->get(num2).segmentAngle);
    int num5 = 2;

    num4 += SaberSwingRating::BeforeCutStepRating(angleDiff, 0);

    while (time - num3 < 0.4 && num5 < self->validCount) {
        num2--;
        if (num2 < 0)
            num2 += num;
        UnityEngine::Vector3 segmentNormal2 = self->data->get(num2).segmentNormal;
        angleDiff = self->data->get(num2).segmentAngle;
        float num6 = UnityEngine::Vector3::Angle(segmentNormal2, segmentNormal);
        if (num6 > 90)
            break;
        num4 += SaberSwingRating::BeforeCutStepRating(angleDiff, num6);
        num3 = self->data->get(num2).time;
        num5++;
    }
    return num4;
}

// fix your game
MAKE_HOOK_MATCH(MakeCompletionResults, &LevelCompletionResultsHelper::Create, LevelCompletionResults*, int levelNotesCount, ::Array<BeatmapObjectExecutionRating*>* beatmapObjectExecutionRatings, GameplayModifiers* gameplayModifiers, GameplayModifiersModelSO* gameplayModifiersModel, int rawScore, int modifiedScore, int maxCombo, ::Array<float>* saberActivityValues, float leftSaberMovementDistance, float rightSaberMovementDistance, ::Array<float>* handActivityValues, float leftHandMovementDistance, float rightHandMovementDistance, float songDuration, LevelCompletionResults::LevelEndStateType levelEndStateType, LevelCompletionResults::LevelEndAction levelEndAction, float energy, float songTime) {
    realRightSaberDistance = rightSaberMovementDistance;
    return MakeCompletionResults(levelNotesCount, beatmapObjectExecutionRatings, gameplayModifiers, gameplayModifiersModel, rawScore, modifiedScore, maxCombo, saberActivityValues, leftSaberMovementDistance, rightSaberMovementDistance, handActivityValues, leftHandMovementDistance, rightHandMovementDistance, songDuration, levelEndStateType, levelEndAction, energy, songTime);
}

// mfers can't even fill an image on a curved surface
// override #2
static void customAddQuad(UnityEngine::UI::VertexHelper* vertexHelper, ::Array<UnityEngine::Vector3>* quadPositions, UnityEngine::Color32 color, ::Array<UnityEngine::Vector3>* quadUVs, float curvedUIRadius) {
    int currentVertCount = vertexHelper->get_currentVertCount();
    UnityEngine::Vector2 uv = UnityEngine::Vector2(curvedUIRadius, 0);
    for (int i = 0; i < 4; i++) {
        vertexHelper->AddVert(quadPositions->get(i), color, v3tov2(quadUVs->get(i)), HMUI::ImageView::_get_kVec2Zero(), uv, HMUI::ImageView::_get_kVec2Zero(), HMUI::ImageView::_get_kVec3Zero(), HMUI::ImageView::_get_kVec4Zero());
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
    self->_get_s_Xy()->get(0) = UnityEngine::Vector3(drawingDimensions.x, drawingDimensions.y, 0);
    self->_get_s_Xy()->get(1) = UnityEngine::Vector3(drawingDimensions.x, drawingDimensions.w, 0);
    self->_get_s_Xy()->get(2) = UnityEngine::Vector3(drawingDimensions.z, drawingDimensions.w, 0);
    self->_get_s_Xy()->get(3) = UnityEngine::Vector3(drawingDimensions.z, drawingDimensions.y, 0);
    self->_get_s_Uv()->get(0) = UnityEngine::Vector3(num, num2, 0);
    self->_get_s_Uv()->get(1) = UnityEngine::Vector3(num, num4, 0);
    self->_get_s_Uv()->get(2) = UnityEngine::Vector3(num3, num4, 0);
    self->_get_s_Uv()->get(3) = UnityEngine::Vector3(num3, num2, 0);
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
                self->_get_s_Xy()->get(0).x = UnityEngine::Mathf::Lerp(drawingDimensions.x, drawingDimensions.z, t3);
                self->_get_s_Xy()->get(1).x = self->_get_s_Xy()->get(0).x;
                self->_get_s_Xy()->get(2).x = UnityEngine::Mathf::Lerp(drawingDimensions.x, drawingDimensions.z, t4);
                self->_get_s_Xy()->get(3).x = self->_get_s_Xy()->get(2).x;
                self->_get_s_Xy()->get(0).y = UnityEngine::Mathf::Lerp(drawingDimensions.y, drawingDimensions.w, t);
                self->_get_s_Xy()->get(1).y = UnityEngine::Mathf::Lerp(drawingDimensions.y, drawingDimensions.w, t2);
                self->_get_s_Xy()->get(2).y = self->_get_s_Xy()->get(1).y;
                self->_get_s_Xy()->get(3).y = self->_get_s_Xy()->get(0).y;
                self->_get_s_Uv()->get(0).x = UnityEngine::Mathf::Lerp(num, num3, t3);
                self->_get_s_Uv()->get(1).x = self->_get_s_Uv()->get(0).x;
                self->_get_s_Uv()->get(2).x = UnityEngine::Mathf::Lerp(num, num3, t4);
                self->_get_s_Uv()->get(3).x = self->_get_s_Uv()->get(2).x;
                self->_get_s_Uv()->get(0).y = UnityEngine::Mathf::Lerp(num2, num4, t);
                self->_get_s_Uv()->get(1).y = UnityEngine::Mathf::Lerp(num2, num4, t2);
                self->_get_s_Uv()->get(2).y = self->_get_s_Uv()->get(1).y;
                self->_get_s_Uv()->get(3).y = self->_get_s_Uv()->get(0).y;
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
                self->_get_s_Xy()->get(0).x = UnityEngine::Mathf::Lerp(drawingDimensions.x, drawingDimensions.z, t5);
                self->_get_s_Xy()->get(1).x = self->_get_s_Xy()->get(0).x;
                self->_get_s_Xy()->get(2).x = UnityEngine::Mathf::Lerp(drawingDimensions.x, drawingDimensions.z, t6);
                self->_get_s_Xy()->get(3).x = self->_get_s_Xy()->get(2).x;
                self->_get_s_Xy()->get(0).y = UnityEngine::Mathf::Lerp(drawingDimensions.y, drawingDimensions.w, t7);
                self->_get_s_Xy()->get(1).y = UnityEngine::Mathf::Lerp(drawingDimensions.y, drawingDimensions.w, t8);
                self->_get_s_Xy()->get(2).y = self->_get_s_Xy()->get(1).y;
                self->_get_s_Xy()->get(3).y = self->_get_s_Xy()->get(0).y;
                self->_get_s_Uv()->get(0).x = UnityEngine::Mathf::Lerp(num, num3, t5);
                self->_get_s_Uv()->get(1).x = self->_get_s_Uv()->get(0).x;
                self->_get_s_Uv()->get(2).x = UnityEngine::Mathf::Lerp(num, num3, t6);
                self->_get_s_Uv()->get(3).x = self->_get_s_Uv()->get(2).x;
                self->_get_s_Uv()->get(0).y = UnityEngine::Mathf::Lerp(num2, num4, t7);
                self->_get_s_Uv()->get(1).y = UnityEngine::Mathf::Lerp(num2, num4, t8);
                self->_get_s_Uv()->get(2).y = self->_get_s_Uv()->get(1).y;
                self->_get_s_Uv()->get(3).y = self->_get_s_Uv()->get(0).y;
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
        HMUI::ImageView::AddQuad(toFill, v3tov2(self->_get_s_Xy()->get(0)), v3tov2(self->_get_s_Xy()->get(2)), colorToc32(self->get_color()), v3tov2(self->_get_s_Uv()->get(0)), v3tov2(self->_get_s_Uv()->get(2)), x, curvedUIRadius);
    }
}

extern "C" void setup(ModInfo& info) {
    info.id = ID;
    info.version = VERSION;
    modInfo = info;
	
    getLogger().info("Completed setup!");
}

extern "C" void load() {
    getLogger().info("Starting setup...");
    il2cpp_functions::Init();

    custom_types::Register::AutoRegister();

    // config stuff idk
    getModConfig().Init(modInfo);
    QuestUI::Init();
    QuestUI::Register::RegisterModSettingsViewController(modInfo, DidActivate);

    getLogger().info("Installing hooks...");
    LoggerContextObject logger = getLogger().WithContext("load");
    // Install hooks
    INSTALL_HOOK(logger, ProcessResults);
    INSTALL_HOOK(logger, SongStart);
    INSTALL_HOOK(logger, LevelPlay);
    INSTALL_HOOK(logger, LevelPause);
    INSTALL_HOOK(logger, NoteCut);
    INSTALL_HOOK(logger, NoteMiss);
    INSTALL_HOOK(logger, AddScore);
    INSTALL_HOOK(logger, AngleData);
    INSTALL_HOOK(logger, PreSwingCalc);
    INSTALL_HOOK(logger, MakeCompletionResults);
    INSTALL_HOOK(logger, FillImage);
    getLogger().info("Installed all hooks!");
}