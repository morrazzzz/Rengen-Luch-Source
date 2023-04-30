////////////////////////////////////////////////////////////////////////////
//	Module 		: xrServer_Objects_ALife_Items.cpp
//	Created 	: 19.09.2002
//  Modified 	: 04.06.2003
//	Author		: Oles Shyshkovtsov, Alexander Maksimchuk, Victor Reutskiy and Dmitriy Iassenev
//	Description : Server objects items for ALife simulator
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "xrMessages.h"
#include "xrServer_Objects_ALife_Items.h"
#include "clsid_game.h"
#include "object_broker.h"



#include "../bone.h"

#ifdef XRGAME_EXPORTS
#	ifdef DEBUG
#		define PHPH_DEBUG
#	endif
#endif

#ifdef PHPH_DEBUG
#include "PHDebug.h"
#endif
////////////////////////////////////////////////////////////////////////////
// CSE_ALifeInventoryItem
////////////////////////////////////////////////////////////////////////////
CSE_ALifeInventoryItem::CSE_ALifeInventoryItem(LPCSTR caSection)
{
	//текущее состояние вещи
	m_fCondition				= 1.0f;

	m_fMass						= pSettings->r_float(caSection, "inv_weight");
	m_dwCost					= pSettings->r_u32(caSection, "cost");

	if (pSettings->line_exist(caSection, "condition"))
		m_fCondition			= pSettings->r_float(caSection, "condition");

	if (pSettings->line_exist(caSection, "health_value"))
		m_iHealthValue			= pSettings->r_s32(caSection, "health_value");
	else
		m_iHealthValue			= 0;

	if (pSettings->line_exist(caSection, "food_value"))
		m_iFoodValue			= pSettings->r_s32(caSection, "food_value");
	else
		m_iFoodValue			= 0;

	m_fDeteriorationValue		= 0;

	m_last_update_time			= 0;

	State.quaternion.x			= 0.f;
	State.quaternion.y			= 0.f;
	State.quaternion.z			= 1.f;
	State.quaternion.w			= 0.f;

	State.angular_vel.set		(0.f,0.f,0.f);
	State.linear_vel.set		(0.f,0.f,0.f);

#ifdef XRGAME_EXPORTS
			m_freeze_time	= EngineTimeU();
#else
			m_freeze_time	= 0;
#endif

	m_relevent_random.seed		(u32(CPU::QPC() & u32(-1)));
	freezed						= false;
}

CSE_Abstract *CSE_ALifeInventoryItem::init	()
{
	m_self						= smart_cast<CSE_ALifeObject*>(this);
	R_ASSERT					(m_self);
//	m_self->m_flags.set			(CSE_ALifeObject::flSwitchOffline,TRUE);
	return						(base());
}

CSE_ALifeInventoryItem::~CSE_ALifeInventoryItem	()
{
}

void CSE_ALifeInventoryItem::STATE_Write	(NET_Packet &tNetPacket)
{
	tNetPacket.w_float			(m_fCondition);
	save_data					(m_upgrades, tNetPacket);
	State.position				= base()->o_Position;
}

void CSE_ALifeInventoryItem::STATE_Read		(NET_Packet &tNetPacket, u16 size)
{
	u16 m_wVersion = base()->m_wVersion;
	if (m_wVersion > 52)
		tNetPacket.r_float		(m_fCondition);

	if (m_wVersion > 119)
	{
		load_data				(m_upgrades, tNetPacket);
	}

	State.position				= base()->o_Position;
}

static inline bool check (const u8 &mask, const u8 &test)
{
	return							(!!(mask & test));
}

const	u32		CSE_ALifeInventoryItem::m_freeze_delta_time		= 1000;
const	u32		CSE_ALifeInventoryItem::random_limit			= 120;		

//if TRUE, then object sends update packet
BOOL CSE_ALifeInventoryItem::NeedDataExport()
{
	if (base()->ID_Parent != u16(-1))
		return		FALSE;

	if (!freezed)
		return		TRUE;

#ifdef XRGAME_EXPORTS
	if (EngineTimeU() >= (m_freeze_time + m_freeze_delta_time))
		return		FALSE;
#endif

	if (!prev_freezed)
	{
		prev_freezed = true;	//i.e. freezed
		return		TRUE;
	}

	if (m_relevent_random.randI(random_limit))
		return		FALSE;

	return			TRUE;
}

void CSE_ALifeInventoryItem::UPDATE_Write(NET_Packet &tNetPacket)
{
	VERIFY(m_fCondition < 1.1);
	clamp(m_fCondition, 0.f, 1.f);
	tNetPacket.w_float_q8(m_fCondition, 0.0f, 1.0f);
};

void CSE_ALifeInventoryItem::UPDATE_Read(NET_Packet &tNetPacket)
{
	tNetPacket.r_float_q8(m_fCondition, 0.0f, 1.0f);
};

