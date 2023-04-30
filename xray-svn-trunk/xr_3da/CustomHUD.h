#pragma once

#include "iinputreceiver.h"

ENGINE_API extern Flags32		psHUD_Flags;
#define HUD_CROSSHAIR			(1<<0)
#define HUD_CROSSHAIR_DIST		(1<<1)
#define HUD_WEAPON				(1<<2)
#define HUD_INFO				(1<<3)
#define HUD_DRAW				(1<<4)
#define HUD_WEAPON_RT			(1<<6)
#define HUD_CROSSHAIR_DYNAMIC	(1<<7)
#define HUD_SHOW_CLOCK			(1<<10)
#define HUD_WEAPON_RT2			(1<<11)
#define HUD_DRAW_RT2			(1<<12)
#define HUD_INFO_CNAME			(1<<13)
#define HUD_NPC_COUNTER			(1<<14)
#define HUD_NPC_DETECTION		(1<<15)

class IRenderBuffer;

struct NvSahderParams
{
	bool needNVShading_;
	Fvector nvColor_;
	float nvSaturation_;
	float nvSaturationOut_;
	u8 nightVisionGoogleType_;
	float borderShadowingStop_;
	float borderShadowingOut_;
	float monocularRadius_;
	float nvGrainPower_;
	float nvGrainSize_;
	float nvGrainClrIntensity_;
	bool nvHasBrokenCamEffect_;

	NvSahderParams();
	virtual ~NvSahderParams();
};

struct HudGlassEffects
{
	bool					castHudGlassEffects_;

	float					actorHudWetness_1_; // for storing actors wetness on hud layer 1
	float					Condition;

	float					ActorHealth;
	float					ActorMaxHealth;

	HudGlassEffects();
	virtual ~HudGlassEffects();

	//Wetness on hud for rain drops count calculation in shader
	void					SetActorHudWetness1(float val)			{ actorHudWetness_1_ = val; };
	//Wetness on hud for rain drops count calculation in shader
	IC float				GetActorHudWetness1()					{ return actorHudWetness_1_; };

	//Do actualy cast visor drops and sun shafts?
	void					SetCastHudGlassEffects(bool val)		{ castHudGlassEffects_ = val; };
	//Do actualy cast visor drops and sun shafts?
	IC bool					GetCastHudGlassEffects()				{ return castHudGlassEffects_; };

	void					SetMaskCondition(float val) { Condition = val; };
	IC float				GetMaskCondition() { return Condition; };

	IC float GetActorMaxHealth() { return ActorMaxHealth; }
	IC float GetActorHealth() { return ActorHealth; }
};

class ENGINE_API IRenderVisual;
class ENGINE_API CCustomHUD :
	public DLL_Pure,
	public IEventReceiver,
	public pureScreenResolutionChanged
{
public:
    CCustomHUD();
    virtual ~CCustomHUD();

	virtual void Render_First(IRenderBuffer& render_buffer) { ; }
	virtual void Render_Last(IRenderBuffer& render_buffer) { ; }

    virtual void OnFrame() { ; }
    virtual void OnEvent(EVENT E, u64 P1, u64 P2) { ; }

    virtual void Load() { ; }
    virtual void OnDisconnected() = 0;
    virtual void OnConnected() = 0;
    virtual void RenderActiveItemUI() = 0;
    virtual bool RenderActiveItemUIQuery() = 0;
	
    virtual void RemoveLinksToCLObj(CObject* object) = 0;

	NvSahderParams	nightVisionEffect_;
	HudGlassEffects	hudGlassEffects_;

	bool showGameHudEffects_;
};

extern ENGINE_API CCustomHUD* g_hud;