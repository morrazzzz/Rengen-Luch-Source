#include "stdafx.h"
#include "stats.h"

#include "GameFont.h"
#include "IGame_Persistent.h"
#include "render.h"

#include "ConstantDebug.h"

#include "../Include/xrRender/DrawUtils.h"

#include "../Include/xrRender/UIShader.h"
#include "../Include/xrRender/UIRender.h"

extern void DisplayWithcolor(CGameFont& targetFont, LPCSTR msg, u32 returntocolor, float value, float bad_value);
extern void DrawRect(Frect const& r, u32 color);

extern BOOL show_UpdateClTimes_;
extern BOOL show_ScedulerUpdateTimes_;
extern BOOL show_MTTimes_;
extern BOOL show_MTRenderDelays_;
extern BOOL show_OnFrameTimes_;

extern Fvector colorok;
extern Fvector colorbad;
extern u32 statscolor1;
extern u32 back_color;

extern FactoryPtr<IUIShader>* back_shader;

Frect back_region2;

void CStats::InitializeMeasuringValuesUCL()
{

#ifdef MEASURE_UPDATES

	updateCL_Object_ = 0.f;
	updateCL_BaseMonster_ = 0.f;
	updateCL_CustomMonster_ = 0.f;
	updateCL_ScriptEntity_ = 0.f;
	updateCL_ScriptObject_ = 0.f;
	updateCL_AIStalker_ = 0.f;
	updateCL_AITrader_ = 0.f;
	updateCL_VariousMonsters_ = 0.f;
	updateCL_Actor_ = 0.f;
	updateCL_GameObject_ = 0.f;
	updateCL_VariousItems_ = 0.f;
	updateCL_VariousPhysics_ = 0.f;
	updateCL_HangingLamp_ = 0.f;
	updateCL_Projector_ = 0.f;
	updateCL_Car_ = 0.f;
	updateCL_CustomZone_ = 0.f;
	updateCL_Helicopter_ = 0.f;

#endif
}


void CStats::InitializeMeasuringValuesUSC()
{
#ifdef MEASURE_UPDATES

	scheduler_Object_ = 0.f;
	scheduler_VisionClient_ = 0.f;
	scheduler_PSInstance_ = 0.f;
	scheduler_AgentManager_ = 0.f;
	scheduler_BaseMonster_ = 0.f;
	scheduler_CustomMonster_ = 0.f;
	scheduler_ScriptBinder_ = 0.f;
	scheduler_ScriptEntity_ = 0.f;
	scheduler_ScriptObject_ = 0.f;
	scheduler_AIStalker_ = 0.f;
	scheduler_AITrader_ = 0.f;
	scheduler_AICrow_ = 0.f;
	scheduler_VariousMonsters_ = 0.f;
	scheduler_ScriptBinderObjectWrapper_ = 0.f;
	scheduler_ScriptParticlesCustom_ = 0.f;
	scheduler_Actor_ = 0.f;
	scheduler_Entity_ = 0.f;
	scheduler_EntityAlive_ = 0.f;
	scheduler_GameObject_ = 0.f;
	scheduler_HangingLamp_ = 0.f;
	scheduler_VariousItems_ = 0.f;
	scheduler_VariousPhysics_ = 0.f;
	scheduler_Car_ = 0.f;
	scheduler_CustomZone_ = 0.f;
	scheduler_LevelChanger_ = 0.f;
	scheduler_ScriptZone_ = 0.f;
	scheduler_VariousAnomalies_ = 0.f;
	scheduler_ParticleObject_ = 0.f;

#endif
}


