#include "pch_script.h"
#include "inventory.h"
#include "actor.h"
#include "weapon.h"

#include "ui/UIInventoryUtilities.h"

#include "eatable_item.h"
#include "script_engine.h"
#include "xrmessages.h"
#include "xr_level_controller.h"
#include "level.h"
#include "ai_space.h"
#include "clsid_game.h"
#include "ai/stalker/ai_stalker.h"
#include "weaponmagazined.h"
#include "game_object_space.h"
#include "script_callback_ex.h"
#include "script_game_object.h"
#include "CustomOutfit.h"
#include "UIGameCustom.h"
#include "UI/UIInventoryWnd.h"
#include "Artifact.h"
#include "alife_simulator.h"

using namespace InventoryUtilities;

// what to block
u32	INV_STATE_BLOCK_ALL		= 0xffffffff;
u32	INV_STATE_CAR			= INV_STATE_BLOCK_ALL ^ (1 << PISTOL_SLOT);
u32	INV_STATE_LADDER		= (1 << RIFLE_SLOT) | (1 << RIFLE_2_SLOT) | (1 << DETECTOR_SLOT);
u32	INV_STATE_INV_WND		= INV_STATE_BLOCK_ALL;

CInventorySlot::CInventorySlot()
{
	m_pIItem				= NULL;
	canCastHud_				= true;
	m_bPersistent			= false;
	m_blockCounter			= 0;
}

CInventorySlot::~CInventorySlot()
{
}

bool CInventorySlot::CanBeActivated() const
{
	return (canCastHud_ && !IsBlocked());
};

bool CInventorySlot::IsBlocked() const
{
	return (m_blockCounter>0);
}

CInventory::CInventory()
{
	m_fTakeDist									= pSettings->r_float	("inventory", "take_dist");
	m_iMaxBelt									= pSettings->r_s32		("inventory", "base_belt_size");
	m_iMaxArtBelt								= pSettings->r_s32		("inventory", "base_art_belt_size");

	m_slots.resize								(LAST_SLOT + 1); // first is [1]

	SetCurrentDetector(NULL);
	m_iActiveSlot								= NO_ACTIVE_SLOT;
	m_iNextActiveSlot							= NO_ACTIVE_SLOT;
	m_iPrevActiveSlot							= NO_ACTIVE_SLOT;
	m_pTarget									= NULL;
	m_bHandsOnly								= false;

	string256 temp;

	for (TSlotId i = FirstSlot(); i <= LastSlot(); ++i)
	{
		xr_sprintf(temp, "slot_persistent_%d", i);

		if(pSettings->line_exist("inventory", temp))
			m_slots[i].m_bPersistent = !!pSettings->r_bool("inventory" ,temp);

		xr_sprintf(temp, "slot_casts_hud_%d", i);

		if (pSettings->line_exist("inventory", temp))
			m_slots[i].canCastHud_ = !!pSettings->r_bool("inventory", temp);
	}

	m_bSlotsUseful								= true;
	m_bBeltUseful								= false;

	m_fTotalWeight								= -1.f;
	m_dwModifyFrame								= 0;
	m_drop_last_frame							= false;
}


CInventory::~CInventory()
{
}

void CInventory::LoadCfg(LPCSTR section)
{
	if (pSettings->line_exist(section, "inv_max_weight"))
		m_fMaxWeight = pSettings->r_float(section, "inv_max_weight");
	else
		m_fMaxWeight = pSettings->r_float("inventory", "max_weight");
}

void CInventory::Clear()
{
	allContainer_.clear();
	ruck_.clear();
	belt_.clear();
	artefactBelt_.clear();

	for (TSlotId i = FirstSlot(); i <= LastSlot(); i++)
	{
		m_slots[i].m_pIItem = NULL;
	}

	m_pOwner = NULL;

	CalcTotalWeight();
	InvalidateState();
}


void CInventory::TakeItem(PIItem item_to_take, bool bNotActivate, bool strict_placement, bool duringSpawn)
{
	CInventoryItem* pIItem = item_to_take;

	VERIFY(pIItem);

	if(pIItem->m_pCurrentInventory)
	{
		Msg("! ERROR CInventory::Take but object has m_pCurrentInventory");
		Msg("! Inventory Owner is [%d]", GetOwner()->object_id());
		Msg("! Object Inventory Owner is [%d]", pIItem->m_pCurrentInventory->GetOwner()->object_id());

		CObject* p = item_to_take->object().H_Parent();
		if(p)
			Msg("! object parent is [%s] [%d]", p->ObjectName().c_str(), p->ID());
	}


	R_ASSERT(CanTakeItem(pIItem));

	pIItem->m_pCurrentInventory = this;
	pIItem->SetDropManual(FALSE);

	bool move_to_slot_anyway = false;

	if (Level().CurrentEntity())
	{
		u16 actor_id = Level().CurrentEntity()->ID();

		if (pIItem->BaseSlot() == GRENADE_SLOT && GetOwner()->object_id() != actor_id)
		{
			move_to_slot_anyway = true;
		}
	
		if (GetOwner()->object_id() == actor_id && this->m_pOwner->object_id() == actor_id) //actors inventory
		{
			CWeaponMagazined* pWeapon = smart_cast<CWeaponMagazined*>(pIItem);

			if (pWeapon && pWeapon->strapped_mode())
			{
				pWeapon->strapped_mode(false);
				MoveToRuck(pWeapon);
			}
	
		}
	}

	allContainer_.push_back(pIItem);

	if(!strict_placement)
		pIItem->SetCurrPlace(eItemPlaceUndefined);

	bool result = false;

	switch(pIItem->CurrPlace())
	{
	case eItemPlaceBelt:
		result = MoveToBelt(pIItem);
#ifdef DEBUG
		if(!result)
			Msg("cant put in belt item %s", *pIItem->object().ObjectName());
#endif
		break;


	case eItemPlaceArtBelt:
		result = MoveToArtBelt(pIItem);
#ifdef DEBUG
		if(!result)
			Msg("cant put in art belt item %s", *pIItem->object().ObjectName());
#endif
		break;


	case eItemPlaceRuck:
		result = MoveToRuck(pIItem);
#ifdef DEBUG
		if(!result)
			Msg("cant put in ruck item %s", *pIItem->object().ObjectName());
#endif
		break;


	case eItemPlaceSlot:
		result = MoveToSlot(pIItem->CurrSlot(), pIItem, bNotActivate);
#ifdef DEBUG
		if(!result)
			Msg("cant slot in ruck item %s", *pIItem->object().ObjectName());
#endif
		break;

	default:
		if (!pIItem->RuckDefault() || move_to_slot_anyway)
		{
			if (CanPutInSlot(pIItem, pIItem->BaseSlot()))
			{
				result = MoveToSlot(pIItem->BaseSlot(), pIItem, bNotActivate);
				VERIFY(result);
			}
			else if (pIItem->BaseSlot() == RIFLE_SLOT && CanPutInSlot(pIItem, RIFLE_2_SLOT))
			{
				result = MoveToSlot(RIFLE_2_SLOT, pIItem, bNotActivate);
				VERIFY(result);
			}
			else if (CanPutInBelt(pIItem))
			{
				result = MoveToBelt(pIItem);
				VERIFY(result);
			}
			else if (CanPutInArtBelt(pIItem))
			{
				result = MoveToArtBelt(pIItem);
				R_ASSERT(result);
			}
			else
			{
				result = MoveToRuck(pIItem);
				VERIFY(result);
			}
		}
		else
		{
			result = MoveToRuck(pIItem);
			VERIFY(result);
		}
	}

	m_pOwner->OnItemTake(pIItem, duringSpawn);

	CWeaponMagazined* pWeapon = smart_cast<CWeaponMagazined*>(pIItem);

	if (pWeapon)
		pWeapon->InitAddons(); //skyloader: need to do it as in CoP when UI will be ported | надо взять реализацию зум текстур из чн\зп, когда будет пересен уи из зп

	//for detecting that an artefact was taken away from anomaly and now we need to free the id of artefact from anomaly soruonding arts list
	CArtefact* artefact = smart_cast<CArtefact*>(pIItem);

	if (artefact && artefact->owningAnomalyID_ != NEW_INVALID_OBJECT_ID)
	{
		Alife()->RemoveArtefactFromAnomList(artefact->ID());
	}

	CalcTotalWeight();
	InvalidateState();

	pIItem->object().processing_deactivate();

	VERIFY(pIItem->CurrPlace() != eItemPlaceUndefined);
}