void CSE_ALifeInventoryItem::FillProps		(LPCSTR pref, PropItemVec& values)
{
	PHelper().CreateFloat			(values, PrepareKey(pref, *base()->s_name, "Item condition").c_str(), 		&m_fCondition, 			0.f, 1.f);
	CSE_ALifeObject					*alife_object = smart_cast<CSE_ALifeObject*>(base());
	R_ASSERT						(alife_object);
	PHelper().CreateFlag32			(values, PrepareKey(pref, *base()->s_name,"ALife\\Useful for AI").c_str(),	&alife_object->m_flags,	CSE_ALifeObject::flUsefulForAI);
	PHelper().CreateFlag32			(values, PrepareKey(pref, *base()->s_name,"ALife\\Visible for AI").c_str(),	&alife_object->m_flags,	CSE_ALifeObject::flVisibleForAI);
}

bool CSE_ALifeInventoryItem::bfUseful		()
{
	return						(true);
}

u32 CSE_ALifeInventoryItem::update_rate		() const
{
	return						(1000);
}

bool CSE_ALifeInventoryItem::has_upgrade( const shared_str& upgrade_id )
{
	return ( std::find( m_upgrades.begin(), m_upgrades.end(), upgrade_id ) != m_upgrades.end() );
}

void CSE_ALifeInventoryItem::add_upgrade( const shared_str& upgrade_id )
{
	if ( !has_upgrade( upgrade_id ) )
	{
		m_upgrades.push_back( upgrade_id );
		return;
	}
	FATAL( make_string( "Can`t add existent upgrade (%s)!", upgrade_id.c_str() ).c_str() );
}


////////////////////////////////////////////////////////////////////////////
// CSE_ALifeItem
////////////////////////////////////////////////////////////////////////////
CSE_ALifeItem::CSE_ALifeItem				(LPCSTR caSection) : CSE_ALifeDynamicObjectVisual(caSection), CSE_ALifeInventoryItem(caSection)
{
	m_physics_disabled			= false;
}

CSE_ALifeItem::~CSE_ALifeItem				()
{
}

CSE_Abstract *CSE_ALifeItem::init			()
{
	inherited1::init			();
	inherited2::init			();
	return						(base());
}

CSE_Abstract *CSE_ALifeItem::base			()
{
	return						(inherited1::base());
}

const CSE_Abstract *CSE_ALifeItem::base		() const
{
	return						(inherited1::base());
}

void CSE_ALifeItem::STATE_Write				(NET_Packet &tNetPacket)
{
	inherited1::STATE_Write		(tNetPacket);
	inherited2::STATE_Write		(tNetPacket);
}

void CSE_ALifeItem::STATE_Read				(NET_Packet &tNetPacket, u16 size)
{
	inherited1::STATE_Read		(tNetPacket, size);
	if ((m_tClassID == CLSID_OBJECT_W_BINOCULAR) && (m_wVersion < 37)) {
		tNetPacket.r_u16		();
		tNetPacket.r_u16		();
		tNetPacket.r_u8			();
	}
	inherited2::STATE_Read		(tNetPacket, size);
}

void CSE_ALifeItem::UPDATE_Write			(NET_Packet &tNetPacket)
{
	inherited1::UPDATE_Write	(tNetPacket);
	inherited2::UPDATE_Write	(tNetPacket);

#ifdef XRGAME_EXPORTS
	m_last_update_time			= EngineTimeU();
#endif // XRGAME_EXPORTS
};

void CSE_ALifeItem::UPDATE_Read				(NET_Packet &tNetPacket)
{
	inherited1::UPDATE_Read		(tNetPacket);
	inherited2::UPDATE_Read		(tNetPacket);

	m_physics_disabled			= false;
};

void CSE_ALifeItem::FillProps				(LPCSTR pref, PropItemVec& values)
{
	inherited1::FillProps		(pref,	 values);
	inherited2::FillProps		(pref,	 values);
}

BOOL CSE_ALifeItem::NeedDataExport			()
{
	if (inherited1::NeedDataExport())
		return					(TRUE);

	if (inherited2::NeedDataExport())
		return					(TRUE);

	if (attached())
		return					(FALSE);

	if (!m_physics_disabled && !fis_zero(State.linear_vel.square_magnitude(),EPS_L))
		return					(TRUE);

#ifdef XRGAME_EXPORTS
//	if (EngineTimeU() < (m_last_update_time + update_rate()))
//		return					(FALSE);
#endif // XRGAME_EXPORTS

	return						(FALSE);
}

void CSE_ALifeItem::OnEvent					(NET_Packet &tNetPacket, u16 type, u32 time, ClientID sender )
{
	inherited1::OnEvent			(tNetPacket,type,time,sender);
}

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeItemTorch
////////////////////////////////////////////////////////////////////////////
CSE_ALifeItemTorch::CSE_ALifeItemTorch		(LPCSTR caSection) : CSE_ALifeItem(caSection)
{
	m_active					= false;
	m_attached					= false;
	m_battery_state				= pSettings->r_u16(caSection, "battery_duration");
}

CSE_ALifeItemTorch::~CSE_ALifeItemTorch		()
{
}

BOOL	CSE_ALifeItemTorch::NeedDataExport			()
{
	if (m_attached) return true;
	return inherited::NeedDataExport();
}


void CSE_ALifeItemTorch::STATE_Read			(NET_Packet	&tNetPacket, u16 size)
{
	if (m_wVersion > 20)
		inherited::STATE_Read	(tNetPacket,size);

}

void CSE_ALifeItemTorch::STATE_Write		(NET_Packet	&tNetPacket)
{
	inherited::STATE_Write		(tNetPacket);
}