void CStats::InitializeMeasuringValuesMT()
{
#ifdef MEASURE_MT

	mtPhysicsTime_ = 0.f;
	mtSoundTime_ = 0.f;
	mtNetworkTime_ = 0.f;
	mtParticlesTime_ = 0.f;

	mtHOM_ = 0.f;
	mtDetails_ = 0.f;

	mtLevelPathTime_ = 0.f;
	mtDetailPathTime_ = 0.f;
	mtMapTime_ = 0.f;
	mtObjectHandlerTime_ = 0.f;
	mtSoundPlayerTime_ = 0.f;
	mtAiVisionTime_ = 0.f;
	mtAIMiscTime_ = 0.f;
	mtBulletsTime_ = 0.f;
	mtLUA_GCTime_ = 0.f;
	mtLevelSoundsTime_ = 0.f;
	mtALifeTime_ = 0.f;
	mtIKinematicsTime_ = 0.f;
	mtLTrackTime_ = 0.f;
	mtVisStructTime_ = 0.f;
	mtMeshBones_ = 0.f;

	mtRenderDelayPrior0_ = 0.f;
	mtRenderDelayPrior1_ = 0.f;
	mtRenderDelayLights_ = 0.f;
	mtRenderDelaySun_ = 0.f;
	mtRenderDelayRain_ = 0.f;
	mtRenderDelayBones_ = 0.f;
	mtRenderDelayDetails_ = 0.f;
	mtRenderDelayHom_ = 0.f;

#endif
}


void CStats::InitializeMeasuringValuesOnFrame()
{
#ifdef MEASURE_ON_FRAME

	onframe_Render_ = 0;
	onframe_App_ = 0;
	onframe_Console_ = 0;
	onframe_Input_ = 0;
	onframe_UISeq_ = 0;
	onframe_UIDragItem_ = 0;
	onframe_MainMenu_ = 0;
	onframe_UIGameCustom_ = 0;
	onframe_GamePersistant_ = 0;
	onframe_IGamePersistent_ = 0;
	onframe_Level_ = 0;
	onframe_ILevel_ = 0;

#endif
}

void CStats::SecondBlockDrawRectAndSetFontPos()
{
	if (show_UpdateClTimes_ || show_ScedulerUpdateTimes_ || show_MTTimes_ || show_MTRenderDelays_ || show_OnFrameTimes_)
	{
		float device_width = (float)Device.dwWidth;
		float device_height = (float)Device.dwHeight;

		CGameFont& fontt = *Device.Statistic->secondBlockFont_;

		{
			float posx, posx2;
			float fontstresh_f = 150 * (device_width / 1280) - 150;
			posx = (700.0f + fontstresh_f) * (device_width / 1280.f);
			posx2 = 965.f * (device_width / 1280.f);

			back_region2.set(posx, 0.0f, posx2, device_height);

			if (!back_shader) //init shader, if not already
			{
				back_shader = xr_new <FactoryPtr<IUIShader> >();
				(*back_shader)->create("hud\\default", "ui\\ui_console");
			}

			UIRender->SetShader(**back_shader);
			UIRender->StartPrimitive(6, IUIRender::ptTriList, IUIRender::pttTL);
			DrawRect(back_region2, back_color);
			UIRender->FlushPrimitive();
		}

		{
			fontt.SetColor(statscolor1);
			fontt.SetAligment(CGameFont::alRight);

			float posx, posy;
			if (device_width / device_height > 1.5f)
			{
				posx = 960.f * (device_width / 1280.f);
				posy = 10.f * (device_height / 768.f);
				fontt.SetHeightI(0.013);
			}
			else
			{
				posx = 800.f * (device_width / 1024.f);
				posy = 15.f * (device_height / 768.f);
				fontt.SetHeightI(0.013);
			}

			fontt.OutSet(posx, posy);
		}
	}
}