#pragma todo("Find out why game loading processes an item in inventory 3 times. MoveToSlot -> DropItem -> MoveToSlot. GE_OWNERSHIP_TAKE -> GE_OWNERSHIP_REJECT -> GE_OWNERSHIP_TAKE... Need to Fix it or remove this net event system already")

bool CInventory::DropItem(PIItem item_to_drop)
{
	CInventoryItem *pIItem = item_to_drop;

	VERIFY(pIItem);

	if(!pIItem)
		return false;

	if(pIItem->m_pCurrentInventory!=this)
	{
		Msg("ahtung !!! [%d]", CurrentFrame());
		Msg("CInventory::DropItem pIItem->m_pCurrentInventory!=this");
		Msg("this = [%d]", GetOwner()->object_id());
		Msg("pIItem->m_pCurrentInventory = [%d]", pIItem->m_pCurrentInventory->GetOwner()->object_id());
	}

	R_ASSERT(pIItem->m_pCurrentInventory);
	R_ASSERT(pIItem->m_pCurrentInventory==this);
	VERIFY(pIItem->CurrPlace() != eItemPlaceUndefined);

	pIItem->object().processing_activate();

	switch(pIItem->CurrPlace())
	{
	case eItemPlaceBelt:
	{
		R_ASSERT2(IsInBelt(pIItem), make_string("Crashed on item: %s. Probably game save is old or belt size has changed in config", pIItem->m_name.c_str()));

		belt_.erase(std::find(belt_.begin(), belt_.end(), pIItem));
		pIItem->object().processing_deactivate();
	}break;

	case eItemPlaceArtBelt:
	{
		R_ASSERT2(IsInArtBelt(pIItem), make_string("Crashed on item: %s. Probably game save is old or art belt size has changed in config", pIItem->m_name.c_str()));

		artefactBelt_.erase(std::find(artefactBelt_.begin(), artefactBelt_.end(), pIItem));
		pIItem->object().processing_deactivate();
	}break;

	case eItemPlaceRuck:
	{
		R_ASSERT(IsInRuck(pIItem));
		ruck_.erase(std::find(ruck_.begin(), ruck_.end(), pIItem));
	}break;

	case eItemPlaceSlot:
	{
		VERIFY			(IsInSlot(pIItem));

		u32 currSlot = pIItem->CurrSlot();
		if (currSlot == NO_ACTIVE_SLOT)
			return false;

		if (m_iActiveSlot == currSlot)
			ActivateSlot(NO_ACTIVE_SLOT);

		m_slots[currSlot].m_pIItem = NULL;
		pIItem->object().processing_deactivate();
	}break;

	default:
		NODEFAULT;
	};

	TIItemContainer::iterator it = std::find(allContainer_.begin(), allContainer_.end(), pIItem);

	if (it != allContainer_.end())
		allContainer_.erase(it);
	else
		Msg("! CInventory::Drop item not found in inventory!!!");

	pIItem->m_pCurrentInventory = NULL;

	CCustomOutfit* outfit_item = smart_cast<CCustomOutfit*>(pIItem);

	if(outfit_item)
		CheckBeltCapacity();

	if (CurrentGameUI()->m_InventoryMenu->IsShown())
		CurrentGameUI()->m_InventoryMenu->UpdateSlotBlockers(); //update slotblockers only

	m_pOwner->OnItemDrop(item_to_drop);

	pIItem->SetCurrPlace(eItemPlaceUndefined);

	CalcTotalWeight();
	InvalidateState();

	m_drop_last_frame = true;

	return true;
}


bool CInventory::Eat(PIItem ItemToEat)
{
	CEatableItem* eatable_item = smart_cast<CEatableItem*>(ItemToEat);

	if (!eatable_item)
		return false;

	R_ASSERT(eatable_item->m_pCurrentInventory == this);

	//устанаовить съедобна ли вещь
	CEntityAlive *entity_alive = smart_cast<CEntityAlive*>(m_pOwner);
	R_ASSERT(entity_alive);

	bool used = eatable_item->UseBy(entity_alive);

	if (!used)
		return false;

	if (Actor()->m_inventory == this)
		Actor()->callback(GameObject::eUseObject)((smart_cast<CGameObject*>(ItemToEat))->lua_game_object());

	if (eatable_item->Empty())
	{
		NET_Packet					P;
		CGameObject::u_EventGen(P, GE_OWNERSHIP_REJECT, entity_alive->ID());
		P.w_u16(ItemToEat->object().ID());
		P.w_u8(1); // send just_before_destroy flag, so physical shell does not activates and disrupts nearby objects
		CGameObject::u_EventSend(P);

		CGameObject::u_EventGen(P, GE_DESTROY, ItemToEat->object().ID());
		CGameObject::u_EventSend(P);
	}

	return true;
}


