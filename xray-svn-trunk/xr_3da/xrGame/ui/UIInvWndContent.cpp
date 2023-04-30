#include "stdafx.h"
#include "UIInventoryWnd.h"
#include "../level.h"
#include "../actor.h"
#include "../inventory.h"
#include "UIInventoryUtilities.h"

#include "UICellItem.h"
#include "UICellItemFactory.h"
#include "UIDragDropListEx.h"
#include "UIDragDropReferenceList.h"

#include "../customoutfit.h"
#include "../eatable_item.h"
#include "../ActorCondition.h"

#include "../UIGameCustom.h"
#include "../string_table.h"
#include "../weaponmagazinedwgrenade.h"

using namespace InventoryUtilities;

void CUIInventoryWnd::Update()
{
	if (m_b_need_reinit)
		InitInventory();

	if (Actor())
	{
		float v = Actor()->conditions().BleedingSpeed()*100.0f;
		UIProgressBarBleeding.SetProgressPos(v);

		v = Actor()->conditions().GetHealth()*100.0f;
		UIProgressBarHealth.SetProgressPos(v);

		v = Actor()->conditions().GetRadiation()*100.0f;
		UIProgressBarRadiation.SetProgressPos(v);

		v = Actor()->conditions().GetPower()*100.0f;
		UIProgressBarStamina.SetProgressPos(v);

		v = Actor()->conditions().GetSatiety()*100.0f;
		UIProgressBarHunger.SetProgressPos(v);

		v = Actor()->conditions().GetThirsty()*100.0f;
		UIProgressBarThirst.SetProgressPos(v);

		v = Actor()->conditions().GetPsyHealth()*100.0f;
		UIProgressBarMozg.SetProgressPos(v);

		// Armor progress bar stuff
		PIItem pItem = Physical_Inventory->ItemFromSlot(OUTFIT_SLOT);
		if (pItem)
		{
			v = pItem->GetCondition() * 100;
			UIProgressBarArmor.SetProgressPos(v);
		}
		else
			UIProgressBarArmor.SetProgressPos(0.f);


		CInventoryOwner* pInvOwner = smart_cast<CInventoryOwner*>(Actor());
		u32 _money = 0;

		_money = pInvOwner->get_money();

		// update money
		string64 sMoney;
		xr_sprintf(sMoney, "%d %s", _money, *CStringTable().translate("ui_st_currency"));
		UIMoneyWnd.SetText(sMoney);

		// update outfit parameters
		CCustomOutfit* outfit = smart_cast<CCustomOutfit*>(Physical_Inventory->m_slots[OUTFIT_SLOT].m_pIItem);
		CHelmet* helmet = (CHelmet*)Physical_Inventory->m_slots[HELMET_SLOT].m_pIItem;
		UIOutfitInfo.Update(outfit, helmet);

		//обновление веса
		UpdateWeight(UIBagWnd, pInvOwner, true);
	}

	if (use_sounds.size()>0)
	{
		for (auto it = use_sounds.begin(); it != use_sounds.end(); ++it)
		{
			ref_sound* sound = *it;

			if (sound && !sound->_feedback())

			{
				sound->destroy();

				use_sounds.erase(it);
				xr_delete(*it); // Удалить из памяти отигранные звуки использования

				it--;
			}
		}
	}

	UIStaticTimeString.SetText(*GetGameTimeAsString(etpTimeToMinutes));

	CUIWindow::Update();
}


void CUIInventoryWnd::PlaySnd(eInventorySndAction a)
{
	if (sounds[a]._handle())
		sounds[a].play(NULL, sm_2D);
}

void CUIInventoryWnd::PlayUseSound(LPCSTR sound_path)
{
	ref_sound* sound = xr_new <ref_sound> ();

	sound->create(sound_path, st_Effect, sg_SourceType);
	sound->play(NULL, sm_2D);

	use_sounds.push_back(sound);
}