void CStats::ShowUpdateCLTimes()
{
#ifdef MEASURE_UPDATES

	if (show_UpdateClTimes_)
	{
		CGameFont& fontt = *Device.Statistic->secondBlockFont_;

		fontt.OutNext("Update CLs");
		fontt.OutSkip();

		DisplayWithcolor(fontt, "CObject", statscolor1, updateCL_Object_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "BaseMonster", statscolor1, updateCL_BaseMonster_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "CustomMonster", statscolor1, updateCL_CustomMonster_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "ScriptEntity", statscolor1, updateCL_ScriptEntity_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "ScriptObject", statscolor1, updateCL_ScriptObject_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "AIStalker", statscolor1, updateCL_AIStalker_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "AITrader", statscolor1, updateCL_AITrader_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "VariousMonsters", statscolor1, updateCL_VariousMonsters_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "Actor", statscolor1, updateCL_Actor_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "GameObject", statscolor1, updateCL_GameObject_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "VariousItems", statscolor1, updateCL_VariousItems_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "VariousPhysics", statscolor1, updateCL_VariousPhysics_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "HangingLamp", statscolor1, updateCL_HangingLamp_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "Projector", statscolor1, updateCL_Projector_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "Car", statscolor1, updateCL_Car_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "CustomZone", statscolor1, updateCL_CustomZone_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "Helicopter", statscolor1, updateCL_Helicopter_ * 1000.f, 1.f);

		InitializeMeasuringValuesUCL();
	}
#else
	if (show_UpdateClTimes_)
	{
		fpsFont.OutNext("Measuring UpdateCL is not available in final release bins");
	}
#endif
}


void CStats::ShowScUpdateTimes()
{
#ifdef MEASURE_UPDATES

	if (show_ScedulerUpdateTimes_)
	{
		CGameFont& fontt = *secondBlockFont_;

		fontt.OutSkip();
		fontt.OutSkip();
		fontt.OutNext("Update Scheduler");
		fontt.OutSkip();

		DisplayWithcolor(fontt, "CObject", statscolor1, scheduler_Object_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "VisionClient", statscolor1, scheduler_VisionClient_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "PSInstance", statscolor1, scheduler_PSInstance_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "AgentManager", statscolor1, scheduler_AgentManager_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "BaseMonster", statscolor1, scheduler_BaseMonster_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "CustomMonster", statscolor1, scheduler_CustomMonster_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "ScriptBinder", statscolor1, scheduler_ScriptBinder_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "ScriptEntity", statscolor1, scheduler_ScriptEntity_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "ScriptObject", statscolor1, scheduler_ScriptObject_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "AIStalker", statscolor1, scheduler_AIStalker_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "AITrader", statscolor1, scheduler_AITrader_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "AICrow", statscolor1, scheduler_AICrow_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "VariousMonsters", statscolor1, scheduler_VariousMonsters_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "ScriptBinderObjectWrapper", statscolor1, scheduler_ScriptBinderObjectWrapper_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "ScriptParticlesCustom", statscolor1, scheduler_ScriptParticlesCustom_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "Entity", statscolor1, scheduler_Entity_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "EntityAlive", statscolor1, scheduler_EntityAlive_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "GameObject", statscolor1, scheduler_GameObject_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "HangingLamp", statscolor1, scheduler_HangingLamp_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "VariousItems", statscolor1, scheduler_VariousItems_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "VariousPhysics", statscolor1, scheduler_VariousPhysics_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "Car", statscolor1, scheduler_Car_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "CustomZone", statscolor1, scheduler_CustomZone_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "LevelChanger", statscolor1, scheduler_LevelChanger_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "ScriptZone", statscolor1, scheduler_ScriptZone_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "VariousAnomalies", statscolor1, scheduler_VariousAnomalies_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "ParticleObject", statscolor1, scheduler_ParticleObject_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "Actor", statscolor1, scheduler_Actor_ * 1000.f, 1.f);

		InitializeMeasuringValuesUSC();
	}
#else
	if (show_ScedulerUpdateTimes_)
	{
		fpsFont.OutNext("Measuring Scheduler Updates is not available in final release bins");
	}
#endif
}