bool CInventory::MoveToSlot(TSlotId slot_id, PIItem pIItem, bool bNotActivate)
{
	VERIFY(pIItem);

	//Msg("%s Inventory::Slot id: %d, %s[%d], notActivate: %d", m_pOwner->Name(), slot_id, *pIItem->object().ObjectName(), pIItem->object().ID(), bNotActivate);
	if(ItemFromSlot(slot_id) == pIItem)
		return false;

	if (!CanPutInSlot(pIItem, slot_id))
	{
		if (m_slots[slot_id].m_pIItem == pIItem && !bNotActivate)
		{
			ActivateSlot(slot_id);
		}

		return false;
	}

	// If item was in another slot already
	auto oldSlot = pIItem->CurrSlot();

	if (oldSlot != NO_ACTIVE_SLOT && oldSlot != slot_id)
	{
		if(GetActiveSlot() == oldSlot)
			ActivateSlot(NO_ACTIVE_SLOT);

		m_slots[oldSlot].m_pIItem = NULL;
	}

	m_slots[slot_id].m_pIItem = pIItem;

	//удалить из рюкзака или пояса
	TIItemContainer::iterator it = std::find(ruck_.begin(), ruck_.end(), pIItem);

	if (ruck_.end() != it)
		ruck_.erase(it);

	it = std::find(belt_.begin(), belt_.end(), pIItem);

	if (belt_.end() != it)
		belt_.erase(it);

	it = std::find(artefactBelt_.begin(), artefactBelt_.end(), pIItem);

	if (artefactBelt_.end() != it)
		artefactBelt_.erase(it);

	if (((m_iActiveSlot == slot_id) || (m_iActiveSlot == NO_ACTIVE_SLOT) && m_iNextActiveSlot == NO_ACTIVE_SLOT) && (!bNotActivate))
		ActivateSlot(slot_id);

	m_pOwner->OnItemSlot(pIItem, pIItem->CurrPlace());

	pIItem->SetCurrPlace(eItemPlaceSlot);
	pIItem->SetCurrSlot(slot_id);
	pIItem->OnMoveToSlot();

	if (Actor()->m_inventory == this)
	{
		Actor()->callback(GameObject::eOnMoveToSlot)((smart_cast<CGameObject*>(pIItem))->lua_game_object(), slot_id - 1);

		// Disable NV device, since we are putting on suit/helmet
		COutfitBase* outfit = smart_cast<COutfitBase*>(pIItem);

		if (outfit && Actor()->GetActorNVHandler())
			Actor()->GetActorNVHandler()->SwitchNightVision(false, true);
	}

	pIItem->object().processing_activate();

	RuckIncompatible(pIItem);

	if (CurrentGameUI()->m_InventoryMenu->IsShown())
		CurrentGameUI()->m_InventoryMenu->UpdateSlotBlockers(); //update slotblockers only

	return true;
}


bool CInventory::MoveToBelt(PIItem pIItem)
{
	if(!CanPutInBelt(pIItem, false))
		return false;

	//Nova:
	auto ammo = smart_cast<CWeaponAmmo*>(pIItem);

	if (ammo)
	{
		int boxCurrBefore = ammo->m_boxCurr;
		RepackBelt(pIItem);

		if (!CanPutInBelt(pIItem, true) || ammo->m_boxCurr == 0)
		{
			// if we can't actually move the whole ammo pack to slot, then return
			// true if some ammo was actually moved (even if ammo object didn't), false if nothing changed
			return boxCurrBefore != ammo->m_boxCurr;
		}
	}

	//вещь была в слоте
	auto currSlot = pIItem->CurrSlot();

	if (currSlot != NO_ACTIVE_SLOT)
	{
		if (m_iActiveSlot == currSlot) ActivateSlot(NO_ACTIVE_SLOT);
		m_slots[currSlot].m_pIItem = NULL;
	}

	belt_.insert(belt_.end(), pIItem);

	// phobos2077: sort items in belt after adding new item
	std::sort(belt_.begin(), belt_.end(), GreaterRoomInRuck);

	if (currSlot == NO_ACTIVE_SLOT)
	{
		TIItemContainer::iterator it = std::find(ruck_.begin(), ruck_.end(), pIItem);
		if (ruck_.end() != it) ruck_.erase(it);
	}

	CalcTotalWeight();
	InvalidateState();

	EItemPlace p = pIItem->CurrPlace();
	pIItem->SetCurrPlace(eItemPlaceBelt);
	m_pOwner->OnItemBelt(pIItem, p);
	pIItem->OnMoveToBelt();

	if(Actor()->m_inventory == this)
		Actor()->callback(GameObject::eOnMoveToBelt)((smart_cast<CGameObject*>(pIItem))->lua_game_object());

	if (currSlot != NO_ACTIVE_SLOT)
		pIItem->object().processing_deactivate();

	pIItem->object().processing_activate();

	if (CurrentGameUI()->m_InventoryMenu->IsShown())
		CurrentGameUI()->m_InventoryMenu->UpdateBeltUI(); //update belt items position in UI

	return true;
}


bool CInventory::MoveToArtBelt(PIItem pIItem)
{
	if (!CanPutInArtBelt(pIItem))
		return false;

	//вещь была в слоте
	auto currSlot = pIItem->CurrSlot();

	if (currSlot != NO_ACTIVE_SLOT)
	{
		if (m_iActiveSlot == currSlot) ActivateSlot(NO_ACTIVE_SLOT);
		m_slots[currSlot].m_pIItem = NULL;
	}

	artefactBelt_.insert(artefactBelt_.end(), pIItem);

	// phobos2077: sort items in belt after adding new item
	std::sort(artefactBelt_.begin(), artefactBelt_.end(), GreaterRoomInRuck);

	if (currSlot == NO_ACTIVE_SLOT)
	{
		TIItemContainer::iterator it = std::find(ruck_.begin(), ruck_.end(), pIItem);
		if (ruck_.end() != it) ruck_.erase(it);
	}

	CalcTotalWeight();
	InvalidateState();

	pIItem->SetCurrPlace(eItemPlaceArtBelt);

	if (Actor()->m_inventory == this)
		Actor()->callback(GameObject::eOnMoveToArtBelt)((smart_cast<CGameObject*>(pIItem))->lua_game_object());

	if (currSlot != NO_ACTIVE_SLOT)
		pIItem->object().processing_deactivate();

	pIItem->object().processing_activate();

	return true;
}