void CSE_ALifeItemTorch::UPDATE_Read		(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Read		(tNetPacket);
	
	BYTE F = tNetPacket.r_u8();
	m_active					= !!(F & eTorchActive);
	m_attached					= !!(F & eAttached);
	m_battery_state				= tNetPacket.r_u16();
}

void CSE_ALifeItemTorch::UPDATE_Write		(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Write		(tNetPacket);

	BYTE F = 0;
	F |= (m_active ? eTorchActive : 0);
	F |= (m_attached ? eAttached : 0);
	tNetPacket.w_u8(F);
	tNetPacket.w_u16(m_battery_state);
}

void CSE_ALifeItemTorch::FillProps			(LPCSTR pref, PropItemVec& values)
{
	inherited::FillProps			(pref,	 values);
}

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeItemWeapon
////////////////////////////////////////////////////////////////////////////
CSE_ALifeItemWeapon::CSE_ALifeItemWeapon	(LPCSTR caSection) : CSE_ALifeItem(caSection)
{
	a_current							=	90;
	a_elapsed							=	0;
	wpn_flags							=	0;
	wpn_state							=	0;
	ammo_type							=	0;

	m_fHitPower					= pSettings->r_float(caSection,"hit_power");
	m_tHitType					= ALife::g_tfString2HitType(pSettings->r_string(caSection,"hit_type"));
	m_caAmmoSections			= pSettings->r_string(caSection,"ammo_class");
	if (pSettings->section_exist(caSection) && pSettings->line_exist(caSection,"visual"))
        set_visual				(pSettings->r_string(caSection,"visual"));

	m_cur_scope					= 0;
	m_addon_flags.zero			();

	m_scope_status				=	(EWeaponAddonStatus)pSettings->r_s32(s_name,"scope_status");
	m_silencer_status			=	(EWeaponAddonStatus)pSettings->r_s32(s_name,"silencer_status");
	m_grenade_launcher_status	=	(EWeaponAddonStatus)pSettings->r_s32(s_name,"grenade_launcher_status");
	m_ef_main_weapon_type		= READ_IF_EXISTS(pSettings,r_u32,caSection,"ef_main_weapon_type",u32(-1));
	m_ef_weapon_type			= READ_IF_EXISTS(pSettings,r_u32,caSection,"ef_weapon_type",u32(-1));

	m_MagazinePacked.clear();
}

CSE_ALifeItemWeapon::~CSE_ALifeItemWeapon	()
{
}

u32	CSE_ALifeItemWeapon::ef_main_weapon_type() const
{
	VERIFY	(m_ef_main_weapon_type != u32(-1));
	return	(m_ef_main_weapon_type);
}

u32	CSE_ALifeItemWeapon::ef_weapon_type() const
{
	VERIFY	(m_ef_weapon_type != u32(-1));
	return	(m_ef_weapon_type);
}

void CSE_ALifeItemWeapon::UPDATE_Read(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Read		(tNetPacket);

	tNetPacket.r_u8				(wpn_flags);
	tNetPacket.r_u16			(a_elapsed);

	if (m_wVersion > 120) {
		tNetPacket.r_u8				(m_cur_scope);
	}

	tNetPacket.r_u8				(m_addon_flags.flags);
	tNetPacket.r_u8				(ammo_type);

	m_MagazinePacked.clear();
	u16 cnt = tNetPacket.r_u16();

	VERIFY(m_MagazinePacked.size() % 2 == 0);

	for (u16 i = 0; i < cnt; i += 2)
	{
		m_MagazinePacked.push_back(tNetPacket.r_u8());
		m_MagazinePacked.push_back(tNetPacket.r_u16());
	}

	tNetPacket.r_u8				(wpn_state);
	tNetPacket.r_u8				(m_bZoom);
}

void CSE_ALifeItemWeapon::clone_addons(CSE_ALifeItemWeapon* parent)
{
	m_addon_flags = parent->m_addon_flags;
}

void CSE_ALifeItemWeapon::UPDATE_Write(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Write		(tNetPacket);

	tNetPacket.w_u8				(wpn_flags);
	tNetPacket.w_u16			(a_elapsed);
	tNetPacket.w_u8				(m_cur_scope);
	tNetPacket.w_u8				(m_addon_flags.get());
	tNetPacket.w_u8				(ammo_type);

	tNetPacket.w_u16			(u16(m_MagazinePacked.size()));

	for (u32 i = 0; i < m_MagazinePacked.size(); i += 2)
	{
		tNetPacket.w_u8((u8)m_MagazinePacked[i]);
		tNetPacket.w_u16(m_MagazinePacked[i + 1]);
	}

	tNetPacket.w_u8				(wpn_state);
	tNetPacket.w_u8				(m_bZoom);
}

void CSE_ALifeItemWeapon::STATE_Read(NET_Packet	&tNetPacket, u16 size)
{
	inherited::STATE_Read		(tNetPacket, size);
	tNetPacket.r_u16			(a_current);
	tNetPacket.r_u16			(a_elapsed);
	tNetPacket.r_u8				(wpn_state);
	
	if (m_wVersion > 40)
		tNetPacket.r_u8			(m_addon_flags.flags);

	if (m_wVersion > 46)
		tNetPacket.r_u8			(ammo_type);
}

