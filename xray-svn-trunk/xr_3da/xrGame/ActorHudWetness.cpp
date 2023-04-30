#include "stdafx.h"
#include "Actor.h"
#include "../CameraBase.h"
#include "Inventory.h"
#include "CustomOutfit.h"
#include "gamepersistent.h"
#include "level.h"
#include "UIGameCustom.h"

#include "actorcondition.h"

#include "actor.h"

#include "inventory.h"

#include "Weapon.h"

#include "../CustomHUD.h"

#define REGULAR_RAIN_DENS 1.7f;
#define REGULAR_HEMI_AMOUNT 0.05f;
#define REGULAR_LOOK_DIR_Y 1.1f;

u8 mainLayer_ = 1;

void CActor::ComputeHudWetness()
{
	COutfitBase* outfit_with_visor = nullptr;

	// outfit with helmet and regular helmets are exlcluding each other, so no need to do extra checks for priority
	if (inventory().ItemFromSlot(OUTFIT_SLOT))
	{
		COutfitBase* outf = smart_cast<COutfitBase*>(inventory().ItemFromSlot(OUTFIT_SLOT));

		if (outf && outf->hasVisorEffects_)
			outfit_with_visor = outf;
	}

	if (inventory().ItemFromSlot(HELMET_SLOT))
	{
		COutfitBase* helm = smart_cast<COutfitBase*>(inventory().ItemFromSlot(HELMET_SLOT));

		if (helm && helm->hasVisorEffects_)
			outfit_with_visor = helm;
	}

	if (outfit_with_visor)
	{
		g_hud->hudGlassEffects_.SetCastHudGlassEffects(true);
		// Calc if actor hud has rain drops on it

		CCameraBase* C = cameras[eacFirstEye];

		float actor_hemi_factor = ROS()->get_luminocity_hemi();

		// check if actro is under open skyes and there is rain
		bool actor_under_rain = g_pGamePersistent->Environment().CurrentEnv->rain_density > 0.01 && actor_hemi_factor > 0.005f;
		if (actor_under_rain) // Increase
		{
			float rain_denc_amount_f = g_pGamePersistent->Environment().CurrentEnv->rain_density / REGULAR_RAIN_DENS;
			float hemi_amount_f = actor_hemi_factor / REGULAR_HEMI_AMOUNT;
			float actor_looks_up_f = (1.f + C->Direction().y) / REGULAR_LOOK_DIR_Y;

			actor_looks_up_f = actor_looks_up_f * actor_looks_up_f * actor_looks_up_f;

			clamp(actor_looks_up_f, 0.01f, 5.f);

			float add = wetnessAccmBase_ * hemi_amount_f * rain_denc_amount_f * actor_looks_up_f * TimeDelta();

				// increase layer 1
				if (outfit_with_visor->visorWetness_1_ <= maxHudWetness_)
				{
					outfit_with_visor->visorWetness_1_ += add;
					g_hud->hudGlassEffects_.SetActorHudWetness1(outfit_with_visor->visorWetness_1_);
				}

		}
		else if (outfit_with_visor->visorWetness_1_ > 0) // Decrease, if not under rain
		{
			if (outfit_with_visor->visorWetness_1_ > 0)
				outfit_with_visor->visorWetness_1_ -= wetnessDecreaseF_ * TimeDelta();

			g_hud->hudGlassEffects_.SetActorHudWetness1(outfit_with_visor->visorWetness_1_);

		}
	}
	else
	{
		g_hud->hudGlassEffects_.SetCastHudGlassEffects(false);
	}
	
	g_hud->hudGlassEffects_.ActorHealth = conditions().GetHealth();
	g_hud->hudGlassEffects_.ActorMaxHealth = conditions().GetMaxHealth();
}



void CActor::ComputeCondition()
{
	COutfitBase* outfit_with_visor = nullptr;
	g_hud->hudGlassEffects_.Condition = 100;
	// outfit with helmet and regular helmets are exlcluding each other, so no need to do extra checks for priority
	if (inventory().ItemFromSlot(OUTFIT_SLOT))
	{
		COutfitBase* outf = smart_cast<COutfitBase*>(inventory().ItemFromSlot(OUTFIT_SLOT));

		if (outf && outf->hasVisorEffects_)
			outfit_with_visor = outf;
	}
	if (inventory().ItemFromSlot(HELMET_SLOT))
	{
		COutfitBase* helm = smart_cast<COutfitBase*>(inventory().ItemFromSlot(HELMET_SLOT));

		if (helm && helm->hasVisorEffects_)
			outfit_with_visor = helm;
	}

	if (outfit_with_visor)
	{
		float cond = outfit_with_visor->GetConditionToShow();
		g_hud->hudGlassEffects_.SetMaskCondition(cond * 100);;
	}
}