bool CInventory::MoveToRuck(PIItem pIItem)
{
	if(!CanPutInRuck(pIItem))
		return true;

	//Nova:
	if ( pIItem->object().CLS_ID==CLSID_OBJECT_AMMO )
	{
		RepackRuck(pIItem);
	}

	auto currSlot = pIItem->CurrSlot();

	// item was in the slot
	if (currSlot != NO_ACTIVE_SLOT)
	{
		if (m_iActiveSlot == currSlot)
		{
			ActivateSlot(NO_ACTIVE_SLOT);
		}
		else if (m_currentDetectorInHand == pIItem)
		{
			m_currentDetectorInHand->HideDetector(false);
		}

		m_slots[currSlot].m_pIItem = NULL;
	}
	else
	{
		//вещь была на одном из поясов или вообще только поднята с земли

		TIItemContainer::iterator it = std::find(belt_.begin(), belt_.end(), pIItem);
		if (belt_.end() != it) belt_.erase(it);

		it = std::find(artefactBelt_.begin(), artefactBelt_.end(), pIItem);
		if (artefactBelt_.end() != it) artefactBelt_.erase(it);
	}
	//Msg("%s Inventory::MoveToRuck() %s from slot: %d", m_pOwner->Name(), pIItem->object().SectionNameStr(), currSlot);

	ruck_.insert(ruck_.end(), pIItem);

	CalcTotalWeight();
	InvalidateState();

	m_pOwner->OnItemRuck(pIItem, pIItem->CurrPlace());
	pIItem->SetCurrPlace(eItemPlaceRuck);
	pIItem->OnMoveToRuck();

	if (Actor()->m_inventory == this)
	{
		Actor()->callback(GameObject::eOnMoveToRuck)((smart_cast<CGameObject*>(pIItem))->lua_game_object());

		// Disable NV device, since we are taking off suit/helmet
		COutfitBase* outfit = smart_cast<COutfitBase*>(pIItem);

		if (outfit && Actor()->GetActorNVHandler())
			Actor()->GetActorNVHandler()->SwitchNightVision(false, true);
	}

	if (currSlot != NO_ACTIVE_SLOT)
		pIItem->object().processing_deactivate();

	CCustomOutfit* outfit_item = smart_cast<CCustomOutfit*>(pIItem);

	if (outfit_item)
	{
		CheckBeltCapacity();
	}

	if (CurrentGameUI()->m_InventoryMenu->IsShown())
		CurrentGameUI()->m_InventoryMenu->UpdateSlotBlockers(); //update slotblockers only

	return true;
}


void CInventory::RepackRuck(PIItem pIItem)
{
	CWeaponAmmo* ammo = smart_cast<CWeaponAmmo*>(pIItem);

	R_ASSERT(ammo);

	if (!ruck_.size() || pIItem->CurrPlace() == eItemPlaceRuck)
		return;

	TIItemContainer::const_iterator it = ruck_.begin();
	TIItemContainer::const_iterator it_end = ruck_.end();

	for (it; it != it_end; it++)
	{
		CInventoryItem* invAmmoObj = (*it);
		if (invAmmoObj->CurrPlace() == eItemPlaceRuck && invAmmoObj->object().SectionName() == pIItem->object().SectionName()) //Is in ruck, is the same type;
		{
			CWeaponAmmo* ruckAmmo = smart_cast<CWeaponAmmo*>(invAmmoObj); //Cast to ammo obj

			R_ASSERT(ruckAmmo); //Check the cast

			if (ammo == ruckAmmo || ruckAmmo->m_boxCurr == ruckAmmo->m_boxSize)
				continue; //Just skip it.

			u16 empty_space = ruckAmmo->m_boxSize - ruckAmmo->m_boxCurr;

			if (empty_space > ammo->m_boxCurr)
			{
				ruckAmmo->m_boxCurr += ammo->m_boxCurr;
				ammo->m_boxCurr = 0;
			}
			else
			{
				ruckAmmo->m_boxCurr += empty_space;
				ammo->m_boxCurr -= empty_space;
			}

			if (ammo->m_boxCurr == 0) //Its and empty Box, Discard it.
			{
				pIItem->SetDropManual(true);
				return;
			}
		}
	}
}


void CInventory::RepackBelt(PIItem pIItem)
{
	CWeaponAmmo* ammo = smart_cast<CWeaponAmmo*>(pIItem);
	R_ASSERT(ammo);

	if (!belt_.size() || pIItem->CurrPlace() == eItemPlaceBelt)
		return; //Belt is empty, nothing to repack.

	TIItemContainer::const_iterator it = belt_.begin();
	TIItemContainer::const_iterator it_end = belt_.end();

	for (it; it != it_end; it++)
	{
		CInventoryItem* invAmmoObj = (*it);

		if (invAmmoObj->CurrPlace() == eItemPlaceBelt && invAmmoObj->object().SectionName() == pIItem->object().SectionName()) //Is on belt, is the same type;
		{
			CWeaponAmmo* beltAmmo = smart_cast<CWeaponAmmo*>(invAmmoObj); //Cast to ammo obj

			R_ASSERT(beltAmmo); //Check the cast

			if (ammo == beltAmmo || beltAmmo->m_boxCurr == beltAmmo->m_boxSize)
				continue; //Just skip it.

			u16 empty_space = beltAmmo->m_boxSize - beltAmmo->m_boxCurr;

			if (empty_space > 0) //Is this box not full?
			{
				if (empty_space > ammo->m_boxCurr)
				{
					beltAmmo->m_boxCurr += ammo->m_boxCurr;
					ammo->m_boxCurr = 0;
				}
				else
				{
					beltAmmo->m_boxCurr += empty_space;
					ammo->m_boxCurr -= empty_space;
				}
			}

			if (ammo->m_boxCurr == 0) //Its empty Box, Discard it.
			{
				pIItem->SetDropManual(true);
				return;
			}
		}
	}
}


void CInventory::ActivateDetector()
{
	PIItem p_item = ItemFromSlot(DETECTOR_SLOT);

	if (p_item)
	{
		CInventoryOwner* actor_owner = Actor() ? Actor()->cast_inventory_owner() : nullptr;

		if (m_pOwner == actor_owner)
		{
			CArtDetectorBase* artifact_detector = smart_cast<CArtDetectorBase*>(p_item);
			if (m_slots[DETECTOR_SLOT].CanBeActivated())
			{
				artifact_detector->ToggleDetector(false);
			}
			else
			{
				artifact_detector->HideDetector(true);
			}
		}
	}
}

bool CInventory::ActivateSlot(TSlotId slot, bool bForce)
{
	if (slot == DETECTOR_SLOT)
	{
		ActivateDetector();

		return true;
	}

	return TryActivate(slot, bForce);
}