void CSE_ALifeItemWeapon::STATE_Write		(NET_Packet	&tNetPacket)
{
	inherited::STATE_Write		(tNetPacket);
	tNetPacket.w_u16			(a_current);
	tNetPacket.w_u16			(a_elapsed);
	tNetPacket.w_u8				(wpn_state);
	tNetPacket.w_u8				(m_addon_flags.get());
	tNetPacket.w_u8				(ammo_type);
}

void CSE_ALifeItemWeapon::OnEvent			(NET_Packet	&tNetPacket, u16 type, u32 time, ClientID sender )
{
	inherited::OnEvent			(tNetPacket,type,time,sender);
}

u8 CSE_ALifeItemWeapon::get_addon_flags		() const
{
	return (m_addon_flags.get());
}

u8	 CSE_ALifeItemWeapon::get_slot			()  const
{
	return						((u8)pSettings->r_u8(s_name,"slot"));
}

u16	 CSE_ALifeItemWeapon::get_ammo_limit	()  const
{
	return						(u16) pSettings->r_u16(s_name,"ammo_limit");
}

u16	 CSE_ALifeItemWeapon::get_ammo_total	()  const
{
	return						((u16)a_current);
}

u16	 CSE_ALifeItemWeapon::get_ammo_elapsed	()  const
{
	return						((u16)a_elapsed);
}

void CSE_ALifeItemWeapon::set_ammo_elapsed	(u16 val)
{
	a_elapsed = val;
}

u16	 CSE_ALifeItemWeapon::get_ammo_magsize	() const
{
	if (pSettings->line_exist(s_name,"ammo_mag_size"))
		return					(pSettings->r_u16(s_name,"ammo_mag_size"));
	else
		return					0;
}

BOOL CSE_ALifeItemWeapon::NeedDataExport()
{
	if (wpn_flags==1)
		return TRUE;

	return inherited::NeedDataExport();
}

void CSE_ALifeItemWeapon::FillProps			(LPCSTR pref, PropItemVec& items)
{
	inherited::FillProps			(pref, items);
	PHelper().CreateU8			(items,PrepareKey(pref,*s_name,"Ammo type:").c_str(), &ammo_type,0,255,1);
	PHelper().CreateU16			(items,PrepareKey(pref,*s_name,"Ammo: in magazine").c_str(),	&a_elapsed,0,30,1);
	

	if (m_scope_status == ALife::eAddonAttachable)
	       PHelper().CreateFlag8(items,PrepareKey(pref,*s_name,"Addons\\Scope").c_str(), 	&m_addon_flags, eWeaponAddonScope);

	if (m_silencer_status == ALife::eAddonAttachable)
        PHelper().CreateFlag8	(items,PrepareKey(pref,*s_name,"Addons\\Silencer").c_str(), 	&m_addon_flags, eWeaponAddonSilencer);

	if (m_grenade_launcher_status == ALife::eAddonAttachable)
        PHelper().CreateFlag8	(items,PrepareKey(pref,*s_name,"Addons\\Podstvolnik").c_str(),&m_addon_flags,eWeaponAddonGrenadeLauncher);
}
////////////////////////////////////////////////////////////////////////////
// CSE_ALifeItemWeaponShotGun
////////////////////////////////////////////////////////////////////////////
CSE_ALifeItemWeaponShotGun::CSE_ALifeItemWeaponShotGun	(LPCSTR caSection) : CSE_ALifeItemWeaponMagazined(caSection)
{

}

CSE_ALifeItemWeaponShotGun::~CSE_ALifeItemWeaponShotGun	()
{
}

void CSE_ALifeItemWeaponShotGun::UPDATE_Read		(NET_Packet& P)
{
	inherited::UPDATE_Read(P);
}
void CSE_ALifeItemWeaponShotGun::UPDATE_Write	(NET_Packet& P)
{
	inherited::UPDATE_Write(P);
}
void CSE_ALifeItemWeaponShotGun::STATE_Read		(NET_Packet& P, u16 size)
{
	inherited::STATE_Read(P, size);
}
void CSE_ALifeItemWeaponShotGun::STATE_Write		(NET_Packet& P)
{
	inherited::STATE_Write(P);
}

void CSE_ALifeItemWeaponShotGun::FillProps			(LPCSTR pref, PropItemVec& items)
{
	inherited::FillProps			(pref, items);
};
////////////////////////////////////////////////////////////////////////////
// CSE_ALifeItemWeaponAutoShotGun
////////////////////////////////////////////////////////////////////////////
CSE_ALifeItemWeaponAutoShotGun::CSE_ALifeItemWeaponAutoShotGun	(LPCSTR caSection) : CSE_ALifeItemWeaponShotGun(caSection)
{
}

CSE_ALifeItemWeaponAutoShotGun::~CSE_ALifeItemWeaponAutoShotGun	()
{
}