bool CUIInventoryWnd::ToSlot(CUICellItem* itm, bool force_place, TSlotId slot_id)
{
	CUIDragDropListEx* old_ddlist = itm->CurrentDDList();
	PIItem iitem = (PIItem)itm->m_pData;

	if (Physical_Inventory->CanPutInSlot(iitem, slot_id))
	{
		CUIDragDropListEx* new_ddlist = GetSlotList(slot_id);

		if (slot_id == GRENADE_SLOT) return false; //Grenades use slot only for internal mechanics. They are put to belt for player visualization
		if (!new_ddlist) return true;

		CUICellItem* i = old_ddlist->RemoveItem(itm, (old_ddlist == new_ddlist));
		if (!new_ddlist) 
			return false;
		else 
			new_ddlist->SetItem(i);

		bool result = Physical_Inventory->MoveToSlot(slot_id, iitem);
		VERIFY(result);
		if (!result) return true;

		Physical_Inventory->ActivateSlot(slot_id);

		PlaySnd(eInvItemToSlot);
		
		return true;
	}
	else // in case moving is blocked somehow
	{
		if (!force_place || slot_id == NO_ACTIVE_SLOT || Physical_Inventory->m_slots[slot_id].m_bPersistent) return false;

		if (!Physical_Inventory->CheckCompatibility(iitem)) return false; // necessary, to avoid crashing below

		if (slot_id == RIFLE_SLOT && Physical_Inventory->CanPutInSlot(iitem, RIFLE_2_SLOT))
			return ToSlot(itm, force_place, RIFLE_2_SLOT);

		if (slot_id == RIFLE_2_SLOT && Physical_Inventory->CanPutInSlot(iitem, RIFLE_SLOT))
			return ToSlot(itm, force_place, RIFLE_SLOT);

		PIItem _iitem = Physical_Inventory->ItemFromSlot(slot_id);
		CUIDragDropListEx* slot_list = GetSlotList(slot_id);
		VERIFY(slot_list->ItemsCount()==1);

		CUICellItem* slot_cell = slot_list->GetItemIdx(0);
		VERIFY (slot_cell && ((PIItem)slot_cell->m_pData)==_iitem);

		bool result	= ToBag(slot_cell, false);
		VERIFY(result);

		return ToSlot(itm, false, slot_id);
	}
}


bool CUIInventoryWnd::ToBag(CUICellItem* itm, bool b_use_cursor_pos)
{
	PIItem iitem = (PIItem)itm->m_pData;

	if (Physical_Inventory->CanPutInRuck(iitem))
	{
		CUIDragDropListEx* old_ddlist = itm->CurrentDDList();
		CUIDragDropListEx* new_ddlist = NULL;
		if(b_use_cursor_pos)
		{
			new_ddlist = CUIDragDropListEx::m_drag_item->BackList();
			VERIFY(new_ddlist == m_pUIBagList);
		}
		else
			new_ddlist = m_pUIBagList;

		CUICellItem* i = old_ddlist->RemoveItem(itm, (old_ddlist == new_ddlist));

		PIItem inv_item = (PIItem)i->m_pData;

		if (currentFilter_.test(inv_item->GetInventoryCategory()))
		{
			if (b_use_cursor_pos)
				new_ddlist->SetItem(i, old_ddlist->GetDragItemPosition());
			else
				new_ddlist->SetItem(i);
		}

		bool result = Physical_Inventory->MoveToRuck(iitem);
		VERIFY(result);
		if (!result) return true;

		PlaySnd(eInvItemToRuck);

		return true;
	}
	return false;
}


bool CUIInventoryWnd::ToBelt(CUICellItem* itm, bool b_use_cursor_pos)
{
	PIItem iitem = (PIItem)itm->m_pData;

	if (Physical_Inventory->CanPutInBelt(iitem, false))
	{
		if (Physical_Inventory->CanPutInBelt(iitem, true))
		{

			CUIDragDropListEx* old_ddlist = itm->CurrentDDList();
			CUIDragDropListEx* new_ddlist = NULL;

			if (b_use_cursor_pos)
			{
				new_ddlist = CUIDragDropListEx::m_drag_item->BackList();
				VERIFY(new_ddlist == m_pUIBeltList);
			}
			else
				new_ddlist = m_pUIBeltList;

			CUICellItem* cell_item = old_ddlist->RemoveItem(itm, (old_ddlist == new_ddlist));

			if (b_use_cursor_pos)
				new_ddlist->SetItem(cell_item, old_ddlist->GetDragItemPosition());
			else
				new_ddlist->SetItem(cell_item);
		}

		Physical_Inventory->MoveToBelt(iitem);

		PlaySnd(eInvItemToBelt);

		return								true;
	}

	return									false;
}


