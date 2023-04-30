#include "stdafx.h"
#include "../../xrphysics/PhysicsShell.h"
#include "Level.h"
#include "BoneProtections.h"
#include "../../Include/xrRender/Kinematics.h"
#include "OutfitBase.h"
#include "WeaponBinocularsVision.h"
#include "gamepersistent.h"
#include "../CustomHUD.h"


#define ALIVE_SCANNER_SECT_PARAM "autodetect_vision"

#define MIN_RDROOPS_SPREAD_VALUE 0.05f
#define MAX_RDROOPS_SPREAD_VALUE 0.1f


COutfitBase::COutfitBase()
	: m_adetector(nullptr)
	, m_alive_detector_state(false)
{
	m_flags.set(FUsingCondition, TRUE);

	m_HitTypeProtection.resize(ALife::eHitTypeMax);
	for(int i=0; i<ALife::eHitTypeMax; i++)
		m_HitTypeProtection[i] = 1.0f;

	m_boneProtection		= xr_new <SBoneProtections>();
	m_BonesProtectionSect	= NULL;

	hasVisorEffects_		= false;

	visorWetness_1_			= 0.f;
	visorWetnessSpreadValue_1_ = Random.randF(MIN_RDROOPS_SPREAD_VALUE, MAX_RDROOPS_SPREAD_VALUE);
}

COutfitBase::~COutfitBase() 
{
	xr_delete(m_boneProtection);
	xr_delete(m_adetector);
}

void COutfitBase::LoadCfg(LPCSTR section) 
{
	inherited::LoadCfg(section);
	CNightVisionUsable::LoadCfg(section);

	m_HitTypeProtection[ALife::eHitTypeBurn]		= pSettings->r_float(section,"burn_protection");
	m_HitTypeProtection[ALife::eHitTypeStrike]		= pSettings->r_float(section,"strike_protection");
	m_HitTypeProtection[ALife::eHitTypeShock]		= pSettings->r_float(section,"shock_protection");
	m_HitTypeProtection[ALife::eHitTypeWound]		= pSettings->r_float(section,"wound_protection");
	m_HitTypeProtection[ALife::eHitTypeRadiation]	= pSettings->r_float(section,"radiation_protection");
	m_HitTypeProtection[ALife::eHitTypeTelepatic]	= pSettings->r_float(section,"telepatic_protection");
	m_HitTypeProtection[ALife::eHitTypeChemicalBurn]= pSettings->r_float(section,"chemical_burn_protection");
	m_HitTypeProtection[ALife::eHitTypeExplosion]	= pSettings->r_float(section,"explosion_protection");
	m_HitTypeProtection[ALife::eHitTypeFireWound]	= pSettings->r_float(section,"fire_wound_protection");
	m_HitTypeProtection[ALife::eHitTypePhysicStrike]	= READ_IF_EXISTS(pSettings, r_float, section, "physic_strike_protection", 0.0f);

	m_boneProtection->m_fHitFracActor					= READ_IF_EXISTS(pSettings, r_float, section, "hit_fraction_actor", 0.0f); // pSettings->r_float(section, "hit_fraction_actor");

	m_fHealthRestoreSpeed		= READ_IF_EXISTS(pSettings, r_float, section, "health_restore_speed", 0.0f);
	m_fRadiationRestoreSpeed	= READ_IF_EXISTS(pSettings, r_float, section, "radiation_restore_speed", 0.0f);
	m_fSatietyRestoreSpeed		= READ_IF_EXISTS(pSettings, r_float, section, "satiety_restore_speed", 0.0f);
	m_fPowerRestoreSpeed		= READ_IF_EXISTS(pSettings, r_float, section, "power_restore_speed", 0.0f);
	m_fBleedingRestoreSpeed		= READ_IF_EXISTS(pSettings, r_float, section, "bleeding_restore_speed", 0.0f);
	m_fPowerLoss				= READ_IF_EXISTS(pSettings, r_float, section, "power_loss", 1.0f);
	clamp(m_fPowerLoss, 0.0f, 1.0f);

	m_BonesProtectionSect	= READ_IF_EXISTS(pSettings, r_string, section, "bones_koeff_protection",  "" );

	m_armorTestBoneBody		= READ_IF_EXISTS(pSettings, r_string, section, "armor_test_bone_body", "");
	m_armorTestBoneHead		= READ_IF_EXISTS(pSettings, r_string, section, "armor_test_bone_head", "");

	m_alive_detector_upgrade_sect = READ_IF_EXISTS(pSettings, r_string, section, ALIVE_SCANNER_SECT_PARAM, "");

	hasVisorEffects_		= !!READ_IF_EXISTS(pSettings, r_bool, section, "casts_visor_effects", FALSE);

	block_pnv_slot			= !!READ_IF_EXISTS(pSettings, r_bool, section, "block_pnv_slot", FALSE);
}

BOOL COutfitBase::SpawnAndImportSOData(CSE_Abstract* data_containing_so)
{
	ReloadBonesProtection();
	
	return inherited::SpawnAndImportSOData(data_containing_so);
}