void CSE_ALifeItemWeaponAutoShotGun::UPDATE_Read	(NET_Packet& P)
{
	inherited::UPDATE_Read(P);
}
void CSE_ALifeItemWeaponAutoShotGun::UPDATE_Write	(NET_Packet& P)
{
	inherited::UPDATE_Write(P);
}
void CSE_ALifeItemWeaponAutoShotGun::STATE_Read		(NET_Packet& P, u16 size)
{
	inherited::STATE_Read(P, size);
}
void CSE_ALifeItemWeaponAutoShotGun::STATE_Write	(NET_Packet& P)
{
	inherited::STATE_Write(P);
}

void CSE_ALifeItemWeaponAutoShotGun::FillProps		(LPCSTR pref, PropItemVec& items)
{
	inherited::FillProps(pref, items);
}

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeItemWeaponMagazined
////////////////////////////////////////////////////////////////////////////
CSE_ALifeItemWeaponMagazined::CSE_ALifeItemWeaponMagazined	(LPCSTR caSection) : CSE_ALifeItemWeapon(caSection)
{
	m_u8CurFireMode = 0;

	if (pSettings->line_exist(caSection, "fire_modes"))
	{
		shared_str FireModesList = pSettings->r_string(caSection, "fire_modes");
		int ModesCount = _GetItemCount(FireModesList.c_str());
		m_u8CurFireMode = (u8)ModesCount - 1;
	}
}

CSE_ALifeItemWeaponMagazined::~CSE_ALifeItemWeaponMagazined	()
{
}

void CSE_ALifeItemWeaponMagazined::UPDATE_Read		(NET_Packet& P)
{
	inherited::UPDATE_Read(P);

	m_u8CurFireMode = P.r_u8();
}
void CSE_ALifeItemWeaponMagazined::UPDATE_Write	(NET_Packet& P)
{
	inherited::UPDATE_Write(P);

	P.w_u8(m_u8CurFireMode);
}
void CSE_ALifeItemWeaponMagazined::STATE_Read		(NET_Packet& P, u16 size)
{
	inherited::STATE_Read(P, size);
}
void CSE_ALifeItemWeaponMagazined::STATE_Write		(NET_Packet& P)
{
	inherited::STATE_Write(P);
}

void CSE_ALifeItemWeaponMagazined::FillProps			(LPCSTR pref, PropItemVec& items)
{
	inherited::FillProps			(pref, items);
};

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeItemWeaponMagazinedWGL
////////////////////////////////////////////////////////////////////////////
CSE_ALifeItemWeaponMagazinedWGL::CSE_ALifeItemWeaponMagazinedWGL	(LPCSTR caSection) : CSE_ALifeItemWeaponMagazined(caSection)
{
	grenadeMode_server = 0;
	grndIsLoaded_ = 0;
	grndID_ = 0;
}

CSE_ALifeItemWeaponMagazinedWGL::~CSE_ALifeItemWeaponMagazinedWGL	()
{
}

void CSE_ALifeItemWeaponMagazinedWGL::UPDATE_Read(NET_Packet& P)
{
	inherited::UPDATE_Read(P);

	grenadeMode_server = !!P.r_u8();
	grndIsLoaded_ = P.r_u8();
	grndID_ = P.r_u8();

}

void CSE_ALifeItemWeaponMagazinedWGL::UPDATE_Write(NET_Packet& P)
{
	inherited::UPDATE_Write(P);

	P.w_u8(grenadeMode_server ? 1 : 0);
	P.w_u8(grndIsLoaded_);
	P.w_u8(grndID_);

}

void CSE_ALifeItemWeaponMagazinedWGL::STATE_Read		(NET_Packet& P, u16 size)
{
	inherited::STATE_Read(P, size);
}

void CSE_ALifeItemWeaponMagazinedWGL::STATE_Write		(NET_Packet& P)
{
	inherited::STATE_Write(P);
}

void CSE_ALifeItemWeaponMagazinedWGL::FillProps			(LPCSTR pref, PropItemVec& items)
{
	inherited::FillProps			(pref, items);
};

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeItemAmmo
////////////////////////////////////////////////////////////////////////////
CSE_ALifeItemAmmo::CSE_ALifeItemAmmo		(LPCSTR caSection) : CSE_ALifeItem(caSection)
{
	a_elapsed					= m_boxSize = (u16)pSettings->r_s32(caSection, "box_size");
	if (pSettings->section_exist(caSection) && pSettings->line_exist(caSection,"visual"))
        set_visual				(pSettings->r_string(caSection,"visual"));
}

CSE_ALifeItemAmmo::~CSE_ALifeItemAmmo		()
{
}

void CSE_ALifeItemAmmo::STATE_Read			(NET_Packet	&tNetPacket, u16 size)
{
	inherited::STATE_Read		(tNetPacket,size);
	tNetPacket.r_u16			(a_elapsed);
}

void CSE_ALifeItemAmmo::STATE_Write			(NET_Packet	&tNetPacket)
{
	inherited::STATE_Write		(tNetPacket);
	tNetPacket.w_u16			(a_elapsed);
}

void CSE_ALifeItemAmmo::UPDATE_Read			(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Read		(tNetPacket);

	tNetPacket.r_u16			(a_elapsed);
}

void CSE_ALifeItemAmmo::UPDATE_Write		(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Write		(tNetPacket);

	tNetPacket.w_u16			(a_elapsed);
}

u16 CSE_ALifeItemAmmo::get_ammo_left		() const
{
	return a_elapsed;
}