bool CUIInventoryWnd::ToArtefactBelt(CUICellItem* itm, bool b_use_cursor_pos)
{
	PIItem iitem = (PIItem)itm->m_pData;

	if (Physical_Inventory->CanPutInArtBelt(iitem))
	{
		CUIDragDropListEx* old_ddlist = itm->CurrentDDList();
		CUIDragDropListEx* new_ddlist = NULL;

		if (b_use_cursor_pos)
		{
			new_ddlist = CUIDragDropListEx::m_drag_item->BackList();
			VERIFY(new_ddlist == m_pUIArtefactBeltList);
		}
		else
			new_ddlist = m_pUIArtefactBeltList;

		CUICellItem* cell_item = old_ddlist->RemoveItem(itm, (old_ddlist == new_ddlist));

		if (b_use_cursor_pos)
			new_ddlist->SetItem(cell_item, old_ddlist->GetDragItemPosition());
		else
			new_ddlist->SetItem(cell_item);

		Physical_Inventory->MoveToArtBelt(iitem);

		PlaySnd(eInvItemToBelt);

		return								true;
	}

	return									false;
}

bool CUIInventoryWnd::ToQuickSlot(CUICellItem* itm, u8 particular_slot = 255)
{
	PIItem iitem = (PIItem)itm->m_pData;
	CEatableItem* eat_item = smart_cast<CEatableItem*>(iitem);
	if (!eat_item)
		return false;


	if (eat_item->notForQSlot_ || itm->GetGridSize().x > 1 || itm->GetGridSize().y > 1) // Prevet crash + allow only 1x1 items
	{
		SDrawStaticStruct* HudMessage = CurrentGameUI()->AddCustomStatic("inv_hud_message", true);
		HudMessage->m_endTime = EngineTime() + 3.0f;

		string1024 str;
		xr_sprintf(str, "%s", *CStringTable().translate("st_cant_place_in_qslot"));
		HudMessage->wnd()->TextItemControl()->SetText(str);
		return false;
	}

	u8 slot_idx = u8(quickUseSlots_->PickCell(GetUICursor().GetCursorPosition()).x);
	if (slot_idx == 255 && particular_slot == 255)
		return false;

	if (particular_slot != 255)
	{
		Ivector2 cell_id = {particular_slot, 0};
		quickUseSlots_->SetItem(create_cell_item(iitem, true), cell_id);
		xr_strcpy(Actor()->quickUseSlotsContents_[particular_slot], iitem->m_section_id.c_str());
	}
	else
	{
		quickUseSlots_->SetItem(create_cell_item(iitem, true), GetUICursor().GetCursorPosition());
		xr_strcpy(Actor()->quickUseSlotsContents_[slot_idx], iitem->m_section_id.c_str());
	}
	return true;
}

void CUIInventoryWnd::EatItem(CEatableItem* ItemToEat)
{
	SetCurrentItem(NULL);

	if (!ItemToEat->Useful()) return;

	Physical_Inventory->Eat(ItemToEat);

	ReinitInventoryWnd(); // To separate the eaten portion items from whole items when they are combined in one cell
}

void CUIInventoryWnd::AttachAddon(PIItem item_to_upgrade)
{
	PlaySnd(eInvAttachAddon);
	R_ASSERT(item_to_upgrade);

	item_to_upgrade->Attach(CurrentIItem(), true);

	//спрятать вещь из активного слота в инвентарь на время вызова менюшки
	if (item_to_upgrade == Physical_Inventory->ActiveItem())
	{
		m_iCurrentActiveSlot = Physical_Inventory->GetActiveSlot();
		Physical_Inventory->ActivateSlot(NO_ACTIVE_SLOT);
	}
	SetCurrentItem(NULL);
}