void COutfitBase::UpdateCL()
{
#ifdef MEASURE_UPDATES
	CTimer measure_updatecl; measure_updatecl.Start();
#endif


	inherited::UpdateCL();

	if (m_adetector)
	{
		m_adetector->Update();
	}
	
	
#ifdef MEASURE_UPDATES
	Device.Statistic->updateCL_VariousItems_ += measure_updatecl.GetElapsed_sec();
#endif
}

void COutfitBase::RemoveLinksToCLObj(CObject* object)
{
	inherited::RemoveLinksToCLObj(object);

	if (m_adetector)
		m_adetector->remove_links(object);
}

bool COutfitBase::render_item_ui_query()
{
	return GetAliveDetectorState();
}

void COutfitBase::render_item_ui()
{
	inherited::render_item_ui();

	if (m_adetector)
	{
		m_adetector->Draw();
	}		
}

void COutfitBase::Hit(float hit_power, ALife::EHitType hit_type)
{
	hit_power *= GetHitImmunity(hit_type);
	ChangeCondition(-hit_power);
}

float COutfitBase::GetDefHitTypeProtection(ALife::EHitType hit_type)
{
	return m_HitTypeProtection[hit_type]*GetCondition();
}

float COutfitBase::GetHitTypeProtection(ALife::EHitType hit_type, s16 element)
{
	float fBase = m_HitTypeProtection[hit_type]*GetCondition();
	float bone = m_boneProtection->getBoneProtection(element);
	return fBase*bone;
}

//tatarinrafa: Берем ситсему из ЗП

float COutfitBase::GetBoneArmor(s16 element)
{
	return m_boneProtection->getBoneArmor(element);
}

float COutfitBase::HitThroughArmor(float hit_power, s16 element, float ap, bool& add_wound, ALife::EHitType hit_type)
{
	float NewHitPower = hit_power;
	if (hit_type == ALife::eHitTypeFireWound)
	{
		float ba = GetBoneArmor(element);
		if (ba<0.0f)
			return NewHitPower;

		float BoneArmor = ba*GetCondition();
		if (/*!fis_zero(ba, EPS) && */(ap > BoneArmor))
		{
			//пуля пробила бронь
			float d_hit_power = (ap - BoneArmor) / ap;
			if (d_hit_power < m_boneProtection->m_fHitFracActor)
			{
				d_hit_power = m_boneProtection->m_fHitFracActor;
			}			
			NewHitPower *= d_hit_power;

			VERIFY(NewHitPower >= 0.0f);
		}
		else
		{
			//пуля НЕ пробила бронь
			NewHitPower *= m_boneProtection->m_fHitFracActor;
			add_wound = false; 	//раны нет
		}
	}
	else if (hit_type == ALife::eHitTypeWound)
	{
		float protect = GetDefHitTypeProtection(hit_type);
		float minimumHit = fmax(hit_power * m_boneProtection->m_fHitFracActor, 0.f);
		NewHitPower -= protect;
		// Костюм не прокушен
		if (NewHitPower < minimumHit)
		{
			NewHitPower = minimumHit;
			add_wound = false; 	//раны нет
		}
	}
	else
	{
		float one = 0.1f;
		if (hit_type == ALife::eHitTypeStrike ||
			hit_type == ALife::eHitTypeWound_2 ||
			hit_type == ALife::eHitTypeExplosion)
		{
			one = 1.0f;
		}
		float protect = GetDefHitTypeProtection(hit_type);
		NewHitPower -= protect * one;
		if (NewHitPower < 0.f)
			NewHitPower = 0.f;
	}
	//увеличить изношенность костюма
	Hit(hit_power, hit_type);
	//Msg("Hit Through Outfit: new hit power = %f; old hit power = %f; hit_type = %u, ammopiercing = %f; element = %i, bonearmor = %f", NewHitPower, hit_power, hit_type, element, GetBoneArmor(element));
	return NewHitPower;
}

BOOL	COutfitBase::BonePassBullet					(int boneID)
{
	return m_boneProtection->getBonePassBullet(s16(boneID));
};

float COutfitBase::GetPowerLoss() const
{
	if (m_fPowerLoss<1 && GetCondition() <= 0)
	{
		return 1.0f;			
	};
	return m_fPowerLoss;
}

bool COutfitBase::GetAliveDetectorState() const
{
	return m_alive_detector_state;
}

void COutfitBase::SwitchAliveDetector()
{
	SwitchAliveDetector(!m_alive_detector_state);
}

void COutfitBase::SwitchAliveDetector(bool state)
{
	if (!m_alive_detector_upgrade_sect.size())
		return;

	m_alive_detector_state = state;

	if (m_alive_detector_state)
	{
		if (!m_adetector)
			m_adetector = xr_new<CBinocularsVision>(m_alive_detector_upgrade_sect);
	}
	else
	{
		xr_delete(m_adetector);
	}
}