//Don't forget: GCS uses this code for NPCs too
bool CInventory::TryActivate(TSlotId slot, bool forced_activation)
{
	R_ASSERT2(slot == NO_ACTIVE_SLOT || slot < m_slots.size(), "wrong slot number");

	if (slot == m_iActiveSlot)
		return false;

	if (slot != NO_ACTIVE_SLOT && !m_slots[slot].CanBeActivated() && !forced_activation)
		return false; //Dont try to activate stuff not ment to be activated, unless forced to

	if (slot == GRENADE_SLOT) // For grenades, since they have different activation logic
	{
		u16 actor_id = Level().CurrentEntity() ? Level().CurrentEntity()->ID() : 65535;

		PIItem grenade_item = GetNotSameBySlotNum(GRENADE_SLOT, NULL, GetOwner()->object_id() == actor_id ? belt_ : ruck_);

		if (grenade_item)
		{
			MoveToSlot(slot, grenade_item);
			if (CurrentGameUI()->m_InventoryMenu->IsShown())
				CurrentGameUI()->m_InventoryMenu->ReinitInventoryWnd(); // needed to update belt cell items
		}
	}

	CInventoryItem* cur_itm_in_this_slot = nullptr;

	if (slot != NO_ACTIVE_SLOT)
	{
		cur_itm_in_this_slot = ItemFromSlot(slot);
	}

	// Если в руке находится детектор и оружие не совместимо с ним, то сначало спрятать детектор
	if (slot != NO_ACTIVE_SLOT && slot <= LAST_SLOT)
	{
		if (m_currentDetectorInHand && cur_itm_in_this_slot && !cur_itm_in_this_slot->IsSingleHand())
		{
			m_currentDetectorInHand->HideDetector(false);
		}
	}

	if (slot != NO_ACTIVE_SLOT && !cur_itm_in_this_slot)
		return false; // if no item - dont activate it, but if need to activate NO_ACTIVE_SLOT - do it

	m_iNextActiveSlot = slot;

	return true;
}


void CInventory::TryNextActiveSlot()
{
	if (m_iNextActiveSlot == m_iActiveSlot) return;

	bool detector_is_hiding = m_currentDetectorInHand && m_currentDetectorInHand->IsHiding();

	if (!detector_is_hiding) //If detector is hiding - wait it
	{
		bool active_slot_visible = false;
		PIItem current_item = m_iActiveSlot != NO_ACTIVE_SLOT ? m_slots[m_iActiveSlot].m_pIItem : nullptr;

		if (current_item && current_item->cast_hud_item() && !current_item->cast_hud_item()->IsHidden())
			active_slot_visible = true;


		if (!active_slot_visible)
		{
			PIItem next_item = m_iNextActiveSlot != NO_ACTIVE_SLOT ? m_slots[m_iNextActiveSlot].m_pIItem : nullptr;
			if (m_iNextActiveSlot != NO_ACTIVE_SLOT && next_item &&	next_item->cast_hud_item()) // If next active slot is a real slot - show its hud item
			{
				//Msg("Start Activating of next active hud item [Inventory ovner name %s]", m_pOwner->cast_game_object()->SectionNameStr());
				next_item->cast_hud_item()->ActivateItem(); //Show next active slot hud item
			}
			m_iActiveSlot = m_iNextActiveSlot; // Disable this function untill next active slot selection
		}

		if (active_slot_visible && (current_item && !current_item->cast_hud_item()->IsHiding())) // If active item is not hiding - start hiding, else wait untill it hides
		{	
			//Msg("Start Hiding of active hud item [Inventory ovner name %s]", m_pOwner->cast_game_object()->SectionNameStr());
			current_item->cast_hud_item()->SendDeactivateItem(); // Start Hiding of active hud item
		}
		else
		{
			//Msg("Waiting while previous item is Hiding");
		}
	}
}

void CInventory::Update()
{
	TryNextActiveSlot();

	UpdateDropTasks();
}


void CInventory::UpdateDropTasks()
{
	for(TSlotId i = FirstSlot(); i <= LastSlot(); ++i)	
	{
		PIItem itm = ItemFromSlot(i);

		if(itm)
			UpdateDropItem(itm);
	}

	for(TSlotId i = 0; i < 3; ++i)
	{
		TIItemContainer &list			= i == 1 ? ruck_ : i == 2 ? belt_ : artefactBelt_;
		TIItemContainer::iterator it	= list.begin();
		TIItemContainer::iterator it_e	= list.end();

		for( ; it != it_e; ++it)
		{
			UpdateDropItem(*it);
		}
	}

	if (m_drop_last_frame)
	{
		m_drop_last_frame			= false;
		m_pOwner->OnItemDropUpdate	();
	}
}

void CInventory::UpdateDropItem(PIItem pIItem)
{
	if(pIItem && pIItem->GetDropManual())
	{
		pIItem->SetDropManual(FALSE);

		NET_Packet					P;
		pIItem->object().u_EventGen	(P, GE_OWNERSHIP_REJECT, pIItem->object().H_Parent()->ID());
		P.w_u16						(u16(pIItem->object().ID()));
		pIItem->object().u_EventSend(P);

	}// dropManual

	if (pIItem->GetDeleteManual())
	{
		pIItem->SetDeleteManual(FALSE);

		NET_Packet					P;
		pIItem->object().u_EventGen	(P, GE_DESTROY, u16(pIItem->object().ID()));
		pIItem->object().u_EventSend(P);
	}
}

bool CInventory::IsInSlot(PIItem pIItem) const
{
	if (pIItem->CurrPlace() != eItemPlaceSlot)
		return false;

	VERIFY(m_slots[pIItem->CurrSlot()].m_pIItem == pIItem);

	return true;
}

bool CInventory::IsInBelt(PIItem pIItem) const
{
	auto it = std::find(belt_.begin(), belt_.end(), pIItem);

	if (belt_.end() != it)
		return true;

	return false;
}

bool CInventory::IsInArtBelt(PIItem pIItem) const
{
	auto it = std::find(artefactBelt_.begin(), artefactBelt_.end(), pIItem);

	if (artefactBelt_.end() != it)
		return true;

	return false;
}

bool CInventory::IsInRuck(PIItem pIItem) const
{
	auto it = std::find(ruck_.begin(), ruck_.end(), pIItem);

	if (ruck_.end() != it)
		return true;

	return false;
}


bool CInventory::CanPutInSlot(PIItem pIItem, TSlotId slot_id) const
{
	if (!m_bSlotsUseful)
		return false;

	if (!GetOwner()->CanPutInSlot(pIItem, slot_id))
		return false;

	if (!CheckCompatibility(pIItem))
		return false;

	if (slot_id != NO_ACTIVE_SLOT && ItemFromSlot(slot_id) == nullptr)
		return true;

	return false;
}