void CUIInventoryWnd::DetachAddon(const char* addon_name)
{
	PlaySnd(eInvDetachAddon);

	CurrentIItem()->Detach(addon_name, true);

	//спрятать вещь из активного слота в инвентарь на время вызова менюшки
	if (CurrentIItem() == Physical_Inventory->ActiveItem())
	{
		m_iCurrentActiveSlot = Physical_Inventory->GetActiveSlot();
		Physical_Inventory->ActivateSlot(NO_ACTIVE_SLOT);
	}
}

bool CUIInventoryWnd::DragDropItem(PIItem itm, CUIDragDropListEx* ddlist)
{
	if (ddlist == m_pUIOutfitList) //Dragging eatable items to outfit slot (never knew this)
	{
		CEatableItem* item_to_eat = smart_cast<CEatableItem*>(CurrentIItem());
		if (item_to_eat)
		{
			EatItem(item_to_eat);
			return true;
		}
	}

	CUICellItem* cell_item = ddlist->ItemsCount() ? ddlist->GetItemIdx(0) : NULL;
	PIItem item_in_cell = cell_item ? (PIItem)cell_item->m_pData : NULL;

	if (!item_in_cell) return false;
	if (!item_in_cell->CanAttach(itm)) return false;
	AttachAddon(item_in_cell);

	return true;
}

void CUIInventoryWnd::DropCurrentItem(bool b_all)
{
	if (CurrentIItem() && !CurrentIItem()->IsQuestItem())
	{
		if (b_all)
		{
			u32 cnt = CurrentItem()->ChildsCount();

			for (u32 i = 0; i<cnt; ++i)
			{
				CUICellItem* cell_itm = CurrentItem()->PopChild(NULL);
				PIItem iitm = (PIItem)cell_itm->m_pData;
				iitm->SetDropManual(TRUE);
			}
		}

		CurrentIItem()->SetDropManual(TRUE);

		SetCurrentItem(NULL);

		PlaySnd(eInvDropItem);
	}
}

void CUIInventoryWnd::AddItemToBag(PIItem pItem)
{
	if (currentFilter_.test(pItem->GetInventoryCategory()))
	{
		CUICellItem* itm = create_cell_item(pItem);
		m_pUIBagList->SetItem(itm);
	}
}

bool CUIInventoryWnd::OnItemStartDrag(CUICellItem* itm)
{
	return false; //default behaviour
}

bool CUIInventoryWnd::OnItemSelected(CUICellItem* itm)
{
	if (m_pCurrentCellItem) 
		m_pCurrentCellItem->Mark(false);

	if (!itm) return false;

	SetCurrentItem(itm);

	MarkApplicableItems(itm);

	itm->Mark(true);

	return false;
}


bool CUIInventoryWnd::OnItemDragDrop(CUICellItem* cellitem)
{
	CUIDragDropListEx* old_ddlist = cellitem->CurrentDDList();
	CUIDragDropListEx* new_ddlist = CUIDragDropListEx::m_drag_item->BackList();

	if (!old_ddlist || !new_ddlist)
		return false;
	
	EListType t_new = GetType(new_ddlist);
	EListType t_old = GetType(old_ddlist);
	
	if (old_ddlist == new_ddlist)
	{
		if (t_new==iwBelt)
			SumAmmoByDrop(cellitem, old_ddlist);

		return false;
	}

	if (t_new != iwSlot && t_new == t_old) return true;

	switch(t_new)
	{
		case iwSlot:
		{
			TSlotId targetSlot = GetSlot(new_ddlist, cellitem);
			if (SlotIsCompatible(targetSlot, cellitem))
			{
				ToSlot(cellitem, true, targetSlot);
			}
		}
		break;

		case iwBag:
		{
			ToBag(cellitem, true);
		}
		break;

		case iwBelt:
		{
			ToBelt(cellitem, true);
		}
		break;

		case iwArtefactBelt:
		{
			ToArtefactBelt(cellitem, true);
		}
		break;

		case iwQuickSlot:
		{
			ToQuickSlot(cellitem);
		}break;
	};

	DragDropItem(CurrentIItem(), new_ddlist);

	return true;
}