void CStats::ShowMultithredingTimes()
{
#ifdef MEASURE_MT

	if (show_MTTimes_)
	{
		CGameFont& fontt = *secondBlockFont_;

		fontt.OutSkip();
		fontt.OutSkip();
		fontt.OutNext("MT Workload Times(Prev. Frame)");
		fontt.OutSkip();

		DisplayWithcolor(fontt, "MTPhysics", statscolor1, mtPhysicsTimeCopy_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "MTSound", statscolor1, mtSoundTimeCopy_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "MTNetwork", statscolor1, mtNetworkTimeCopy_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "MTParticles", statscolor1, mtParticlesTimeCopy_ * 1000.f, 5.f);

		DisplayWithcolor(fontt, "MTHOM", statscolor1, mtHOMCopy_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "MTDetails", statscolor1, mtDetailsCopy_ * 1000.f, 1.f);

		DisplayWithcolor(fontt, "MTLevelPath", statscolor1, mtLevelPathTimeCopy_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "MTDetailPath", statscolor1, mtDetailPathTimeCopy_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "MTMapTime", statscolor1, mtMapTimeCopy_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "MTObjectHandler", statscolor1, mtObjectHandlerTimeCopy_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "MTSoundPalyer", statscolor1, mtSoundPlayerTimeCopy_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "MTAIVision", statscolor1, mtAiVisionTimeCopy_ * 1000.f, 5.f);
		DisplayWithcolor(fontt, "MTAIMisc", statscolor1, mtAIMiscTimeCopy_ * 1000.f, 5.f);
		DisplayWithcolor(fontt, "MTBullets", statscolor1, mtBulletsTimeCopy_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "MTLua_GC", statscolor1, mtLUA_GCTimeCopy_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "MTLevelSounds", statscolor1, mtLevelSoundsTimeCopy_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "MTAlifeTime", statscolor1, mtALifeTimeCopy_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "MTIKinematics", statscolor1, mtIKinematicsTimeCopy_ * 1000.f, 4.f);
		DisplayWithcolor(fontt, "MTLightTrack", statscolor1, mtLTrackTimeCopy_ * 1000.f, 1.f);
		DisplayWithcolor(fontt, "MTVisStructure", statscolor1, mtVisStructTimeCopy_ * 1000.f, 4.f);
		DisplayWithcolor(fontt, "MTMeshBonesCalc", statscolor1, mtMeshBonesCopy_ * 1000.f, 4.f);
	}
#else
	if (show_MTTimes_)
	{
		fpsFont.OutNext("Measuring Multithreading performance is not available in final release bins");
	}

#endif
}

void CStats::ShowMTRenderDelays()
{
#ifdef MEASURE_MT
	if (show_MTRenderDelays_)
	{
		CGameFont& fontt = *secondBlockFont_;

		fontt.OutSkip();
		fontt.OutSkip();
		fontt.OutNext("MT Render Delays(Prev. Frame)");
		fontt.OutSkip();

		DisplayWithcolor(fontt, "Prior0 VisStruct", statscolor1, mtRenderDelayPrior0Copy_, 0.1f);
		DisplayWithcolor(fontt, "Prior1 VisStruct", statscolor1, mtRenderDelayPrior1Copy_, 0.1f);
		DisplayWithcolor(fontt, "Lights VisStruct", statscolor1, mtRenderDelayLightsCopy_, 0.1f);
		DisplayWithcolor(fontt, "Sun VisStruct", statscolor1, mtRenderDelaySunCopy_, 0.1f);
		DisplayWithcolor(fontt, "Rain VisStruct", statscolor1, mtRenderDelayRainCopy_, 0.1f);
		DisplayWithcolor(fontt, "Bones Calc", statscolor1, mtRenderDelayBonesCopy_, 0.1f);
		DisplayWithcolor(fontt, "Details Preparer", statscolor1, mtRenderDelayDetailsCopy_, 0.1f);
		DisplayWithcolor(fontt, "Hom Preparer", statscolor1, mtRenderDelayHomCopy_, 0.1f);

	}

#else
	if (show_MTRenderDelays_)
	{
		fpsFont.OutNext("Measuring Multithreading Render delays is not available in final release bins");
	}
#endif
}


