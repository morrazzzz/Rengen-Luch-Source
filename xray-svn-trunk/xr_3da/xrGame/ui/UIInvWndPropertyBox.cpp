#include "pch_script.h"
#include "UIInventoryWnd.h"
#include "../actor.h"
#include "../silencer.h"
#include "../scope.h"
#include "../grenadelauncher.h"
#include "../eatable_item.h"
#include "../BottleItem.h"
#include "../WeaponMagazined.h"
#include "../inventory.h"
#include "../xr_level_controller.h"
#include "UICellItem.h"
#include "UIListBoxItem.h"
#include "../CustomOutfit.h"
#include "../script_callback_ex.h"
#include "../Medkit.h"
#include "../Antirad.h"
#include "../battery.h"
#include "../UICursor.h"
#include "../level.h"

void CUIInventoryWnd::ActivatePropertiesBox()
{
	UIPropertiesBox.RemoveAll();
	
	auto selected_item = CurrentIItem();
	CMedkit*			pMedkit				= smart_cast<CMedkit*>			(selected_item);
	CAntirad*			pAntirad			= smart_cast<CAntirad*>			(selected_item);
	CCustomOutfit*		pOutfit				= smart_cast<CCustomOutfit*>	(selected_item);
	CWeapon*			pWeapon				= smart_cast<CWeapon*>			(selected_item);
	CScope*				pScope				= smart_cast<CScope*>			(selected_item);
	CSilencer*			pSilencer			= smart_cast<CSilencer*>		(selected_item);
	CGrenadeLauncher*	pGrenadeLauncher	= smart_cast<CGrenadeLauncher*>	(selected_item);
	CBottleItem*		pBottleItem			= smart_cast<CBottleItem*>		(selected_item);
	CBattery*			pBattery			= smart_cast<CBattery*>			(selected_item);
	CEatableItem*		eatable_item		= smart_cast<CEatableItem*>		(selected_item);
    
	bool b_show_property_box = false;
	auto baseSlot = selected_item->BaseSlot();

	if(!pOutfit && baseSlot != NO_ACTIVE_SLOT && (CanPutInSlot(baseSlot, CurrentItem()) || (baseSlot == RIFLE_SLOT && CanPutInSlot(RIFLE_2_SLOT, CurrentItem()))))
	{
		UIPropertiesBox.AddItem("st_move_to_slot", NULL, INVENTORY_TO_SLOT_ACTION);
		b_show_property_box = true;
	}

	if (selected_item->IsForBelt() && Physical_Inventory->CanPutInBelt(selected_item, false))
	{
		UIPropertiesBox.AddItem("st_move_on_belt",  NULL, INVENTORY_TO_BELT_ACTION);
		b_show_property_box = true;
	}

	if (selected_item->IsForArtBelt() && Physical_Inventory->CanPutInArtBelt(selected_item))
	{
		UIPropertiesBox.AddItem("st_move_to_artefact_belt", NULL, INVENTORY_TO_ARTEFACT_BELT_ACTION);
		b_show_property_box = true;
	}

	bool already_dressed = false; 	// Флаг для невключения пункта контекстного меню: Dress Outfit, если костюм уже надет

	if (selected_item->IsForRuck() && Physical_Inventory->CanPutInRuck(selected_item) && (baseSlot == NO_ACTIVE_SLOT || baseSlot == GRENADE_SLOT || !Physical_Inventory->m_slots[baseSlot].m_bPersistent))
	{
		if(!pOutfit)
			UIPropertiesBox.AddItem("st_move_to_bag",  NULL, INVENTORY_TO_BAG_ACTION);
		else
			UIPropertiesBox.AddItem("st_undress_outfit",  NULL, INVENTORY_TO_BAG_ACTION);
		already_dressed = true;
		b_show_property_box = true;
	}

	if (pOutfit && !already_dressed)
	{
		UIPropertiesBox.AddItem("st_dress_outfit",  NULL, INVENTORY_TO_SLOT_ACTION);
		b_show_property_box = true;
	}
	
	//отсоединение аддонов от вещи
	if(pWeapon)
	{
		luabind::functor<bool> lua_bool;
		R_ASSERT2(ai().script_engine().functor<bool>(lua_is_reapirable_func, lua_bool), make_string("Can't find function %s", lua_is_reapirable_func));

		if (lua_bool(CurrentIItem()->object().ID()))
		{
			UIPropertiesBox.AddItem("st_repair_weapon", NULL, INVENTORY_REPAIR);
		}

		if(pWeapon->GrenadeLauncherAttachable() && pWeapon->IsGrenadeLauncherAttached())
		{
			UIPropertiesBox.AddItem("st_detach_gl",  NULL, INVENTORY_DETACH_GRENADE_LAUNCHER_ADDON);
			b_show_property_box = true;
		}

		if(pWeapon->ScopeAttachable() && pWeapon->IsScopeAttached())
		{
			UIPropertiesBox.AddItem("st_detach_scope",  NULL, INVENTORY_DETACH_SCOPE_ADDON);
			b_show_property_box = true;
		}

		if(pWeapon->SilencerAttachable() && pWeapon->IsSilencerAttached())
		{
			UIPropertiesBox.AddItem("st_detach_silencer",  NULL, INVENTORY_DETACH_SILENCER_ADDON);
			b_show_property_box = true;
		}

		if(smart_cast<CWeaponMagazined*>(pWeapon))
		{
			bool has_ammo = (pWeapon->GetAmmoElapsed() != 0);

			if (!has_ammo)
			{
				CUICellItem* itm = CurrentItem();
				for(u32 i=0; i<itm->ChildsCount(); ++i)
				{
					pWeapon		= smart_cast<CWeaponMagazined*>((CWeapon*)itm->Child(i)->m_pData);
					if(pWeapon->GetAmmoElapsed())
					{
						has_ammo = true;
						break;
					}
				}
			}

			if (has_ammo)
			{
				UIPropertiesBox.AddItem("st_unload_magazine",  NULL, INVENTORY_UNLOAD_MAGAZINE);
				b_show_property_box = true;
			}
		}
	}
	
	//присоединение аддонов к активному слоту (2 или 3)
	if(pScope)
	{
		AttachActionToPropertyBox(PISTOL_SLOT,  pScope, "st_attach_scope_to_pistol");
		AttachActionToPropertyBox(RIFLE_SLOT,   pScope, "st_attach_scope_to_rifle");
		AttachActionToPropertyBox(RIFLE_2_SLOT, pScope, "st_attach_scope_to_rifle");
	}

	else if(pSilencer)
	{
		AttachActionToPropertyBox(PISTOL_SLOT,  pSilencer, "st_attach_silencer_to_pistol");
		AttachActionToPropertyBox(RIFLE_SLOT,   pSilencer, "st_attach_silencer_to_rifle");
		AttachActionToPropertyBox(RIFLE_2_SLOT, pSilencer, "st_attach_silencer_to_rifle");
	}

	else if(pGrenadeLauncher)
	{
		AttachActionToPropertyBox(RIFLE_SLOT,   pGrenadeLauncher, "st_attach_gl_to_rifle");
		AttachActionToPropertyBox(RIFLE_2_SLOT, pGrenadeLauncher, "st_attach_gl_to_rifle");
	}

	LPCSTR eatable_action = NULL;

	if(pMedkit || pAntirad || pBattery)
	{
		eatable_action = "st_use";
	}
	else if (pBottleItem)
	{
		if(pBottleItem)
			eatable_action = "st_drink";
		else
			eatable_action = "st_eat";
	}

	if (eatable_action){
		UIPropertiesBox.AddItem(eatable_action, NULL, INVENTORY_EAT_ACTION);
		b_show_property_box = true;
	}

	if (eatable_item)
	{
		UIPropertiesBox.AddItem("st_to_quick_slot_1", NULL, INVENTORY_TO_QUICK_SLOT_1);
		UIPropertiesBox.AddItem("st_to_quick_slot_2", NULL, INVENTORY_TO_QUICK_SLOT_2);
		UIPropertiesBox.AddItem("st_to_quick_slot_3", NULL, INVENTORY_TO_QUICK_SLOT_3);
		UIPropertiesBox.AddItem("st_to_quick_slot_4", NULL, INVENTORY_TO_QUICK_SLOT_4);
		b_show_property_box = true;

	}

	bool block_drop = (pOutfit && already_dressed);
	block_drop |= !!CurrentIItem()->IsQuestItem();

	if (!block_drop)
	{
		UIPropertiesBox.AddItem("st_drop", NULL, INVENTORY_DROP_ACTION);
		b_show_property_box = true;

		if(CurrentItem()->ChildsCount())
			UIPropertiesBox.AddItem("st_drop_all", (void*)33, INVENTORY_DROP_ACTION);
	}

	if (b_show_property_box)
	{
		UIPropertiesBox.AutoUpdateSize();
		UIPropertiesBox.BringAllToTop();

		Fvector2						cursor_pos;
		Frect							vis_rect;
		GetAbsoluteRect					(vis_rect);
		cursor_pos						= GetUICursor().GetCursorPosition();
		cursor_pos.sub					(vis_rect.lt);
		UIPropertiesBox.Show			(vis_rect, cursor_pos);
		PlaySnd							(eInvProperties);
	}
}