bool CUIInventoryWnd::OnItemDbClick(CUICellItem* cellitem)
{
	CEatableItem* item_to_eat = smart_cast<CEatableItem*>(CurrentIItem());
	if (item_to_eat)
	{
		EatItem(item_to_eat);
		return true;
	}

	auto iitem = (PIItem)cellitem->m_pData;

	CUIDragDropListEx* old_ddlist = cellitem->CurrentDDList();
	EListType old_ddlist_type = GetType(old_ddlist);

	switch (old_ddlist_type)
	{
		case iwSlot:
		{
			ToBag(cellitem, false);
		}break;

		case iwBelt:
		{
			ToBag(cellitem, false);
		}break;

		case iwArtefactBelt:
		{
			ToBag(cellitem, false);
		}break;

		case iwBag:
		{
			TSlotId baseSlot = iitem->BaseSlot();

			if (!ToSlot(cellitem, false, baseSlot))
			{
				if (!ToBelt(cellitem, false))
					if (!ToArtefactBelt(cellitem, false))
						ToSlot(cellitem, true, baseSlot);
			}
		}break;
	};

	return true;
}

bool CUIInventoryWnd::OnItemRButtonClick(CUICellItem* itm)
{
	SetCurrentItem				(itm);
	ActivatePropertiesBox		();
	return						false;
}

void CUIInventoryWnd::MarkApplicableItems(CUICellItem* itm)
{
	CInventoryItem* inventoryitem = (CInventoryItem*)itm->m_pData;
	if (!inventoryitem) return;

	u32 color = pSettings->r_color("inventory_color_ammo", "color");

	CWeaponMagazined* weapon = smart_cast<CWeaponMagazined*>(inventoryitem);
	xr_vector<shared_str> addons;

	if (weapon)
		addons = GetAddonsForWeapon(weapon);

	MarkItems(weapon, addons, *m_pUIBagList, color);
	MarkItems(weapon, addons, *m_pUIBeltList, color);
}

void CUIInventoryWnd::MarkItems(CWeaponMagazined* weapon, const xr_vector<shared_str>& addons, CUIDragDropListEx& list, u32 ammoColor)
{
	u32 item_count = list.ItemsCount();

	CWeaponMagazinedWGrenade* weapon_with_gl = smart_cast<CWeaponMagazinedWGrenade*>(weapon);

	for (u32 i = 0; i<item_count; ++i) {
		CUICellItem* cell_item = list.GetItemIdx(i);
		PIItem invitem = (PIItem)cell_item->m_pData;

		u32 texColor = 0xffffffff;
		if (weapon && invitem && invitem->Useful())
		{
			auto type = GetWeaponAccessoryType(invitem->object().SectionName(), weapon->m_ammoTypes, addons);

			if (weapon_with_gl && type == EAccossoryType::eNotAccessory) //try to check if the item is in seccond ammo type
				type = GetWeaponAccessoryType(invitem->object().SectionName(), weapon_with_gl->ammoList2_, addons);

			switch (type)
			{
			case EAccossoryType::eAmmo:
				texColor = ammoColor;
				break;
			case EAccossoryType::eAddon:
				texColor = color_rgba(255, 150, 50, 255);
				break;
			}
		}
		cell_item->SetTextureColor(texColor);
	}
}

