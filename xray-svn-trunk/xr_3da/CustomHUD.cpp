#include "stdafx.h"
#include "CustomHUD.h"

Flags32 psHUD_Flags = {HUD_WEAPON_RT|HUD_WEAPON_RT2|HUD_CROSSHAIR_DYNAMIC|HUD_DRAW_RT2|HUD_SHOW_CLOCK};

ENGINE_API CCustomHUD* g_hud = NULL;

NvSahderParams::NvSahderParams()
{
	needNVShading_ = false;
	nvColor_ = { 0.0, 0.8, 0.2 };
	nvSaturation_ = 8;
	nvSaturationOut_ = 6;
	nightVisionGoogleType_ = 2;
	borderShadowingStop_ = -0.6;
	borderShadowingOut_ = 0.5;
	monocularRadius_ = 256;
	nvGrainPower_ = 0.2;
	nvGrainSize_ = 4;
	nvGrainClrIntensity_ = 0.25;
	nvHasBrokenCamEffect_ = false;
}

NvSahderParams::~NvSahderParams()
{
}

HudGlassEffects::HudGlassEffects()
{
	castHudGlassEffects_ = false;

	actorHudWetness_1_ = 0.f;
	ActorHealth = 0.f;
	ActorMaxHealth = 0.f;
}

HudGlassEffects::~HudGlassEffects()
{
}

CCustomHUD::CCustomHUD()
{
	g_hud = this;
	Device.seqResolutionChanged.Add(this);

	showGameHudEffects_ = true;
}

CCustomHUD::~CCustomHUD()
{
	g_hud = NULL;
	Device.seqResolutionChanged.Remove(this);
}

