#include "stdafx.h"
#include "weaponammo.h"
#include "../../xrphysics/PhysicsShell.h"
#include "xrserver_objects_alife_items.h"
#include "inventory.h"
#include "weapon.h"
#include "../gamemtllib.h"
#include "level.h"
#include "string_table.h"

#define BULLET_MANAGER_SECTION "bullet_manager"

CCartridge::CCartridge() 
{
	m_flags.assign			(cfTracer | cfRicochet);
	m_ammoSect = NULL;
	param_s.Init();
	bullet_material_idx = u16(-1);
}

void CCartridge::Load(LPCSTR section, u8 LocalAmmoType) 
{
	m_ammoSect				= section;
	m_LocalAmmoType			= LocalAmmoType;
	param_s.kDist				= pSettings->r_float(section, "k_dist");
	param_s.kDisp				= pSettings->r_float(section, "k_disp");
	param_s.kHit				= pSettings->r_float(section, "k_hit");
//.	param_s.kCritical			= pSettings->r_float(section, "k_hit_critical");
	param_s.kImpulse			= pSettings->r_float(section, "k_impulse");
	//m_kPierce				= pSettings->r_float(section, "k_pierce");
	param_s.kAP					= pSettings->r_float(section, "k_ap");
	param_s.u8ColorID			= READ_IF_EXISTS(pSettings, r_u8, section, "tracer_color_ID", 0);
	
	if (pSettings->line_exist(section, "k_air_resistance"))
		param_s.kAirRes			=  pSettings->r_float(section, "k_air_resistance");
	else
		param_s.kAirRes			= pSettings->r_float(BULLET_MANAGER_SECTION, "air_resistance_k");

	param_s.kSpeed				= READ_IF_EXISTS(pSettings, r_float, section, "k_speed", 1.0f);

	m_flags.set					(cfTracer, pSettings->r_bool(section, "tracer"));
	param_s.buckShot			= pSettings->r_s32(  section, "buck_shot");
	param_s.impair				= pSettings->r_float(section, "impair");
	param_s.fWallmarkSize		= pSettings->r_float(section, "wm_size");
	
	m_flags.set					(cfCanBeUnlimited | cfRicochet, TRUE);
	m_flags.set					(cfMagneticBeam, FALSE);

	if (pSettings->line_exist(section, "allow_ricochet"))
	{
		if (!pSettings->r_bool(section, "allow_ricochet"))
			m_flags.set(cfRicochet, FALSE);
	}
	if (pSettings->line_exist(section, "magnetic_beam_shot"))
	{
		if (pSettings->r_bool(section, "magnetic_beam_shot"))
			m_flags.set(cfMagneticBeam, TRUE);
	}

	if(pSettings->line_exist(section,"can_be_unlimited"))
		m_flags.set(cfCanBeUnlimited, pSettings->r_bool(section, "can_be_unlimited"));

	m_flags.set			(cfExplosive, pSettings->r_bool(section, "explosive"));

	bullet_material_idx		=  GMLib.GetMaterialIdx(WEAPON_MATERIAL_NAME);
	VERIFY	(u16(-1)!=bullet_material_idx);
	VERIFY	(param_s.fWallmarkSize>0);

	m_InvShortName			= CStringTable().translate( pSettings->r_string(section, "inv_name_short"));
	//Msg("Allow Ricochet for %s %s", section, (m_flags.test(cfRicochet)) ? "true" : "false");

	param_s.aiAproximateEffectiveDistanceK_ = READ_IF_EXISTS(pSettings, r_float, section, "ai_effective_dist_k", 1.0f);
}

CWeaponAmmo::CWeaponAmmo(void) 
{
}

CWeaponAmmo::~CWeaponAmmo(void)
{
}

void CWeaponAmmo::LoadCfg(LPCSTR section) 
{
	inherited::LoadCfg			(section);

	cartridge_param.kDist		= pSettings->r_float(section, "k_dist");
	cartridge_param.kDisp		= pSettings->r_float(section, "k_disp");
	cartridge_param.kHit		= pSettings->r_float(section, "k_hit");
//.	cartridge_param.kCritical	= pSettings->r_float(section, "k_hit_critical");
	cartridge_param.kImpulse	= pSettings->r_float(section, "k_impulse");
	//m_kPierce				= pSettings->r_float(section, "k_pierce");
	cartridge_param.kAP			= pSettings->r_float(section, "k_ap");
	cartridge_param.u8ColorID	= READ_IF_EXISTS(pSettings, r_u8, section, "tracer_color_ID", 0);

	if (pSettings->line_exist(section, "k_air_resistance"))
		cartridge_param.kAirRes		= pSettings->r_float(section, "k_air_resistance");
	else
		cartridge_param.kAirRes		= pSettings->r_float(BULLET_MANAGER_SECTION, "air_resistance_k");

	cartridge_param.kSpeed = READ_IF_EXISTS(pSettings, r_float, section, "k_speed", 1.0f);
	m_tracer				= !!pSettings->r_bool(section, "tracer");
	cartridge_param.buckShot		= pSettings->r_s32(  section, "buck_shot");
	cartridge_param.impair			= pSettings->r_float(section, "impair");
	cartridge_param.fWallmarkSize	= pSettings->r_float(section, "wm_size");
	R_ASSERT				(cartridge_param.fWallmarkSize>0);

	m_boxSize				= (u16)pSettings->r_s32(section, "box_size");
	m_boxCurr				= m_boxSize;	
}