void CSE_ALifeItemAmmo::FillProps			(LPCSTR pref, PropItemVec& values) {
  	inherited::FillProps			(pref,values);
	PHelper().CreateU16			(values, PrepareKey(pref, *s_name, "Ammo: left").c_str(), &a_elapsed, 0, m_boxSize, m_boxSize);
}

bool CSE_ALifeItemAmmo::can_switch_online	() const
{
	return inherited::can_switch_online();
}

bool CSE_ALifeItemAmmo::can_switch_offline	() const
{
	return ( inherited::can_switch_offline() && a_elapsed!=0 );
}

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeItemDetector
////////////////////////////////////////////////////////////////////////////
CSE_ALifeItemDetector::CSE_ALifeItemDetector(LPCSTR caSection) : CSE_ALifeItem(caSection)
{
	m_ef_detector_type	= pSettings->r_u32(caSection,"ef_detector_type");
}

CSE_ALifeItemDetector::~CSE_ALifeItemDetector()
{
}

u32	 CSE_ALifeItemDetector::ef_detector_type	() const
{
	return	(m_ef_detector_type);
}

void CSE_ALifeItemDetector::STATE_Read		(NET_Packet	&tNetPacket, u16 size)
{
	if (m_wVersion > 20)
		inherited::STATE_Read	(tNetPacket,size);
}

void CSE_ALifeItemDetector::STATE_Write		(NET_Packet	&tNetPacket)
{
	inherited::STATE_Write		(tNetPacket);
}

void CSE_ALifeItemDetector::UPDATE_Read		(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Read		(tNetPacket);
}

void CSE_ALifeItemDetector::UPDATE_Write	(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Write		(tNetPacket);
}

void CSE_ALifeItemDetector::FillProps		(LPCSTR pref, PropItemVec& items)
{
  	inherited::FillProps			(pref,items);
}

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeItemDetector
////////////////////////////////////////////////////////////////////////////
CSE_ALifeItemArtefact::CSE_ALifeItemArtefact(LPCSTR caSection) : CSE_ALifeItem(caSection)
{
	m_fAnomalyValue				= 100.f;
	serverOwningAnomalyID_		= NEW_INVALID_OBJECT_ID;
}

CSE_ALifeItemArtefact::~CSE_ALifeItemArtefact()
{
}

void CSE_ALifeItemArtefact::STATE_Read		(NET_Packet	&tNetPacket, u16 size)
{
	inherited::STATE_Read		(tNetPacket, size);
}

void CSE_ALifeItemArtefact::STATE_Write		(NET_Packet	&tNetPacket)
{
	inherited::STATE_Write		(tNetPacket);
}

void CSE_ALifeItemArtefact::UPDATE_Read		(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Read		(tNetPacket);
	serverOwningAnomalyID_ = tNetPacket.r_u32();
}

void CSE_ALifeItemArtefact::UPDATE_Write	(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Write		(tNetPacket);
	tNetPacket.w_u32(serverOwningAnomalyID_);
}

void CSE_ALifeItemArtefact::FillProps		(LPCSTR pref, PropItemVec& items)
{
	inherited::FillProps			(pref, items);
	PHelper().CreateFloat			(items, PrepareKey(pref, *s_name, "Anomaly value:").c_str(), &m_fAnomalyValue, 0.f, 200.f);
}

BOOL CSE_ALifeItemArtefact::NeedDataExport	()
{
	if (base()->ID_Parent == u16(-1))
		return TRUE;

	return FALSE;
}

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeItemPDA
////////////////////////////////////////////////////////////////////////////
CSE_ALifeItemPDA::CSE_ALifeItemPDA		(LPCSTR caSection) : CSE_ALifeItem(caSection)
{
	m_original_owner		= 0xffff;
	m_specific_character	= NULL;
	m_info_portion			= NULL;
}


CSE_ALifeItemPDA::~CSE_ALifeItemPDA		()
{
}

void CSE_ALifeItemPDA::STATE_Read		(NET_Packet	&tNetPacket, u16 size)
{
	inherited::STATE_Read		(tNetPacket,size);
	if (m_wVersion > 58)
		tNetPacket.r_u16		(m_original_owner);

	if (m_wVersion > 89)

	if ( (m_wVersion > 89)&&(m_wVersion < 98)  )
	{
		int tmp,tmp2;
		tNetPacket.r			(&tmp,		sizeof(int));
		tNetPacket.r			(&tmp2,		sizeof(int));
		m_info_portion			=	NULL;
		m_specific_character	= NULL;
	}else{
		tNetPacket.r_stringZ	(m_specific_character);
		tNetPacket.r_stringZ	(m_info_portion);
	
	}
}

void CSE_ALifeItemPDA::STATE_Write		(NET_Packet	&tNetPacket)
{
	inherited::STATE_Write		(tNetPacket);
	tNetPacket.w_u16				(m_original_owner);
#ifdef XRGAME_EXPORTS
	tNetPacket.w_stringZ		(m_specific_character);
	tNetPacket.w_stringZ		(m_info_portion);
#else
	shared_str		tmp_1	= NULL;
	shared_str						tmp_2	= NULL;

	tNetPacket.w_stringZ		(tmp_1);
	tNetPacket.w_stringZ		(tmp_2);
#endif

}

