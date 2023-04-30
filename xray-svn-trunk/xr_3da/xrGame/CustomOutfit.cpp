#include "stdafx.h"

#include "customoutfit.h"
#include "inventory_space.h"
#include "Inventory.h"
#include "Actor.h"
#include "Level.h"
#include "BoneProtections.h"
#include "../../Include/xrRender/Kinematics.h"
#include "player_hud.h"

CCustomOutfit::CCustomOutfit()
{
	m_baseSlot			= OUTFIT_SLOT;
}

CCustomOutfit::~CCustomOutfit()
{
}

void CCustomOutfit::ExportDataToServer(NET_Packet& P)
{
	inherited::ExportDataToServer(P);
}

void CCustomOutfit::LoadCfg(LPCSTR section)
{
	inherited::LoadCfg(section);

	if (pSettings->line_exist(section, "actor_visual"))
		m_ActorVisual = pSettings->r_string(section, "actor_visual");
	else
		m_ActorVisual = NULL;

	if (pSettings->line_exist(section, "actor_visual_legs"))
		m_ActorVisual_legs = pSettings->r_string(section, "actor_visual_legs");
	else
		m_ActorVisual_legs = NULL;

	m_ef_equipment_type = pSettings->r_u32(section, "ef_equipment_type");

	m_additional_weight = pSettings->r_float(section, "additional_inventory_weight");
	m_additional_weight2 = pSettings->r_float(section, "additional_inventory_weight2");

	//tatarinrafa: added additional jump speed sprint speed walk speed
	m_additional_jump_speed = READ_IF_EXISTS(pSettings, r_float, section, "additional_jump_speed", 0.0f);
	m_additional_run_coef = READ_IF_EXISTS(pSettings, r_float, section, "additional_run_coef", 0.0f);
	m_additional_sprint_koef = READ_IF_EXISTS(pSettings, r_float, section, "additional_sprint_koef", 0.0f);

	block_helmet_slot	= !!READ_IF_EXISTS(pSettings, r_bool, section, "block_helmet_slot", FALSE);

	maxArtefactCount_	= READ_IF_EXISTS(pSettings, r_u32, section, "artefact_count", 0);
	maxAmmoCount_		= READ_IF_EXISTS(pSettings, r_u32, section, "ammo_belt_capacity", 0);

	m_full_icon_name = pSettings->r_string(section, "full_icon_name");
}

void CCustomOutfit::DestroyClientObj()
{
	inherited::DestroyClientObj();
}

BOOL CCustomOutfit::SpawnAndImportSOData(CSE_Abstract* data_containing_so)
{
	return inherited::SpawnAndImportSOData(data_containing_so);
}

void CCustomOutfit::BeforeDetachFromParent(bool just_before_destroy)
{
	inherited::BeforeDetachFromParent(just_before_destroy);
}

void	CCustomOutfit::OnMoveToSlot()
{
	inherited::OnMoveToSlot();

	if (m_pCurrentInventory)
	{
		CActor* pActor = smart_cast<CActor*> (m_pCurrentInventory->GetOwner());
		if (pActor)
		{
			if (pActor->IsFirstEye() && !pActor->IsActorShadowsOn())
			{
				if (m_ActorVisual_legs.size())
				{
					shared_str NewVisual = m_ActorVisual_legs;
					pActor->ChangeVisual(NewVisual);

					if (pActor == Level().CurrentViewEntity())
						g_player_hud->load(pSettings->r_string(SectionName(), "player_hud_section"));

				}
				else {
					shared_str NewVisual = pActor->GetDefaultVisualOutfit_legs();
					pActor->ChangeVisual(NewVisual);

					if (pActor == Level().CurrentViewEntity())
						g_player_hud->load_default();
				}
			} else {
				if (m_ActorVisual.size())
				{
					shared_str NewVisual = NULL;

					if (!NewVisual.size())
						NewVisual = m_ActorVisual;

					pActor->ChangeVisual(NewVisual);

					if (pActor == Level().CurrentViewEntity())
						g_player_hud->load(pSettings->r_string(SectionName(), "player_hud_section"));

				}
				else {
					shared_str NewVisual = pActor->GetDefaultVisualOutfit();
					pActor->ChangeVisual(NewVisual);

					if (pActor == Level().CurrentViewEntity())
						g_player_hud->load_default();
				}
			}
		}
	}
};

void	CCustomOutfit::OnMoveToRuck()
{
	inherited::OnMoveToRuck();

	if (m_pCurrentInventory)
	{
		CActor* pActor = smart_cast<CActor*> (m_pCurrentInventory->GetOwner());

		if (pActor)
		{
			CCustomOutfit* outfit = pActor->GetOutfit();
			if (!outfit)
			{
				if (pActor->IsFirstEye())
				{
					shared_str DefVisual = pActor->GetDefaultVisualOutfit_legs();
					if (DefVisual.size())
					{
						pActor->ChangeVisual(DefVisual);
					}
				}
				else {
					shared_str DefVisual = pActor->GetDefaultVisualOutfit();
					if (DefVisual.size())
					{
						pActor->ChangeVisual(DefVisual);
					}
				}

				if (pActor == Level().CurrentViewEntity())
					g_player_hud->load_default();

			}
		}
	}
};

u32	CCustomOutfit::ef_equipment_type() const
{
	return		(m_ef_equipment_type);
}

bool CCustomOutfit::install_upgrade_impl(LPCSTR section, bool test)
{
	bool result = inherited::install_upgrade_impl(section, test);

	result |= process_if_exists(section, "additional_inventory_weight", &CInifile::r_float, m_additional_weight, test);
	result |= process_if_exists(section, "additional_inventory_weight2", &CInifile::r_float, m_additional_weight2, test);

	result |= process_if_exists_set(section, "block_helmet_slot", &CInifile::r_bool, block_helmet_slot, test);

	result |= process_if_exists(section, "additional_jump_speed", &CInifile::r_float, m_additional_jump_speed, test);
	result |= process_if_exists(section, "additional_run_coef", &CInifile::r_float, m_additional_run_coef, test);
	result |= process_if_exists(section, "additional_sprint_koef", &CInifile::r_float, m_additional_sprint_koef, test);

	result |= process_if_exists(section, "artefact_count", &CInifile::r_u32, maxArtefactCount_, test);
	result |= process_if_exists(section, "ammo_belt_capacity", &CInifile::r_u32, maxAmmoCount_, test);

	return result;
}