bool COutfitBase::install_upgrade_impl( LPCSTR section, bool test )
{
	bool result = inherited::install_upgrade_impl( section, test );

	result |= process_if_exists( section, "burn_protection",          &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeBurn]        , test );
	result |= process_if_exists( section, "shock_protection",         &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeShock]       , test );
	result |= process_if_exists( section, "strike_protection",        &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeStrike]      , test );
	result |= process_if_exists( section, "wound_protection",         &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeWound]       , test );
	result |= process_if_exists( section, "radiation_protection",     &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeRadiation]   , test );
	result |= process_if_exists( section, "telepatic_protection",     &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeTelepatic]   , test );
	result |= process_if_exists( section, "chemical_burn_protection", &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeChemicalBurn], test );
	result |= process_if_exists( section, "explosion_protection",     &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeExplosion]   , test );
	result |= process_if_exists( section, "fire_wound_protection",    &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeFireWound]   , test );
//	result |= process_if_exists( section, "physic_strike_protection", &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypePhysicStrike], test );

	LPCSTR str;
	bool result2;

	result2 = process_if_exists_set( section, "bones_koeff_protection", &CInifile::r_string, str, test );
	if ( result2 && !test )
	{
		m_BonesProtectionSect	= str;
		ReloadBonesProtection	();
	}
	result2 = process_if_exists_set( section, "bones_koeff_protection_add", &CInifile::r_string, str, test );
	if ( result2 && !test )
		AddBonesProtection	(str);

	result |= result2;
	result |= process_if_exists( section, "hit_fraction_actor", &CInifile::r_float, m_boneProtection->m_fHitFracActor, test );

	result |= process_if_exists( section, "health_restore_speed",    &CInifile::r_float, m_fHealthRestoreSpeed,    test );
	result |= process_if_exists( section, "radiation_restore_speed", &CInifile::r_float, m_fRadiationRestoreSpeed, test );
	result |= process_if_exists( section, "satiety_restore_speed",   &CInifile::r_float, m_fSatietyRestoreSpeed,   test );
	result |= process_if_exists( section, "power_restore_speed",     &CInifile::r_float, m_fPowerRestoreSpeed,     test );
	result |= process_if_exists( section, "bleeding_restore_speed",  &CInifile::r_float, m_fBleedingRestoreSpeed,  test );
	result |= process_if_exists( section, "power_loss", &CInifile::r_float, m_fPowerLoss, test );
	clamp( m_fPowerLoss, 0.0f, 1.0f );

	result |= process_if_exists_set(section, "block_pnv_slot", &CInifile::r_bool, block_pnv_slot, test);

	result2 = process_if_exists_set(section, "nightvision_sect_ppe", &CInifile::r_string, str, test);
	if (result2 && !test)
	{
		NightVisionSectionPPE._set(str);
	}

	result |= result2;

	result2 = process_if_exists_set(section, "nightvision_sect_shader", &CInifile::r_string, str, test);
	if (result2 && !test)
	{
		NightVisionSectionShader._set(str);
	}

	result |= result2;

	result |= process_if_exists_set(section, ALIVE_SCANNER_SECT_PARAM, &CInifile::r_string_wb, m_alive_detector_upgrade_sect, test);

	return result;
}

void COutfitBase::AddBonesProtection(LPCSTR bones_section)
{
	CObject* parent = smart_cast<CObject*>(Level().CurrentViewEntity());

	if (parent && parent->Visual() && m_BonesProtectionSect.size())
		m_boneProtection->add(bones_section, smart_cast<IKinematics*>(parent->Visual()));
}

void COutfitBase::ReloadBonesProtection()
{
	CObject* parent = smart_cast<CObject*>(Level().CurrentViewEntity());

	if (parent && parent->Visual() && m_BonesProtectionSect.size())
		m_boneProtection->reload(m_BonesProtectionSect, smart_cast<IKinematics*>(parent->Visual()));
}

float COutfitBase::GetArmorByBoneName(const shared_str& boneName)
{
	auto parent = smart_cast<CObject*>(Level().CurrentViewEntity());
	if (parent && parent->Visual() && m_boneProtection && boneName.size() > 0)
	{
		auto kinematics = smart_cast<IKinematics*>(parent->Visual());
		VERIFY(kinematics);
		return m_boneProtection->getBoneArmor(kinematics->LL_BoneID(boneName));
	}
	return 0.0f;
}

float COutfitBase::GetArmorBody()
{
	return GetArmorByBoneName(m_armorTestBoneBody);
}

float COutfitBase::GetArmorHead()
{
	return GetArmorByBoneName(m_armorTestBoneHead);
}

void COutfitBase::OnMoveToRuck()
{
	if (m_pCurrentInventory)
	{
		visorWetness_1_ = 0.f; // как буд-то гг протер перед убиранием
		g_hud->hudGlassEffects_.SetActorHudWetness1(visorWetness_1_);
	}

};

void COutfitBase::save(NET_Packet &packet)
{
	inherited::save(packet);

	packet.w_float			(visorWetness_1_);
}

void COutfitBase::load(IReader &packet)
{
	inherited::load(packet);

	visorWetness_1_			= packet.r_float();
	g_hud->hudGlassEffects_.SetActorHudWetness1(visorWetness_1_);
}