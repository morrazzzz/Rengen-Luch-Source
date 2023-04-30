#include "stdafx.h"
#include "Actor.h"

#include "inventory.h"
#include "CharacterPhysicsSupport.h"
#include "actor_memory.h"
#include "ActorCondition.h"
#include "ActorEffector.h"
#include "mounted_turret.h"
#include "weapon.h"
#include "level.h"
#include "UIGameCustom.h"
#include "ui/UIMainIngameWnd.h"
#include "CustomOutfit.h"
#include "artifact.h"
#include "../CameraBase.h"

float CActor::currentFOV()
{
	CWeapon* pWeapon = smart_cast<CWeapon*>(inventory().ActiveItem());

	if (eacFreeLook != cam_active && pWeapon &&	pWeapon->IsZoomed() && (!pWeapon->ZoomTexture() ||
		(!pWeapon->IsRotatingToZoom() && pWeapon->ZoomTexture() && eacFirstEye == cam_active) || (pWeapon->ZoomTexture() && eacLookAt == cam_active)))
		return pWeapon->GetZoomFactor() * (0.75f);
	else
		return camFov;
}

void CActor::SetZoomRndSeed(s32 Seed)
{
	if (0 != Seed) m_ZoomRndSeed = Seed;
	else m_ZoomRndSeed = s32(Level().timeServer_Async());
};

void CActor::SetShotRndSeed(s32 Seed)
{
	if (0 != Seed)
		m_ShotRndSeed = Seed;
	else
		m_ShotRndSeed = s32(Level().timeServer_Async());
};

void CActor::AnimTorsoPlayCallBack(CBlend* B)
{
	CActor* actor = (CActor*)B->CallbackParam;
	actor->m_bAnimTorsoPlayed = FALSE;
}

void CActor::SetActorVisibility(u16 who, float value)
{
	CUIMotionIcon& motion_icon = CurrentGameUI()->UIMainIngameWnd->MotionIcon();

	motion_icon.SetActorVisibility(who, value);
}

void CActor::OnDifficultyChanged()
{
	// immunities
	VERIFY(g_SingleGameDifficulty >= egdNovice && g_SingleGameDifficulty <= egdMaster);

	LPCSTR diff_name = get_token_name(difficulty_type_token, g_SingleGameDifficulty);

	string128 tmp;
	strconcat(sizeof(tmp), tmp, "actor_immunities_", diff_name);

	conditions().LoadImmunities(tmp, pSettings);

	// hit probability
	strconcat(sizeof(tmp), tmp, "hit_probability_", diff_name);
	m_hit_probability = pSettings->r_float(*SectionName(), tmp);

	// two hits death parameters
	strconcat(sizeof(tmp), tmp, "actor_thd_", diff_name);
	conditions().LoadTwoHitsDeathParams(tmp);

}

CCustomOutfit* CActor::GetOutfit() const
{
	PIItem _of = inventory().m_slots[OUTFIT_SLOT].m_pIItem;

	return _of ? smart_cast<CCustomOutfit*>(_of) : NULL;
}

void CActor::RechargeTorchBattery(void)
{
	m_current_torch->Recharge();
}

CTorch *CActor::GetCurrentTorch(void)
{
	if (inventory().ItemFromSlot(TORCH_SLOT))
	{
		CTorch* torch = smart_cast<CTorch*>(inventory().ItemFromSlot(TORCH_SLOT));

		if (torch)
			m_current_torch = torch;
		else
			m_current_torch = 0;
	}
	else
		m_current_torch = 0;

	return m_current_torch;
}

bool CActor::UsingTurret()
{
	return m_holder && smart_cast<CMountedTurret*>(m_holder);
}

u16 CActor::GetTurretTemp()
{
	CMountedTurret* turret = smart_cast<CMountedTurret*>(m_holder);

	R_ASSERT(turret);

	return turret->GetTemperature();
}

void CActor::SetDirectionSlowly(Fvector pos, float time)
{
	if (!m_ScriptCameraDirection)
		m_ScriptCameraDirection = xr_new <CScriptCameraDirection>();

	m_ScriptCameraDirection->Start(this, pos, time);
}

void CActor::SetIconState(EActorState state, bool show)
{
	m_icons_state.set(1 << state, show);
}

float CActor::SetWalkAccel(float new_value)
{
	float old_value = m_fWalkAccel;
	m_fWalkAccel = new_value;

	return old_value;
}

bool CActor::CanPutInSlot(PIItem item, u32 slot)
{
	if (slot == RIFLE_2_SLOT && !m_secondRifleSlotAllowed)
	{
		return false;
	}

	return CInventoryOwner::CanPutInSlot(item, slot);
}

float CActor::GetMass()
{
	return g_Alive() ? character_physics_support()->movement()->GetMass() : m_pPhysicsShell ? m_pPhysicsShell->getMass() : 0;
}

bool CActor::is_on_ground()
{
	return (character_physics_support()->movement()->Environment() != CPHMovementControl::peInAir);
}