void CSE_ALifeItemPDA::UPDATE_Read		(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Read		(tNetPacket);
}

void CSE_ALifeItemPDA::UPDATE_Write	(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Write		(tNetPacket);
}

void CSE_ALifeItemPDA::FillProps		(LPCSTR pref, PropItemVec& items)
{
	inherited::FillProps			(pref,items);
}

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeItemDocument
////////////////////////////////////////////////////////////////////////////
CSE_ALifeItemDocument::CSE_ALifeItemDocument(LPCSTR caSection): CSE_ALifeItem(caSection)
{
	if (pSettings->line_exist(caSection, "info_portion"))
		m_wDoc					= pSettings->r_string(caSection,"info_portion");
	else
		m_wDoc					= NULL;
}

CSE_ALifeItemDocument::~CSE_ALifeItemDocument()
{
}

void CSE_ALifeItemDocument::STATE_Read		(NET_Packet	&tNetPacket, u16 size)
{
	inherited::STATE_Read		(tNetPacket,size);

	if ( m_wVersion < 98  ){
		u16 tmp;
		tNetPacket.r_u16			(tmp);
		m_wDoc = NULL;
	}else
		tNetPacket.r_stringZ		(m_wDoc);
}

void CSE_ALifeItemDocument::STATE_Write		(NET_Packet	&tNetPacket)
{
	inherited::STATE_Write		(tNetPacket);
	tNetPacket.w_stringZ		(m_wDoc);
}

void CSE_ALifeItemDocument::UPDATE_Read		(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Read		(tNetPacket);
}

void CSE_ALifeItemDocument::UPDATE_Write	(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Write		(tNetPacket);
}

void CSE_ALifeItemDocument::FillProps		(LPCSTR pref, PropItemVec& items)
{
	inherited::FillProps			(pref,items);
//	PHelper().CreateU16			(items, PrepareKey(pref, *s_name, "Document index :"), &m_wDocIndex, 0, 65535);
	PHelper().CreateRText		(items, PrepareKey(pref, *s_name, "Info portion :").c_str(), &m_wDoc);
}

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeItemGrenade
////////////////////////////////////////////////////////////////////////////
CSE_ALifeItemGrenade::CSE_ALifeItemGrenade	(LPCSTR caSection): CSE_ALifeItem(caSection)
{
	m_ef_weapon_type	= READ_IF_EXISTS(pSettings,r_u32,caSection,"ef_weapon_type",u32(-1));
}

CSE_ALifeItemGrenade::~CSE_ALifeItemGrenade	()
{
}

u32	CSE_ALifeItemGrenade::ef_weapon_type() const
{
	VERIFY	(m_ef_weapon_type != u32(-1));
	return	(m_ef_weapon_type);
}

void CSE_ALifeItemGrenade::STATE_Read		(NET_Packet	&tNetPacket, u16 size)
{
	inherited::STATE_Read		(tNetPacket,size);
}

void CSE_ALifeItemGrenade::STATE_Write		(NET_Packet	&tNetPacket)
{
	inherited::STATE_Write		(tNetPacket);
}

void CSE_ALifeItemGrenade::UPDATE_Read		(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Read		(tNetPacket);
}

void CSE_ALifeItemGrenade::UPDATE_Write		(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Write		(tNetPacket);
}

void CSE_ALifeItemGrenade::FillProps			(LPCSTR pref, PropItemVec& items)
{
	inherited::FillProps			(pref,items);
}

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeItemExplosive
////////////////////////////////////////////////////////////////////////////
CSE_ALifeItemExplosive::CSE_ALifeItemExplosive	(LPCSTR caSection): CSE_ALifeItem(caSection)
{
}

CSE_ALifeItemExplosive::~CSE_ALifeItemExplosive	()
{
}

void CSE_ALifeItemExplosive::STATE_Read		(NET_Packet	&tNetPacket, u16 size)
{
	inherited::STATE_Read		(tNetPacket,size);
}

void CSE_ALifeItemExplosive::STATE_Write		(NET_Packet	&tNetPacket)
{
	inherited::STATE_Write		(tNetPacket);
}

void CSE_ALifeItemExplosive::UPDATE_Read		(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Read		(tNetPacket);
}

void CSE_ALifeItemExplosive::UPDATE_Write		(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Write		(tNetPacket);
}

void CSE_ALifeItemExplosive::FillProps			(LPCSTR pref, PropItemVec& items)
{
	inherited::FillProps			(pref,items);
}

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeItemBolt
////////////////////////////////////////////////////////////////////////////
CSE_ALifeItemBolt::CSE_ALifeItemBolt		(LPCSTR caSection) : CSE_ALifeItem(caSection)
{
	m_flags.set					(flUseSwitches,FALSE);
	m_flags.set					(flSwitchOffline,FALSE);
	m_ef_weapon_type			= READ_IF_EXISTS(pSettings,r_u32,caSection,"ef_weapon_type",u32(-1));
}

CSE_ALifeItemBolt::~CSE_ALifeItemBolt		()
{
}

u32	CSE_ALifeItemBolt::ef_weapon_type() const
{
	VERIFY	(m_ef_weapon_type != u32(-1));
	return	(m_ef_weapon_type);
}