bool CInventory::CanPutInBelt(PIItem pIItem, bool forceRoomCheck)
{
	if (loading_save_timer_started)
		return true; // dont check during loading - save file contains only alowed ones
	if (!pIItem)
		return false;
	if (IsInBelt(pIItem))
		return false;
	if (!m_bBeltUseful)
		return false;
	if (!pIItem->IsForBelt())
		return false;

	bool needRoom = (forceRoomCheck || smart_cast<CWeaponAmmo*>(pIItem) == nullptr);

	if (needRoom && GetUsedSlots_Width(belt_) >= GetBeltMaxSlots())
		return false;

	return !needRoom || FreeRoom_inBelt(belt_, pIItem, GetBeltMaxSlots(), 1);
}


bool CInventory::CanPutInArtBelt(PIItem pIItem)
{
	if (loading_save_timer_started)
		return true; // dont check during loading - save file contains only alowed ones
	if (!pIItem)
		return false;
	if (IsInArtBelt(pIItem))
		return false;
	if (!pIItem->IsForArtBelt())
		return false;

	if (GetUsedSlots_Width(artefactBelt_) >= GetArtBeltMaxSlots()) return false;

	return FreeRoom_inBelt(artefactBelt_, pIItem, GetArtBeltMaxSlots(), 1);

	return true;
}


bool CInventory::CanPutInRuck(PIItem pIItem) const
{
	if (IsInRuck(pIItem))
		return false;

	return true;
}

bool CInventory::CanTakeItem(CInventoryItem *inventory_item) const
{
	if (inventory_item->object().getDestroy())
		return false;

	if (!inventory_item->CanTake())
		return false;

	TIItemContainer::const_iterator it = allContainer_.begin();
	for (; it != allContainer_.end(); it++)
		if ((*it)->object().ID() == inventory_item->object().ID()) break;

	VERIFY3(it == allContainer_.end(), "item already exists in inventory", *inventory_item->object().ObjectName());

	CActor* pActor = smart_cast<CActor*>(m_pOwner);

	//актер всегда может взять вещь
	if (!pActor && (GetTotalWeight() + inventory_item->Weight() > m_pOwner->MaxCarryWeight()))
		return	false;

	return	true;
}


bool CInventory::CheckCompatibility(PIItem moving_item) const
{
	if (moving_item->BaseSlot() == HELMET_SLOT || moving_item->BaseSlot() == PNV_SLOT)
	{
		PIItem outfit_slot_item = ItemFromSlot(OUTFIT_SLOT);
		PIItem helmet_slot_item = ItemFromSlot(HELMET_SLOT);

		if (outfit_slot_item)
		{
			CCustomOutfit* outfit_item = smart_cast<CCustomOutfit*>(outfit_slot_item);

			if (outfit_item)
			{
				if (moving_item->BaseSlot() == HELMET_SLOT && outfit_item->block_helmet_slot)
					return false;

				if (moving_item->BaseSlot() == PNV_SLOT && outfit_item->block_pnv_slot)
					return false;
			}
		}

		if (helmet_slot_item)
		{
			COutfitBase* helmet = smart_cast<COutfitBase*>(helmet_slot_item);

			if (helmet && moving_item->BaseSlot() == PNV_SLOT && helmet->block_pnv_slot)
				return false;
		}
	}

	return true;
}

void CInventory::RuckIncompatible(PIItem priority_item)
{
	// Ruck pnv and helmet if outfit is not compatable with them
	COutfitBase* base_outfit = smart_cast<COutfitBase*>(priority_item);

	if (base_outfit)
	{
		bool need_whole_wnd_update = false;

		if (base_outfit->block_pnv_slot)
		{
			CInventoryItem* pnv = ItemFromSlot(PNV_SLOT);
			if (pnv)
			{
				MoveToRuck(pnv);
				need_whole_wnd_update = true;
			}
		}

		CCustomOutfit* outfit = smart_cast<CCustomOutfit*>(priority_item);

		if (outfit && outfit->block_helmet_slot)
		{
			CInventoryItem* helmet = ItemFromSlot(HELMET_SLOT);
			if (helmet)
			{
				MoveToRuck(helmet);
				need_whole_wnd_update = true;
			}
		}

		// Reinit inventory wnd if its shown to update slotblockers and items position
		if (CurrentGameUI()->m_InventoryMenu->IsShown() && need_whole_wnd_update)
		{
			CurrentGameUI()->m_InventoryMenu->ReinitInventoryWnd();
		}
	}

}

void CInventory::CheckBeltCapacity()
{
	if (loading_save_timer_started)
		return; // dont remove items from belt during loading - save file contains only alowed ones

	bool need_whole_wnd_update = false;

	while (GetUsedSlots_Width(belt_) > GetBeltMaxSlots())
	{
		MoveToRuck(belt_.back());
		need_whole_wnd_update = true;
	}

	while (GetUsedSlots_Width(artefactBelt_) > GetArtBeltMaxSlots())
	{
		MoveToRuck(artefactBelt_.back());
		need_whole_wnd_update = true;
	}

	// Reinit inventory wnd if its shown to update slotblockers and items position
	if (CurrentGameUI()->m_InventoryMenu->IsShown() && need_whole_wnd_update)
	{
		CurrentGameUI()->m_InventoryMenu->ReinitInventoryWnd();
	}
}

static bool GetItemPredicate(PIItem item, const char* name)
{
	return item != nullptr && !xr_strcmp(item->object().SectionName(), name) && item->Useful();
}

PIItem CInventory::GetItemBySect(const char *section, TIItemContainer& container_to_search) const
{
	for (auto it = container_to_search.cbegin(); container_to_search.cend() != it; ++it)
	{
		if (GetItemPredicate(*it, section))
			return *it;
	}

	return nullptr;
}

PIItem CInventory::GetItemByID(const u16 id, TIItemContainer& container_to_search) const
{
	for (TIItemContainer::const_iterator it = container_to_search.begin(); container_to_search.end() != it; ++it)
	{
		PIItem pIItem = *it;

		if (pIItem && pIItem->object().ID() == id)
			return pIItem;
	}

	return nullptr;
}

PIItem CInventory::GetItemByClassID(CLASS_ID cls_id, TIItemContainer& container_to_search) const
{
	for (TIItemContainer::const_iterator it = container_to_search.begin(); container_to_search.end() != it; ++it)
	{
		PIItem pIItem = *it;

		if (pIItem)
			if (pIItem->object().CLS_ID == cls_id && pIItem->Useful())
				return pIItem;
	}

	return nullptr;
}

