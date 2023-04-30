////////////////////////////////////////////////////////////////////////////
//	Module 		: inventory_item.cpp
//	Created 	: 24.03.2003
//  Modified 	: 29.01.2004
//	Author		: Victor Reutsky, Yuri Dobronravin
//	Description : Inventory item
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "inventory_item.h"
#include "inventory.h"
#include "xrserver_objects_alife.h"
#include "xrserver_objects_alife_items.h"
#include "entity_alive.h"
#include "Level.h"
#include "Actor.h"
#include "string_table.h"
#include "../Include/xrRender/Kinematics.h"
#include "ai_object_location.h"
#include "object_broker.h"
#include "inventory_item_impl.h"
#ifdef DEBUG
#	include "debug_renderer.h"
#endif

CInventoryItem::CInventoryItem() 
{
	m_baseSlot			= /*m_currentSlot =*/ NO_ACTIVE_SLOT;
	m_flags.set			(Fbelt,FALSE);
	m_flags.set			(FArtefactbelt,FALSE);
	m_flags.set			(Fruck,TRUE);
	m_flags.set			(FRuckDefault,TRUE);
	m_pCurrentInventory	= NULL;

	SetDropManual		(FALSE);
	SetDeleteManual		(FALSE);

	m_flags.set			(FCanTake,TRUE);
	m_flags.set			(FCanTrade,TRUE);
	m_flags.set			(FUsingCondition,FALSE);
	condition_			= 1.0f;

	m_name = m_nameShort = NULL;

	m_eItemPlace		= eItemPlaceUndefined;
	m_currentSlot		= NO_ACTIVE_SLOT;
	m_Description		= "";
	m_section_id		= 0;
	m_bPickUpVisible	= true;
	m_bTradeIgnoreCondition = false;
	m_fConditionCostKoef = 0.9f;
	m_fConditionCostCurve = 0.75f;

	needKnifeToLoot_	= false;
	inventoryCategory_	= icMisc;
}

CInventoryItem::~CInventoryItem() 
{
	bool B_GOOD			= (	!m_pCurrentInventory || 
		(std::find(m_pCurrentInventory->allContainer_.begin(), m_pCurrentInventory->allContainer_.end(), this) == m_pCurrentInventory->allContainer_.end()));
	if(!B_GOOD)
	{
		CObject* p	= object().H_Parent();
		Msg("inventory ptr is [%s]",m_pCurrentInventory?"not-null":"null");
		if(p)
			Msg("parent name is [%s]",p->ObjectName().c_str());

			Msg("! ERROR item_id[%d] H_Parent=[%s][%d] [%d]",
				object().ID(),
				p ? p->ObjectName().c_str() : "none",
				p ? p->ID() : -1,
				CurrentFrame());
	}
}