void CUIInventoryWnd::SumAmmoByDrop(CUICellItem* cell_itm, CUIDragDropListEx* ddlist)
{
	u32 idx = ddlist->GetItemIdx(ddlist->GetDragItemPosition());
	if (idx == u32(-1)) return;
	if (!(idx<ddlist->ItemsCount())) return;

	CUICellItem* stuck_target = ddlist->GetItemIdx(idx);

	if (!stuck_target) return;

	CWeaponAmmo* dragging_ammo = smart_cast<CWeaponAmmo*>((CInventoryItem*)cell_itm->m_pData);
	CWeaponAmmo* stuck_target_ammo = smart_cast<CWeaponAmmo*>((CInventoryItem*)stuck_target->m_pData);

	if (!dragging_ammo || !stuck_target_ammo) return;

	u16 free_space = stuck_target_ammo->m_boxSize - stuck_target_ammo->m_boxCurr;

	if (free_space >= dragging_ammo->m_boxCurr)
	{
		stuck_target_ammo->m_boxCurr += dragging_ammo->m_boxCurr;
		DeleteFromInventory((CInventoryItem*)cell_itm->m_pData);
	}
	else
	{
		dragging_ammo->m_boxCurr -= free_space;
		stuck_target_ammo->m_boxCurr = stuck_target_ammo->m_boxSize;
	}
}

void CUIInventoryWnd::UpdateSlotBlockers()
{
	// Блокеры патронов
	for (u32 i = 0; i < UI_artSlotBlockers.size(); i++)
	{
		if (i < Physical_Inventory->GetArtBeltMaxSlots())
		{
			UI_artSlotBlockers[i]->Show(false);
		}
		else
		{
			UI_artSlotBlockers[i]->Show(true);
		}
	}

	// Блокеры артифактов
	for (u32 i = 0; i < UI_ammoSlotBlockers.size(); i++)
	{
		if (i < Physical_Inventory->GetBeltMaxSlots())
		{
			UI_ammoSlotBlockers[i]->Show(false);
		}
		else
		{
			UI_ammoSlotBlockers[i]->Show(true);
		}
	}

	// Блокеры ПНВ и Шлема
	UI_pnvSlotBlocker->Show(false);
	UI_helmetSlotBlocker->Show(false);

	PIItem item_from_oslot = Physical_Inventory->ItemFromSlot(OUTFIT_SLOT);
	PIItem helmet_slot_item = Physical_Inventory->ItemFromSlot(HELMET_SLOT);

	if (item_from_oslot)
	{
		CCustomOutfit* outfit = smart_cast<CCustomOutfit*>(item_from_oslot);

		if (outfit->block_pnv_slot)
			UI_pnvSlotBlocker->Show(true);

		if (outfit->block_helmet_slot)
			UI_helmetSlotBlocker->Show(true);
	}

	if (helmet_slot_item)
	{
		COutfitBase* helmet = smart_cast<COutfitBase*>(helmet_slot_item);

		if (helmet->block_pnv_slot)
			UI_pnvSlotBlocker->Show(true);
	}
}

void CUIInventoryWnd::UpdateBeltUI()
{
	m_pUIBeltList->ClearAll(true);

	TIItemContainer::iterator it, it_e;
	for (it = Physical_Inventory->belt_.begin(), it_e = Physical_Inventory->belt_.end(); it != it_e; ++it)
	{
		CUICellItem* cell_itm = create_cell_item(*it);
		m_pUIBeltList->SetItem(cell_itm);
	}
}

void CUIInventoryWnd::SetCategoryFilter(u32 mask)
{
	currentFilter_.zero();
	currentFilter_.set(mask, true);

	if (IsShown())
		UpdateBagList();

	UIFilterTabs.SetHighlitedTab(mask);
}

bool CUIInventoryWnd::DropAllItemsFromRuck(bool quest_force)
{
	if (!IsShown())
		return false;

	u32 const ci_count = m_pUIBagList->ItemsCount();

	for (u32 i = 0; i < ci_count; ++i)
	{
		CUICellItem* ci = m_pUIBagList->GetItemIdx(i);

		VERIFY(ci);

		PIItem item = (PIItem)ci->m_pData;

		VERIFY(item);

		if (!quest_force && item->IsQuestItem())
			continue;

		u32 const cnt = ci->ChildsCount();

		for (u32 j = 0; j < cnt; ++j)
		{
			CUICellItem* child_ci = ci->PopChild(NULL);
			PIItem child_item = (PIItem)child_ci->m_pData;

			DeleteFromInventory(child_item);
		}

		DeleteFromInventory(item);
	}

	SetCurrentItem(NULL);

	return true;
}