PIItem CInventory::GetItemByIndex(int iIndex, TIItemContainer& container_to_search)
{
	if ((iIndex >= 0) && (iIndex < (int)container_to_search.size()))
	{
		if (container_to_search[iIndex])
			return (container_to_search[iIndex]);
	}
	else
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError, "invalid inventory index!");
		return nullptr;
	}

	R_ASSERT(false);

	return nullptr;
}

PIItem CInventory::GetItemByName(LPCSTR gameobject_name, TIItemContainer& container_to_search)
{
	u32 crc = crc32(gameobject_name, xr_strlen(gameobject_name));

	for (TIItemContainer::iterator l_it = container_to_search.begin(); container_to_search.end() != l_it; ++l_it)

		if ((*l_it)->object().SectionName()._get()->dwCRC == crc)
		{
			VERIFY(0 == xr_strcmp((*l_it)->object().SectionName().c_str(), gameobject_name));
			return	(*l_it);
		}

	return nullptr;
}

PIItem CInventory::GetNotSameBySect(const char *section, const PIItem not_same_with, TIItemContainer& container_to_search) const
{
	for (TIItemContainer::const_iterator it = container_to_search.begin(); container_to_search.end() != it; ++it)
	{
		const PIItem l_pIItem = *it;

		if ((l_pIItem != not_same_with) && !xr_strcmp(l_pIItem->object().SectionName(), section))
			return l_pIItem;
	}

	return nullptr;
}

PIItem CInventory::GetNotSameBySlotNum(const TSlotId slot, const PIItem not_same_with, TIItemContainer& container_to_search) const
{
	if (slot == NO_ACTIVE_SLOT)
		return nullptr;

	for (TIItemContainer::const_iterator it = container_to_search.begin(); container_to_search.end() != it; ++it)
	{
		PIItem _pIItem = *it;
		if (_pIItem != not_same_with && _pIItem->BaseSlot() == slot) return _pIItem;
	}

	return nullptr;
}

PIItem CInventory::GetAmmo(const char *section, const TIItemContainer& container_to_search) const
{
	for (auto it = container_to_search.crbegin(); container_to_search.crend() != it; ++it)
	{
		PIItem pIItem = *it;
		if (GetItemPredicate(pIItem, section))
		{
			return pIItem;
		}
	}

	return nullptr;
}

PIItem CInventory::GetAmmoFromInv(const char *section) const
{
	PIItem itm = GetAmmo(section, belt_);

	//для НПС ищем в рюкзаке
	if (!itm && (this != &g_actor->inventory()))
	{
		itm = GetAmmo(section, ruck_);
	}

	return itm;
}

u32	CInventory::GetItemsCount()
{
	return(allContainer_.size());
}

u32 CInventory::GetSameItemCountBySect(const char *section, TIItemContainer& container_to_search)
{
	u32 l_dwCount = 0;

	for (TIItemContainer::iterator l_it = container_to_search.begin(); container_to_search.end() != l_it; ++l_it)
	{
		PIItem	l_pIItem = *l_it;
		if (l_pIItem && !xr_strcmp(l_pIItem->object().SectionName(), section))
			++l_dwCount;
	}

	return(l_dwCount);
}

u32 CInventory::GetSameItemCountBySlot(TSlotId slot_id, TIItemContainer& container_to_search)
{
	u32 l_dwCount = 0;

	for (TIItemContainer::iterator l_it = container_to_search.begin(); container_to_search.end() != l_it; ++l_it)
	{
		PIItem	l_pIItem = *l_it;
		if (l_pIItem && l_pIItem->BaseSlot() == slot_id)
			++l_dwCount;
	}

	return(l_dwCount);
}

PIItem CInventory::ItemFromSlot(TSlotId slot) const
{
	VERIFY(NO_ACTIVE_SLOT != slot);

	return m_slots[slot].m_pIItem;
}

float CInventory::GetTotalWeight() const
{
	VERIFY(m_fTotalWeight>=0.f);

	return m_fTotalWeight;
}

float CInventory::CalcTotalWeight()
{
	float weight = 0;

	for (TIItemContainer::const_iterator it = allContainer_.begin(); allContainer_.end() != it; ++it)
		weight += (*it)->Weight();

	m_fTotalWeight = weight;

	return m_fTotalWeight;
}

u32  CInventory::GetBeltMaxSlots() const
{
	PIItem item_from_outfit_slot = ItemFromSlot(OUTFIT_SLOT);
	CCustomOutfit* outfit = nullptr;

	if (item_from_outfit_slot)
		outfit = smart_cast<CCustomOutfit*>(item_from_outfit_slot);

	u32 outfit_ammo_slots_num = 0;

	if (outfit)
		outfit_ammo_slots_num = outfit->maxAmmoCount_;

	return m_iMaxBelt + outfit_ammo_slots_num;
}

u32  CInventory::GetArtBeltMaxSlots() const
{
	PIItem item_from_outfit_slot = ItemFromSlot(OUTFIT_SLOT);
	CCustomOutfit* outfit = nullptr;

	if (item_from_outfit_slot)
		outfit = smart_cast<CCustomOutfit*>(item_from_outfit_slot);

	u32 outfit_af_slots_num = 0;
	if (outfit)
		outfit_af_slots_num = outfit->maxArtefactCount_;

	return m_iMaxArtBelt + outfit_af_slots_num;
}

#define LUA_CAN_TRADE_CHECK "bind_trader.should_trade_item"

bool CInventory::LuaCheckCanTrade(CAI_Stalker* npc, PIItem item_to_check) const
{
	if (!npc || npc->IsTrader())
		return true;

	luabind::functor<bool> lua_function;

	R_ASSERT2(ai().script_engine().functor<bool>(LUA_CAN_TRADE_CHECK, lua_function), make_string("Can't find function %s", LUA_CAN_TRADE_CHECK));

	return lua_function(m_pOwner->cast_game_object()->lua_game_object(), item_to_check->cast_game_object()->lua_game_object());
}