void CInventoryItem::LoadCfg(LPCSTR section) 
{
	CHitImmunity::LoadImmunities	(pSettings->r_string(section,"immunities_sect"),pSettings);

	ISpatial*			self				=	smart_cast<ISpatial*> (this);
	if (self)			self->spatial.s_type |=	STYPE_VISIBLEFORAI;

	m_section_id._set	(section);
	m_name				= CStringTable().translate( pSettings->r_string(section, "inv_name") );
	m_nameShort			= CStringTable().translate( pSettings->r_string(section, "inv_name_short"));

//.	NameComplex			();
	m_weight			= pSettings->r_float(section, "inv_weight");
	R_ASSERT			(m_weight>=0.f);

	m_cost				= pSettings->r_u32(section, "cost");

	u8 sl = READ_IF_EXISTS(pSettings, r_u8, section, "slot", -1);

	m_baseSlot			= (sl == -1) ? 0 : (sl + 1);

	// Description
	if ( pSettings->line_exist(section, "description") )
		m_Description = CStringTable().translate( pSettings->r_string(section, "description") );

	m_flags.set(Fbelt,			READ_IF_EXISTS(pSettings, r_bool, section, "belt",				FALSE));
	m_flags.set(FArtefactbelt,	READ_IF_EXISTS(pSettings, r_bool, section, "artefact_belt",		FALSE));

	R_ASSERT2(!m_flags.test(Fbelt) || !m_flags.test(FArtefactbelt), make_string("An item with name %s and section %s has both targed belts (belt = true and artefact_belt = true). That's not supported", m_name.c_str(), section)); // make sure the item is only for one belt type.

	m_flags.set(FRuckDefault,	READ_IF_EXISTS(pSettings, r_bool, section, "default_to_ruck",	TRUE));

	m_flags.set(FCanTake,		READ_IF_EXISTS(pSettings, r_bool, section, "can_take",			TRUE));
	m_flags.set(FCanTrade,		READ_IF_EXISTS(pSettings, r_bool, section, "can_trade",			TRUE));
	m_flags.set(FIsQuestItem,	READ_IF_EXISTS(pSettings, r_bool, section, "quest_item",		FALSE));

	m_flags.set					(FAllowSprint,READ_IF_EXISTS	(pSettings, r_bool, section,"sprint_allowed",			TRUE));
	m_fControlInertionFactor	= READ_IF_EXISTS(pSettings, r_float,section,"control_inertion_factor",	1.0f);
	m_icon_name					= READ_IF_EXISTS(pSettings, r_string,section,"icon_name",				NULL);

	m_bPickUpVisible  		= !!READ_IF_EXISTS(pSettings,r_bool,section,"pickup_visible",TRUE);

	m_bUniqueMarked			= !!READ_IF_EXISTS(pSettings, r_bool, section, "unique_item", FALSE);

	m_bSingleHand			= !!READ_IF_EXISTS(pSettings, r_bool, section, "single_handed", FALSE);

	m_attach_to_owner		= READ_IF_EXISTS(pSettings, r_u32, section, "attach_to_owner", 0);
	m_bTradeIgnoreCondition = !!READ_IF_EXISTS(pSettings, r_bool, section, "trade_ignore_condition", false);
	m_fConditionCostKoef	= READ_IF_EXISTS(pSettings, r_float, section, "condition_cost_koef", 0.9f);
	clamp(m_fConditionCostKoef, 0.f, 1.f);
	m_fConditionCostCurve	= READ_IF_EXISTS(pSettings, r_float, section, "condition_cost_curve", 0.75f);

	needKnifeToLoot_		= !!READ_IF_EXISTS(pSettings, r_bool, section, "need_knife_to_loot", FALSE);

	LPCSTR category			= READ_IF_EXISTS(pSettings, r_string, section, "inv_category", "misc");
	inventoryCategory_ = xr_strcmp(category, "arts") == 0 ? icArtefacts : xr_strcmp(category, "mutant_parts") == 0 ? icMutantParts : xr_strcmp(category, "ammo") == 0 ? icAmmo :
		xr_strcmp(category, "food") == 0 ? icFood : xr_strcmp(category, "outfits") == 0 ? icOutfits : xr_strcmp(category, "weapons") == 0 ? icWeapons : xr_strcmp(category, "devices") == 0 ? icDevices :
		xr_strcmp(category, "drugs") == 0 ? icMedicine : icMisc;
}

void  CInventoryItem::ChangeCondition(float fDeltaCondition)
{
	condition_ += fDeltaCondition;
	clamp(condition_, 0.f, 1.f);
}

void  CInventoryItem::SetCondition(float fCondition)
{
	condition_ = fCondition;
	clamp(condition_, 0.f, 1.f);
}

void  CInventoryItem::ResetCondition()
{
	condition_ = 1.f;
}

void CInventoryItem::SetCurrPlace(EItemPlace place)
{
	if (place != eItemPlaceSlot)
	{
		m_currentSlot = NO_ACTIVE_SLOT;
	}
	m_eItemPlace = place;
}

void CInventoryItem::SetCurrSlot(TSlotId slot)
{
	m_currentSlot = slot;
}

void	CInventoryItem::Hit					(SHit* pHDS)
{
	if( !m_flags.test(FUsingCondition) ) return;

	float hit_power = pHDS->damage();
	hit_power *= GetHitImmunity(pHDS->hit_type);

	ChangeCondition(-hit_power);
}

const char* CInventoryItem::Name() 
{
	return *m_name;
}

const char* CInventoryItem::NameShort() 
{
	return *m_nameShort;
}