void CSE_ALifeItemBolt::STATE_Write			(NET_Packet &tNetPacket)
{
	inherited::STATE_Write		(tNetPacket);
}

void CSE_ALifeItemBolt::STATE_Read			(NET_Packet &tNetPacket, u16 size)
{
	inherited::STATE_Read		(tNetPacket, size);
}

void CSE_ALifeItemBolt::UPDATE_Write		(NET_Packet &tNetPacket)
{
	inherited::UPDATE_Write	(tNetPacket);
};

void CSE_ALifeItemBolt::UPDATE_Read			(NET_Packet &tNetPacket)
{
	inherited::UPDATE_Read		(tNetPacket);
};

bool CSE_ALifeItemBolt::can_save			() const
{
	return						(false);//!attached());
}
bool CSE_ALifeItemBolt::used_ai_locations		() const
{
	return false;
}
void CSE_ALifeItemBolt::FillProps			(LPCSTR pref, PropItemVec& values)
{
	inherited::FillProps			(pref,	 values);
}

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeItemCustomOutfit
////////////////////////////////////////////////////////////////////////////
CSE_ALifeItemCustomOutfit::CSE_ALifeItemCustomOutfit	(LPCSTR caSection): CSE_ALifeItem(caSection)
{
	m_ef_equipment_type		= pSettings->r_u32(caSection,"ef_equipment_type");
}

CSE_ALifeItemCustomOutfit::~CSE_ALifeItemCustomOutfit	()
{
}

u32	CSE_ALifeItemCustomOutfit::ef_equipment_type		() const
{
	return			(m_ef_equipment_type);
}

void CSE_ALifeItemCustomOutfit::STATE_Read		(NET_Packet	&tNetPacket, u16 size)
{
	inherited::STATE_Read		(tNetPacket,size);
}

void CSE_ALifeItemCustomOutfit::STATE_Write		(NET_Packet	&tNetPacket)
{
	inherited::STATE_Write		(tNetPacket);
}

void CSE_ALifeItemCustomOutfit::UPDATE_Read(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Read(tNetPacket);
}

void CSE_ALifeItemCustomOutfit::UPDATE_Write(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Write(tNetPacket);
}

void CSE_ALifeItemCustomOutfit::FillProps			(LPCSTR pref, PropItemVec& items)
{
	inherited::FillProps			(pref,items);
}

BOOL CSE_ALifeItemCustomOutfit::NeedDataExport		()
{
	return							(true);
}


////////////////////////////////////////////////////////////////////////////
// CSE_ALifeEatableItem
////////////////////////////////////////////////////////////////////////////
CSE_ALifeEatableItem::CSE_ALifeEatableItem(LPCSTR caSection) : CSE_ALifeItem(caSection)
{
	numOfPortionsServer_ = READ_IF_EXISTS(pSettings, r_u16, caSection, "eat_portions_num", 1);
	initialNumOfPortionsServer_ = numOfPortionsServer_;
}

CSE_ALifeEatableItem::~CSE_ALifeEatableItem()
{
}

void CSE_ALifeEatableItem::STATE_Read(NET_Packet	&tNetPacket, u16 size)
{
	inherited::STATE_Read(tNetPacket, size);
}

void CSE_ALifeEatableItem::STATE_Write(NET_Packet	&tNetPacket)
{
	inherited::STATE_Write(tNetPacket);
}

void CSE_ALifeEatableItem::UPDATE_Read(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Read(tNetPacket);

	tNetPacket.r_u16(numOfPortionsServer_);
}

void CSE_ALifeEatableItem::UPDATE_Write(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Write(tNetPacket);

	tNetPacket.w_u16(numOfPortionsServer_);
}

void CSE_ALifeEatableItem::FillProps(LPCSTR pref, PropItemVec& values)
{
	inherited::FillProps(pref, values);

	PHelper().CreateU16(values, PrepareKey(pref, *s_name, "Portions Number:").c_str(), &numOfPortionsServer_, 0, 65535, 1); // this is for object properties config in SDK, i guess
}

////////////////////////////////////////////////////////////////////////////
// CSE_ALifeItemHelmet
////////////////////////////////////////////////////////////////////////////
CSE_ALifeItemHelmet::CSE_ALifeItemHelmet(LPCSTR caSection) : CSE_ALifeItem(caSection)
{
}

CSE_ALifeItemHelmet::~CSE_ALifeItemHelmet()
{
}

void CSE_ALifeItemHelmet::STATE_Read(NET_Packet	&tNetPacket, u16 size)
{
	inherited::STATE_Read(tNetPacket, size);
}

void CSE_ALifeItemHelmet::STATE_Write(NET_Packet	&tNetPacket)
{
	inherited::STATE_Write(tNetPacket);
}

void CSE_ALifeItemHelmet::UPDATE_Read(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Read(tNetPacket);
}

void CSE_ALifeItemHelmet::UPDATE_Write(NET_Packet	&tNetPacket)
{
	inherited::UPDATE_Write(tNetPacket);
}

void CSE_ALifeItemHelmet::FillProps(LPCSTR pref, PropItemVec& items)
{
	inherited::FillProps(pref, items);
}

BOOL CSE_ALifeItemHelmet::NeedDataExport()
{
	return							(true);
}