void  CInventory::AddAvailableItems(TIItemContainer& items_container, bool for_trade) const
{
	CAI_Stalker* npc = smart_cast<CAI_Stalker*>(m_pOwner);

	for (TIItemContainer::const_iterator it = ruck_.begin(); ruck_.end() != it; ++it)
	{
		PIItem pIItem = *it;
		if (!for_trade || (pIItem->CanTrade() && LuaCheckCanTrade(npc, pIItem)))
			items_container.push_back(pIItem);
	}

	if(m_bBeltUseful)
	{
		for (TIItemContainer::const_iterator it = belt_.begin(); belt_.end() != it; ++it)
		{
			PIItem pIItem = *it;
			if (!for_trade || (pIItem->CanTrade() && LuaCheckCanTrade(npc, pIItem)))
				items_container.push_back(pIItem);
		}
	}

	for (TIItemContainer::const_iterator it = artefactBelt_.begin(); artefactBelt_.end() != it; ++it)
	{
		PIItem pIItem = *it;
		if (!for_trade || (pIItem->CanTrade() && LuaCheckCanTrade(npc, pIItem)))
			items_container.push_back(pIItem);
	}

	if (npc && !npc->g_Alive())
	{
		TISlotArr::const_iterator slot_it = m_slots.begin();
		TISlotArr::const_iterator slot_it_e	= m_slots.end();

		for(; slot_it != slot_it_e; ++slot_it)
		{
			const CInventorySlot& S = *slot_it;
			if(S.m_pIItem && S.m_pIItem->BaseSlot() != BOLT_SLOT)
				items_container.push_back(S.m_pIItem);
		}

	}
	else if (m_bSlotsUseful)
	{
		TISlotArr::const_iterator slot_it			= m_slots.begin();
		TISlotArr::const_iterator slot_it_e			= m_slots.end();

		for(; slot_it != slot_it_e; ++slot_it)
		{
			const CInventorySlot& S = *slot_it;
			if (S.m_pIItem && (!for_trade || (S.m_pIItem->CanTrade() && LuaCheckCanTrade(npc, S.m_pIItem))))
			{
				if(!S.m_bPersistent || S.m_pIItem->BaseSlot() == GRENADE_SLOT)
				{
					if (npc)
					{
						u32 slot = S.m_pIItem->BaseSlot();

						if (slot != PISTOL_SLOT && slot != RIFLE_SLOT && slot != RIFLE_2_SLOT)
							items_container.push_back(S.m_pIItem);
					}
					else
					{
						items_container.push_back(S.m_pIItem);
					}
				}
			}
		}
	}
}


void  CInventory::AddAvailableItems(TIItemContainer& items_container, SInventorySelectorPredicate& pred) const
{
	for (TIItemContainer::const_iterator it = ruck_.begin(); ruck_.end() != it; ++it)
	{
		PIItem pIItem = *it;
		if (pred(pIItem))
			items_container.push_back(pIItem);
	}

	if (m_bBeltUseful)
	{
		for (TIItemContainer::const_iterator it = belt_.begin(); belt_.end() != it; ++it)
		{
			PIItem pIItem = *it;
			if (pred(pIItem))
				items_container.push_back(pIItem);
		}
	}

	for (TIItemContainer::const_iterator it = artefactBelt_.begin(); artefactBelt_.end() != it; ++it)
	{
		PIItem pIItem = *it;
		if (pred(pIItem))
			items_container.push_back(pIItem);
	}

	CAI_Stalker* pOwner = smart_cast<CAI_Stalker*>(m_pOwner);

	if (pOwner && !pOwner->g_Alive())
	{
		TISlotArr::const_iterator slot_it = m_slots.begin();
		TISlotArr::const_iterator slot_it_e = m_slots.end();

		for (; slot_it != slot_it_e; ++slot_it)
		{
			const CInventorySlot& S = *slot_it;

			if (S.m_pIItem && S.m_pIItem->BaseSlot() != BOLT_SLOT)
				items_container.push_back(S.m_pIItem);
		}

	}
	else if (m_bSlotsUseful)
	{
		TISlotArr::const_iterator slot_it = m_slots.begin();
		TISlotArr::const_iterator slot_it_e = m_slots.end();

		for (; slot_it != slot_it_e; ++slot_it)
		{
			const CInventorySlot& S = *slot_it;

			if (S.m_pIItem && pred(S.m_pIItem))
			{
				if (!S.m_bPersistent || S.m_pIItem->BaseSlot() == GRENADE_SLOT)
				{
					if (pOwner)
					{
						u32 slot = S.m_pIItem->BaseSlot();

						if (slot != PISTOL_SLOT && slot != RIFLE_SLOT && slot != RIFLE_2_SLOT)
							items_container.push_back(S.m_pIItem);
					}
					else
					{
						items_container.push_back(S.m_pIItem);
					}
				}
			}
		}
	}
}


void CInventory::Items_SetCurrentEntityHud(bool current_entity)
{
	TIItemContainer::iterator it;

	for (it = allContainer_.begin(); allContainer_.end() != it; ++it)
	{
		CWeapon* pWeapon = smart_cast<CWeapon*>(*it);

		if (pWeapon)
		{
			pWeapon->InitAddons();
			pWeapon->UpdateAddonsVisibility();
		}
	}
};

//call this only via Actor()->SetWeaponHideState()
void CInventory::SetSlotsBlocked(u16 mask, bool bBlock, bool unholster)
{
	bool bChanged = false;

	for(int i = FirstSlot(); i <= LastSlot(); ++i)
	{
		if(mask & (1<<i))
		{
			bool bCanBeActivated = m_slots[i].CanBeActivated();

			if(bBlock)
			{
				++m_slots[i].m_blockCounter;

				if (m_slots[i].m_blockCounter > 5)
					m_slots[i].m_blockCounter = 1;

				VERIFY2(m_slots[i].m_blockCounter< 5, "block slots overflow");
			}
			else
			{
				--m_slots[i].m_blockCounter;
				if (m_slots[i].m_blockCounter < -5)
					m_slots[i].m_blockCounter = -1;

				VERIFY2(m_slots[i].m_blockCounter>-5, "block slots underflow");
			}

			if(bCanBeActivated != m_slots[i].CanBeActivated())
				bChanged = true;
		}
	}

	if (bChanged)
	{
		auto ActiveSlot		= GetActiveSlot();
		auto PrevActiveSlot	= GetPrevActiveSlot();

		if(ActiveSlot==NO_ACTIVE_SLOT)
		{
			//try to restore hidden weapon
			if (unholster && PrevActiveSlot != NO_ACTIVE_SLOT && m_slots[PrevActiveSlot].CanBeActivated())
				if (ActivateSlot(PrevActiveSlot))
					SetPrevActiveSlot(NO_ACTIVE_SLOT);
		}
		else
		{
			//try to hide active weapon
			if(!m_slots[ActiveSlot].CanBeActivated())
			{
				if (ActivateSlot(NO_ACTIVE_SLOT))
					SetPrevActiveSlot(ActiveSlot);
			}
		}

		if (m_currentDetectorInHand != nullptr && !m_slots[DETECTOR_SLOT].CanBeActivated())
		{
			m_currentDetectorInHand->HideDetector(false, true);
		}
	}
}

bool CInventory::AreSlotsBlocked()
{
	for (int i = FirstSlot(); i <= LastSlot(); ++i)
	{
		if (!m_slots[i].IsBlocked())
			return false;
	}

	return true;
}