bool CInventoryItem::Useful() const
{
	return CanTake();
}

void CInventoryItem::BeforeDetachFromParent(bool just_before_destroy)
{
	UpdateXForm();
	SetCurrPlace(eItemPlaceUndefined);
}

void CInventoryItem::AfterDetachFromParent()
{
	SetCurrPlace(eItemPlaceUndefined);
	inherited::AfterDetachFromParent();
}

void CInventoryItem::BeforeAttachToParent()
{
}

void CInventoryItem::AfterAttachToParent()
{
	inherited::AfterAttachToParent();
}
#ifdef DEBUG
extern	Flags32	dbg_net_Draw_Flags;
#endif

void CInventoryItem::UpdateCL()
{
#ifdef DEBUG
	if(bDebug){
		if (dbg_net_Draw_Flags.test(1<<4) )
		{
			Device.seqRender.Remove(this);
			Device.seqRender.Add(this);
		}else
		{
			Device.seqRender.Remove(this);
		}
	}

#endif
}

void CInventoryItem::OnEvent (NET_Packet& P, u16 type)
{

}

void CInventoryItem::ChangePosition(Fvector& new_pos)
{
	CPHSynchronize* pSyncObj = NULL;
	pSyncObj = object().PHGetSyncItem(0);
	if (!pSyncObj) return;
	SPHNetState state;
	pSyncObj->get_State(state);

	state.position = new_pos;
	state.previous_position = new_pos;

	pSyncObj->set_State(state);
}

//процесс отсоединения вещи заключается в спауне новой вещи 
//в инвентаре и установке соответствующих флагов в родительском
//объекте, поэтому функция должна быть переопределена
bool CInventoryItem::Detach(const char* item_section_name, bool b_spawn_item) 
{
	if(b_spawn_item)
	{
		CSE_Abstract*		D	= F_entity_Create(item_section_name);
		R_ASSERT		   (D);
		CSE_ALifeDynamicObject	*l_tpALifeDynamicObject = 
								smart_cast<CSE_ALifeDynamicObject*>(D);
		R_ASSERT			(l_tpALifeDynamicObject);
		
		l_tpALifeDynamicObject->m_tNodeID = object().ai_location().level_vertex_id();
			
		// Fill
		D->s_name			=	item_section_name;
		D->set_name_replace	("");
		D->s_RP				=	0xff;
		D->ID				=	0xffff;

		D->ID_Parent		=	u16(object().H_Parent()->ID());

		D->ID_Phantom		=	0xffff;
		D->o_Position		=	object().Position();
		D->s_flags.assign	(M_SPAWN_OBJECT_LOCAL);
		D->RespawnTime		=	0;
		// Send
		NET_Packet			P;
		D->Spawn_Write		(P,TRUE);
		Level().Send		(P,net_flags(TRUE));
		// Destroy
		F_entity_Destroy	(D);
	}
	return true;
}

/////////// network ///////////////////////////////
BOOL CInventoryItem::SpawnAndImportSOData(CSE_Abstract* data_containing_so)
{
//	m_bInInterpolation				= false;
//	m_bInterpolate					= false;

	m_flags.set						(Fuseful_for_NPC, TRUE);
	CSE_Abstract					*e	= (CSE_Abstract*)(data_containing_so);
	CSE_ALifeObject					*alife_object = smart_cast<CSE_ALifeObject*>(e);
	if (alife_object)	{
		m_flags.set(Fuseful_for_NPC, alife_object->m_flags.test(CSE_ALifeObject::flUsefulForAI));
	}

	CSE_ALifeInventoryItem			*pSE_InventoryItem = smart_cast<CSE_ALifeInventoryItem*>(e);
	if (!pSE_InventoryItem)			return TRUE;

	//import variable values from server class, which were exported before
	condition_ = pSE_InventoryItem->m_fCondition;

	ImportInstalledUbgrades(pSE_InventoryItem->m_upgrades);
	
	m_just_after_spawn		= true;
	m_activated				= false;

	return TRUE;
}

void CInventoryItem::DestroyClientObj()
{
	if(m_pCurrentInventory){
		VERIFY(std::find(m_pCurrentInventory->allContainer_.begin(), m_pCurrentInventory->allContainer_.end(), this) == m_pCurrentInventory->allContainer_.end());
	}

	//инвентарь которому мы принадлежали
//.	m_pCurrentInventory = NULL;
}