bool CActor::is_ai_obstacle() const
{
	return (false);
}

bool CActor::unlimited_ammo()
{
	return !!psActorFlags.test(AF_UNLIMITEDAMMO);
}

void CActor::SetPhPosition(const Fmatrix& transform)
{
	if (!m_pPhysicsShell)
	{
		character_physics_support()->movement()->SetPosition(transform.c);
	}
}

void CActor::ForceTransform(const Fmatrix& m)
{
	character_physics_support()->ForceTransform(m);
}

float CActor::Radius()const
{
	float R = inherited::Radius();

	CWeapon* W = smart_cast<CWeapon*>(inventory().ActiveItem());

	if (W)
		R += W->Radius();

	//	if (HUDview()) R *= 1.f/psHUD_FOV;

	return R;
}

bool CActor::use_bolts() const
{
	return CInventoryOwner::use_bolts();
};

CVisualMemoryManager *CActor::visual_memory() const
{
	return (&memory().visual());
}

bool CActor::use_center_to_aim() const
{
	return (!!(mstate_real&mcCrouch));
}

bool CActor::can_attach(const CInventoryItem* inventory_item) const
{
	const CAttachableItem* item = smart_cast<const CAttachableItem*>(inventory_item);
	if (!item || (item && !item->can_be_attached()))
		return(false);

	//можно ли присоединять объекты такого типа
	if (m_attach_item_sections.end() == std::find(m_attach_item_sections.begin(), m_attach_item_sections.end(), inventory_item->object().SectionName()))
		return false;

	//если уже есть присоединненый объет такого типа 
	if (attached(inventory_item->object().SectionName()))
		return false;

	return true;
}

void CActor::spawn_supplies()
{
	inherited::spawn_supplies();
	CInventoryOwner::spawn_supplies();
}

bool CActor::InventoryAllowSprint()
{
	PIItem pActiveItem = inventory().ActiveItem();

	if (pActiveItem && !pActiveItem->IsSprintAllowed())
	{
		return false;
	};

	PIItem pOutfitItem = inventory().ItemFromSlot(OUTFIT_SLOT);

	if (pOutfitItem && !pOutfitItem->IsSprintAllowed())
	{
		return false;
	}

	for (u16 it = 0; it < inventory().artefactBelt_.size(); ++it)
	{
		CArtefact*	artefact = smart_cast<CArtefact*>(inventory().artefactBelt_[it]);

		if (artefact && artefact->IsSprintAllowed() == false)
		{
			return false;
		}
	}

	return true;
};

BOOL CActor::BonePassBullet(int boneID)
{
	return inherited::BonePassBullet(boneID);
}

void CActor::On_B_NotCurrentEntity()
{
	inventory().Items_SetCurrentEntityHud(false);
};

void CActor::MoveActor(Fvector NewPos, Fvector NewDir)
{
	Fmatrix	M = XFORM();
	M.translate(NewPos);
	r_model_yaw = NewDir.y;
	r_torso.yaw = NewDir.y;
	r_torso.pitch = -NewDir.x;
	unaffected_r_torso.yaw = r_torso.yaw;
	unaffected_r_torso.pitch = r_torso.pitch;
	unaffected_r_torso.roll = 0;//r_torso.roll;

	r_torso_tgt_roll = 0;
	cam_Active()->Set(-unaffected_r_torso.yaw, unaffected_r_torso.pitch, unaffected_r_torso.roll);
	ForceTransform(M);
}

void CActor::OnHitHealthLoss(float NewHealth)
{
};

void CActor::OnCriticalHitHealthLoss()
{
};

void CActor::OnCriticalWoundHealthLoss()
{
};

void CActor::OnCriticalRadiationHealthLoss()
{
};

float CActor::GetRawHitDamage(float hit_power, ALife::EHitType hit_type)
{
	COutfitBase* outfit = smart_cast<COutfitBase*>(inventory().ItemFromSlot(OUTFIT_SLOT));
	COutfitBase* helmet = smart_cast<COutfitBase*>(inventory().ItemFromSlot(HELMET_SLOT));

	float value = hit_power;

	if (outfit)
		value *= 1.f - outfit->GetDefHitTypeProtection(hit_type);

	if (helmet)
		value *= 1.f - helmet->GetDefHitTypeProtection(hit_type);


	value = HitArtefactsOnBelt(value, hit_type);
	value = HitBoosters(value, hit_type);

	return value;
}

bool CActor::use_default_throw_force()
{
	if (!g_Alive())
		return false;

	return true;
}

float CActor::missile_throw_force()
{
	return 0.f;
}

void CActor::set_state_box(u32	mstate)
{
	if (mstate & mcCrouch)
	{
		if (isActorAccelerated(mstate_real, IsZoomAimingMode()))
			character_physics_support()->movement()->ActivateBox(1, true);
		else
			character_physics_support()->movement()->ActivateBox(2, true);
	}
	else
		character_physics_support()->movement()->ActivateBox(0, true);
}