bool CUIInventoryWnd::AttachActionToPropertyBox(TSlotId slot, CInventoryItem* addon, LPCSTR text)
{
	auto tgt = Physical_Inventory->ItemFromSlot(slot);
	if (tgt != NULL && tgt->CanAttach(addon))
	{
		UIPropertiesBox.AddItem(text, (void*)tgt, INVENTORY_ATTACH_ADDON);
		return true;
	}
	return false;
}

void CUIInventoryWnd::ProcessPropertiesBoxClicked	()
{
	if(UIPropertiesBox.GetClickedItem())
	{
		switch(UIPropertiesBox.GetClickedItem()->GetTAG())
		{
		case INVENTORY_TO_SLOT_ACTION:	
			ToSlot(CurrentItem(), true, CurrentIItem()->BaseSlot());
			break;

		case INVENTORY_TO_BELT_ACTION:	
			ToBelt(CurrentItem(),false);
			break;

		case INVENTORY_TO_ARTEFACT_BELT_ACTION:
			ToArtefactBelt(CurrentItem(), false);
			break;

		case INVENTORY_TO_BAG_ACTION:	
			ToBag(CurrentItem(),false);
			break;

		case INVENTORY_DROP_ACTION:
			{
				void* d = UIPropertiesBox.GetClickedItem()->GetData();
				bool b_all = (d==(void*)33);

				DropCurrentItem(b_all);
			}break;

		case INVENTORY_EAT_ACTION:
			{
				CEatableItem* item_to_eat = smart_cast<CEatableItem*>(CurrentIItem());
				if (item_to_eat)
					EatItem(item_to_eat);
			}
			break;

		case INVENTORY_TO_QUICK_SLOT_1:
			ToQuickSlot(CurrentItem(), 0);
			break;
		case INVENTORY_TO_QUICK_SLOT_2:
			ToQuickSlot(CurrentItem(), 1);
			break;
		case INVENTORY_TO_QUICK_SLOT_3:
			ToQuickSlot(CurrentItem(), 2);
			break;
		case INVENTORY_TO_QUICK_SLOT_4:
			ToQuickSlot(CurrentItem(), 3);
			break;

		case INVENTORY_ATTACH_ADDON:
			AttachAddon((PIItem)(UIPropertiesBox.GetClickedItem()->GetData()));
			break;

		case INVENTORY_DETACH_SCOPE_ADDON:
			DetachAddon(*(smart_cast<CWeapon*>(CurrentIItem()))->GetScopeName());
			break;

		case INVENTORY_DETACH_SILENCER_ADDON:
			DetachAddon(*(smart_cast<CWeapon*>(CurrentIItem()))->GetSilencerName());
			break;

		case INVENTORY_DETACH_GRENADE_LAUNCHER_ADDON:
			DetachAddon(*(smart_cast<CWeapon*>(CurrentIItem()))->GetGrenadeLauncherName());
			break;

		case INVENTORY_RELOAD_MAGAZINE:
			(smart_cast<CWeapon*>(CurrentIItem()))->Action(kWPN_RELOAD, CMD_START);
			break;

		case INVENTORY_REPAIR:
			{
				luabind::functor<void>	repair;

				R_ASSERT2(ai().script_engine().functor<void>(lua_on_repair_wpn_func, repair), make_string("Can't find function %s", lua_on_repair_wpn_func));

				repair(CurrentIItem()->object().ID());																			//Repair
			}break;

		case INVENTORY_UNLOAD_MAGAZINE:
			{
				CUICellItem* selected_item = CurrentItem();
				(smart_cast<CWeaponMagazined*>((CWeapon*)selected_item->m_pData))->UnloadMagazine();
				for (u32 i = 0; i<selected_item->ChildsCount(); ++i)
				{
					CUICellItem* child_itm = selected_item->Child(i);
					(smart_cast<CWeaponMagazined*>((CWeapon*)child_itm->m_pData))->UnloadMagazine();
				}
			}break;
		}
	}
}