void CStats::ShowOnFrameTimes()
{
#ifdef MEASURE_ON_FRAME

	if (show_OnFrameTimes_)
	{
		CGameFont& fontt = *secondBlockFont_;

		fontt.OutSkip();

		fontt.OutSkip();
		fontt.OutNext("On Frame Times");
		fontt.OutSkip();

		DisplayWithcolor(fontt, "Render", statscolor1, onframe_Render_, 1.f);
		DisplayWithcolor(fontt, "GamePersistent", statscolor1, onframe_GamePersistant_, 1.f);
		DisplayWithcolor(fontt, "I_GamePersistent", statscolor1, onframe_IGamePersistent_, 1.f);
		DisplayWithcolor(fontt, "Level", statscolor1, onframe_Level_, 1.f);
		DisplayWithcolor(fontt, "I_Level", statscolor1, onframe_ILevel_, 1.f);
		DisplayWithcolor(fontt, "Application", statscolor1, onframe_App_, 1.f);
		DisplayWithcolor(fontt, "Console", statscolor1, onframe_Console_, 1.f);
		DisplayWithcolor(fontt, "Input", statscolor1, onframe_Input_, 1.f);
		DisplayWithcolor(fontt, "UISequencer", statscolor1, onframe_UISeq_, 1.f);
		DisplayWithcolor(fontt, "UIDragItem", statscolor1, onframe_UIDragItem_, 1.f);
		DisplayWithcolor(fontt, "UiGameCustom", statscolor1, onframe_UIGameCustom_, 1.f);
		DisplayWithcolor(fontt, "MainMenu", statscolor1, onframe_MainMenu_, 1.f);

		InitializeMeasuringValuesOnFrame();
	}
#else
	if (show_OnFrameTimes_)
	{
		fpsFont.OutNext("Measuring On Frame is not available in final release bins");
	}
#endif
}

void CStats::RenderSecondBlock()
{
	if (show_UpdateClTimes_ || show_ScedulerUpdateTimes_ || show_MTTimes_ || show_MTRenderDelays_ || show_OnFrameTimes_)
	{
		secondBlockFont_->OnRender();
	}
}

void CStats::GatherMeasurments()
{
#ifdef MEASURE_MT

	mtPhysicsTimeCopy_ = mtPhysicsTime_;
	mtSoundTimeCopy_ = mtSoundTime_;
	mtNetworkTimeCopy_ = mtNetworkTime_;
	mtParticlesTimeCopy_ = mtParticlesTime_;

	mtHOMCopy_ = mtHOM_;
	mtDetailsCopy_ = mtDetails_;

	mtLevelPathTimeCopy_ = mtLevelPathTime_;
	mtDetailPathTimeCopy_ = mtDetailPathTime_;
	mtMapTimeCopy_ = mtMapTime_;
	mtObjectHandlerTimeCopy_ = mtObjectHandlerTime_;
	mtSoundPlayerTimeCopy_ = mtSoundPlayerTime_;
	mtAiVisionTimeCopy_ = mtAiVisionTime_;
	mtAIMiscTimeCopy_ = mtAIMiscTime_;
	mtBulletsTimeCopy_ = mtBulletsTime_;
	mtLUA_GCTimeCopy_ = mtLUA_GCTime_;
	mtLevelSoundsTimeCopy_ = mtLevelSoundsTime_;
	mtALifeTimeCopy_ = mtALifeTime_;
	mtIKinematicsTimeCopy_ = mtIKinematicsTime_;
	mtLTrackTimeCopy_ = mtLTrackTime_;
	mtVisStructTimeCopy_ = mtVisStructTime_;
	mtMeshBonesCopy_ = mtMeshBones_;

	mtRenderDelayPrior0Copy_ = mtRenderDelayPrior0_;
	mtRenderDelayPrior1Copy_ = mtRenderDelayPrior1_;
	mtRenderDelayLightsCopy_ = mtRenderDelayLights_;
	mtRenderDelaySunCopy_ = mtRenderDelaySun_;
	mtRenderDelayRainCopy_ = mtRenderDelayRain_;
	mtRenderDelayBonesCopy_ = mtRenderDelayBones_;
	mtRenderDelayDetailsCopy_ = mtRenderDelayDetails_;
	mtRenderDelayHomCopy_ = mtRenderDelayHom_;

	InitializeMeasuringValuesMT();

#endif
}