void CInventoryItem::ExportDataToServer(NET_Packet& P)
{
	P.w_float_q8(condition_, 0.0f, 1.0f);
};

void CInventoryItem::save(NET_Packet &packet)
{
	SInvItemPlace place;
	place.type = m_eItemPlace;
	place.slot_id = m_currentSlot;
	//if (m_eItemPlace == eItemPlaceSlot) Msg("Saving item %s, slot: %d", object().SectionNameStr(), m_currentSlot);
	packet.w_u8				(place.value);

	if (object().H_Parent()) {
		packet.w_u8			(0);
		return;
	}

	u8 _num_items			= (u8)object().PHGetSyncItemsNumber(); 
	packet.w_u8				(_num_items);
	object().PHSaveState	(packet);
}

void CInventoryItem::load(IReader &packet)
{
	SInvItemPlace place;
	place.value = packet.r_u8();
	m_eItemPlace = (EItemPlace)place.type;
	if (m_eItemPlace == eItemPlaceSlot)
	{
		//Msg("Loading item %s with slot: %d", object().SectionNameStr(), place.slot_id);
		m_currentSlot = (place.slot_id == NO_ACTIVE_SLOT)
			? m_baseSlot
			: (TSlotId)place.slot_id;
	}
	else
	{
		m_currentSlot = NO_ACTIVE_SLOT;
	}

	u8						tmp = packet.r_u8();
	if (!tmp)
		return;

	if (!object().PPhysicsShell())
	{
		object().setup_physic_shell();
		object().PPhysicsShell()->Disable();
	}

	object().PHLoadState(packet);
	object().PPhysicsShell()->Disable();
}

void CInventoryItem::reload		(LPCSTR section)
{
	inherited::reload		(section);
	m_holder_range_modifier	= READ_IF_EXISTS(pSettings,r_float,section,"holder_range_modifier",1.f);
	m_holder_fov_modifier	= READ_IF_EXISTS(pSettings,r_float,section,"holder_fov_modifier",1.f);
}

void CInventoryItem::reinit		()
{
	m_pCurrentInventory	= NULL;
	SetCurrPlace(eItemPlaceUndefined);
}

bool CInventoryItem::can_kill			() const
{
	return				(false);
}

CInventoryItem *CInventoryItem::can_kill	(CInventory *inventory) const
{
	return				(0);
}

const CInventoryItem *CInventoryItem::can_kill			(const xr_vector<const CGameObject*> &items) const
{
	return				(0);
}

CInventoryItem *CInventoryItem::can_make_killing	(const CInventory *inventory) const
{
	return				(0);
}

bool CInventoryItem::ready_to_kill		() const
{
	return				(false);
}

void CInventoryItem::activate_physic_shell()
{
	CEntityAlive*	E		= smart_cast<CEntityAlive*>(object().H_Parent());
	if (!E) {
		on_activate_physic_shell();
		return;
	};

	UpdateXForm();

	object().CPhysicsShellHolder::activate_physic_shell();
}

void CInventoryItem::UpdateXForm	()
{
	if (0==object().H_Parent())	return;

	// Get access to entity and its visual
	CEntityAlive*	E		= smart_cast<CEntityAlive*>(object().H_Parent());
	if (!E) return;
	
	if (E->cast_base_monster()) return;

	const CInventoryOwner	*parent = smart_cast<const CInventoryOwner*>(E);
	if (parent && parent->use_simplified_visual())
		return;

	if (parent->attached(this))
		return;

	R_ASSERT		(E);
	IKinematics*	V		= smart_cast<IKinematics*>	(E->Visual());
	VERIFY			(V);

	// Get matrices
	int						boneL = -1, boneR = -1, boneR2 = -1;
	E->g_WeaponBones(boneL,boneR,boneR2);
	if ((boneR == -1) || (boneL == -1)) return;
	//	if ((HandDependence() == hd1Hand) || (STATE == eReload) || (!E->g_Alive()))
	//		boneL = boneR2;

	V->NeedToCalcBones();

	Fmatrix& mL			= V->LL_GetTransform(u16(boneL));
	Fmatrix& mR			= V->LL_GetTransform(u16(boneR));
	// Calculate
	Fmatrix			mRes;
	Fvector			R,D,N;
	D.sub			(mL.c,mR.c);	D.normalize_safe();

	if(fis_zero(D.magnitude()))
	{
		mRes.set(E->XFORM());
		mRes.c.set(mR.c);
	}
	else
	{		
		D.normalize();
		R.crossproduct	(mR.j,D);

		N.crossproduct	(D,R);
		N.normalize();

		mRes.set		(R,N,D,mR.c);
		mRes.mulA_43	(E->XFORM());
	}

	//	UpdatePosition	(mRes);
	object().Position().set(mRes.c);
}