BOOL CWeaponAmmo::SpawnAndImportSOData(CSE_Abstract* data_containing_so) 
{
	BOOL bResult			= inherited::SpawnAndImportSOData(data_containing_so);
	CSE_Abstract	*e		= (CSE_Abstract*)(data_containing_so);
	CSE_ALifeItemAmmo* l_pW	= smart_cast<CSE_ALifeItemAmmo*>(e);
	m_boxCurr				= l_pW->a_elapsed;
	
	if(m_boxCurr > m_boxSize)
		l_pW->a_elapsed		= m_boxCurr = m_boxSize;

	return					bResult;
}

void CWeaponAmmo::DestroyClientObj() 
{
	inherited::DestroyClientObj();
}

void CWeaponAmmo::BeforeAttachToParent() 
{
	inherited::BeforeAttachToParent();
}

void CWeaponAmmo::BeforeDetachFromParent(bool just_before_destroy) 
{
	if(!Useful()) {
		DestroyObject	();

		m_ready_to_destroy	= true;
	}
	inherited::BeforeDetachFromParent(just_before_destroy);
}


bool CWeaponAmmo::Useful() const
{
	// ���� IItem ��� �� ��������� �������������, ������� true
	return !!m_boxCurr;
}
/*
s32 CWeaponAmmo::Sort(PIItem pIItem) 
{
	// ���� ����� ���������� IItem ����� this - ������� 1, ����
	// ����� - -1. ���� ����� �� 0.
	CWeaponAmmo *l_pA = smart_cast<CWeaponAmmo*>(pIItem);
	if(!l_pA) return 0;
	if(xr_strcmp(SectionName(), l_pA->SectionName())) return 0;
	if(m_boxCurr <= l_pA->m_boxCurr) return 1;
	else return -1;
}
*/
bool CWeaponAmmo::Get(CCartridge &cartridge) 
{
	if(!m_boxCurr) return false;
	cartridge.m_ammoSect = SectionName();
	
	cartridge.param_s = cartridge_param;

	cartridge.m_flags.set(CCartridge::cfTracer ,m_tracer);
	cartridge.bullet_material_idx = GMLib.GetMaterialIdx(WEAPON_MATERIAL_NAME);
	cartridge.m_InvShortName = NameShort();
	--m_boxCurr;
	if(m_pCurrentInventory)
		m_pCurrentInventory->InvalidateState();
	return true;
}

void CWeaponAmmo::renderable_Render(IRenderBuffer& render_buffer) 
{
	if(!m_ready_to_destroy)
		inherited::renderable_Render(render_buffer);
}

void CWeaponAmmo::UpdateCL() 
{
	inherited::UpdateCL	();

	VERIFY2(_valid(renderable.xform), *ObjectName());

}

void CWeaponAmmo::ExportDataToServer(NET_Packet& P) 
{
	inherited::ExportDataToServer(P);
	
	P.w_u16					(m_boxCurr);
}

CInventoryItem *CWeaponAmmo::can_make_killing	(const CInventory *inventory) const
{
	VERIFY					(inventory);

	TIItemContainer::const_iterator	I = inventory->allContainer_.begin();
	TIItemContainer::const_iterator	E = inventory->allContainer_.end();
	for ( ; I != E; ++I) {
		CWeapon		*weapon = smart_cast<CWeapon*>(*I);
		if (!weapon)
			continue;
		xr_vector<shared_str>::const_iterator	i = std::find(weapon->m_ammoTypes.begin(),weapon->m_ammoTypes.end(),SectionName());
		if (i != weapon->m_ammoTypes.end())
			return			(weapon);
	}

	return					(0);
}

float CWeaponAmmo::Weight() const
{
	float res = inherited::Weight();

	res *= (float)m_boxCurr/(float)m_boxSize;

	return res;
}

u32 CWeaponAmmo::Cost() const
{
	u32 res = inherited::Cost();

	res = iFloor(res*(float)m_boxCurr/(float)m_boxSize+0.5f);

	return res;
}