#ifdef DEBUG

void CInventoryItem::OnRender()
{
	if (Render->currentViewPort != MAIN_VIEWPORT)
		return;
	
	if (bDebug && object().Visual())
	{
		if (!(dbg_net_Draw_Flags.is_any((1<<4)))) return;

		Fvector bc,bd; 
		object().Visual()->getVisData().box.get_CD	(bc,bd);
		Fmatrix	M = object().XFORM();
		M.c.add (bc);
		Level().debug_renderer().draw_obb			(M,bd,color_rgba(0,0,255,255));
	};
}
#endif

DLL_Pure *CInventoryItem::_construct	()
{
	m_object	= smart_cast<CPhysicsShellHolder*>(this);
	VERIFY		(m_object);
	return		(inherited::_construct());
}

void CInventoryItem::modify_holder_params	(float &range, float &fov) const
{
	range		*= m_holder_range_modifier;
	fov			*= m_holder_fov_modifier;
}

bool	CInventoryItem::CanTrade() const 
{
	bool res = true;

	if(m_pCurrentInventory)
		res = inventory_owner().AllowItemToTrade(this,m_eItemPlace);

	return (res && m_flags.test(FCanTrade) && !IsQuestItem());
}

int  CInventoryItem::GetGridWidth			() const 
{
	return pSettings->r_u32(m_object->SectionName(), "inv_grid_width");
}

int  CInventoryItem::GetGridHeight			() const 
{
	return pSettings->r_u32(m_object->SectionName(), "inv_grid_height");
}
int  CInventoryItem::GetXPos				() const 
{
	return pSettings->r_u32(m_object->SectionName(), "inv_grid_x");
}
int  CInventoryItem::GetYPos				() const 
{
	return pSettings->r_u32(m_object->SectionName(), "inv_grid_y");
}

Irect CInventoryItem::GetInvGridRect() const
{
	u32 x,y,w,h;

	x = pSettings->r_u32(m_object->SectionName(),"inv_grid_x");
	y = pSettings->r_u32(m_object->SectionName(),"inv_grid_y");
	w = pSettings->r_u32(m_object->SectionName(),"inv_grid_width");
	h = pSettings->r_u32(m_object->SectionName(),"inv_grid_height");

	return Irect().set(x,y,w,h);
}

Irect CInventoryItem::GetUpgrIconRect() const
{
	u32 x,y,w,h;

	x = READ_IF_EXISTS(pSettings,r_u32,m_object->SectionName(),"upgr_icon_x", 0);
	y = READ_IF_EXISTS(pSettings,r_u32,m_object->SectionName(),"upgr_icon_y", 0);
	w = READ_IF_EXISTS(pSettings,r_u32,m_object->SectionName(),"upgr_icon_width", 0);
	h = READ_IF_EXISTS(pSettings,r_u32,m_object->SectionName(),"upgr_icon_height", 0);

	return Irect().set(x,y,w,h);
}

LPCSTR CInventoryItem::GetUpgrIconName() const
{
	LPCSTR return_string = READ_IF_EXISTS(pSettings, r_string, m_object->SectionName(), "upgr_icon", "");

	return return_string;
}

bool CInventoryItem::IsNecessaryItem(CInventoryItem* item)		
{
	return IsNecessaryItem(item->object().SectionName());
};

BOOL CInventoryItem::IsInvalid() const
{
	return object().getDestroy() || GetDropManual();
}
