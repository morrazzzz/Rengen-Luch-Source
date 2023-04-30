#include "pch_script.h"
#include <dinput.h>
#include "UIStashWnd.h"
#include "xrUIXmlParser.h"
#include "UIXmlInit.h"
#include "../HUDManager.h"
#include "../level.h"
#include "UICharacterInfo.h"
#include "UIDragDropListEx.h"
#include "UIFrameWindow.h"
#include "UIItemInfo.h"
#include "UIPropertiesBox.h"
#include "../ai/monsters/BaseMonster/base_monster.h"
#include "../inventory.h"
#include "UIInventoryUtilities.h"
#include "UICellItem.h"
#include "UICellItemFactory.h"
#include "../WeaponMagazined.h"
#include "../Actor.h"
#include "../eatable_item.h"
#include "../alife_registry_wrappers.h"
#include "UI3tButton.h"
#include "UIListBoxItem.h"
#include "../InventoryBox.h"
#include "../BottleItem.h"
#include "../Car.h"
#include "../uicursor.h"
#include "../string_table.h"
#include "../weaponmagazinedwgrenade.h"

using namespace InventoryUtilities;

void move_item (u16 from_id, u16 to_id, u16 what_id);

CUIStashWnd::CUIStashWnd()
{
	m_pInventoryBox	= NULL;
	m_pCar = NULL;
	m_pOthersOwner = NULL;

	Init();

	m_b_need_update	= false;

	currentFilterOur_.set(icAll, true);
	currentFilterTheir_.set(icAll, true);
}

CUIStashWnd::~CUIStashWnd()
{
	m_pUIOurBagList->ClearAll					(true);
	m_pUIOthersBagList->ClearAll				(true);
}


void CUIStashWnd::Init()
{
	CUIXml uiXml;

	string128 STASH_WND_XML;
	xr_sprintf(STASH_WND_XML, "stash_wnd_%d.xml", ui_hud_type);

	string128 STASH_ITEM_XML;
	xr_sprintf(STASH_ITEM_XML, "stash_wnd_item_%d.xml", ui_hud_type);

	string128 TRADE_CHARACTER_XML;
	xr_sprintf(TRADE_CHARACTER_XML, "trade_character_%d.xml", ui_hud_type);

	uiXml.Load(CONFIG_PATH, UI_PATH, STASH_WND_XML);
	
	CUIXmlInit xml_init;

	xml_init.InitWindow			(uiXml, "main", 0, this);

	m_pUIStaticTop				= xr_new <CUIStatic>(); m_pUIStaticTop->SetAutoDelete(true);
	AttachChild					(m_pUIStaticTop);
	xml_init.InitStatic			(uiXml, "top_background", 0, m_pUIStaticTop);

	m_pUIStaticBottom			= xr_new <CUIStatic>(); m_pUIStaticBottom->SetAutoDelete(true);
	AttachChild					(m_pUIStaticBottom);
	xml_init.InitStatic			(uiXml, "bottom_background", 0, m_pUIStaticBottom);

	m_pUIOurIcon				= xr_new <CUIStatic>(); m_pUIOurIcon->SetAutoDelete(true);
	AttachChild					(m_pUIOurIcon);
	xml_init.InitStatic			(uiXml, "static_icon", 0, m_pUIOurIcon);

	m_pUIOthersIcon				= xr_new <CUIStatic>(); m_pUIOthersIcon->SetAutoDelete(true);
	AttachChild					(m_pUIOthersIcon);
	xml_init.InitStatic			(uiXml, "static_icon", 1, m_pUIOthersIcon);


	m_pUICharacterInfoLeft		= xr_new <CUICharacterInfo>(); m_pUICharacterInfoLeft->SetAutoDelete(true);
	m_pUIOurIcon->AttachChild	(m_pUICharacterInfoLeft);
	m_pUICharacterInfoLeft->Init(0,0, m_pUIOurIcon->GetWidth(), m_pUIOurIcon->GetHeight(), TRADE_CHARACTER_XML);


	m_pUICharacterInfoRight			= xr_new <CUICharacterInfo>(); m_pUICharacterInfoRight->SetAutoDelete(true);
	m_pUIOthersIcon->AttachChild	(m_pUICharacterInfoRight);
	m_pUICharacterInfoRight->Init	(0,0, m_pUIOthersIcon->GetWidth(), m_pUIOthersIcon->GetHeight(), TRADE_CHARACTER_XML);

	m_pUIOurBagWnd					= xr_new <CUIStatic>(); m_pUIOurBagWnd->SetAutoDelete(true);
	AttachChild						(m_pUIOurBagWnd);
	xml_init.InitStatic				(uiXml, "our_bag_static", 0, m_pUIOurBagWnd);
	m_pUIOurBagWnd->SetTextComplexMode(true);

	m_pUIOthersBagWnd				= xr_new <CUIStatic>(); m_pUIOthersBagWnd->SetAutoDelete(true);
	AttachChild						(m_pUIOthersBagWnd);
	xml_init.InitStatic				(uiXml, "others_bag_static", 0, m_pUIOthersBagWnd);
	m_othersBagWndPrefix			= m_pUIOthersBagWnd->GetText();
	m_pUIOthersBagWnd->SetTextComplexMode(true);

	m_pUIOurBagList					= xr_new <CUIDragDropListEx>(); m_pUIOurBagList->SetAutoDelete(true);
	m_pUIOurBagWnd->AttachChild		(m_pUIOurBagList);	
	xml_init.InitDragDropListEx		(uiXml, "dragdrop_list_our", 0, m_pUIOurBagList);

	m_pUIOthersBagList				= xr_new <CUIDragDropListEx>(); m_pUIOthersBagList->SetAutoDelete(true);
	m_pUIOthersBagWnd->AttachChild	(m_pUIOthersBagList);	
	xml_init.InitDragDropListEx		(uiXml, "dragdrop_list_other", 0, m_pUIOthersBagList);


	//информация о предмете
	m_pUIDescWnd					= xr_new <CUIFrameWindow>(); m_pUIDescWnd->SetAutoDelete(true);
	AttachChild						(m_pUIDescWnd);
	xml_init.InitFrameWindow		(uiXml, "frame_window", 0, m_pUIDescWnd);

	m_pUIStaticDesc					= xr_new <CUIStatic>(); m_pUIStaticDesc->SetAutoDelete(true);
	m_pUIDescWnd->AttachChild		(m_pUIStaticDesc);
	xml_init.InitStatic				(uiXml, "descr_static", 0, m_pUIStaticDesc);

	m_pUIItemInfo					= xr_new <CUIItemInfo>(); m_pUIItemInfo->SetAutoDelete(true);
	m_pUIDescWnd->AttachChild		(m_pUIItemInfo);
	m_pUIItemInfo->InitItemInfo		(Fvector2().set(0,0),Fvector2().set(m_pUIDescWnd->GetWidth(), m_pUIDescWnd->GetHeight()), STASH_ITEM_XML);


	xml_init.InitAutoStatic			(uiXml, "auto_static", this);

	m_pUIPropertiesBox				= xr_new <CUIPropertiesBox>(); m_pUIPropertiesBox->SetAutoDelete(true);
	AttachChild						(m_pUIPropertiesBox);
	m_pUIPropertiesBox->InitPropertiesBox	(Fvector2().set(0,0),Fvector2().set(300,300));
	m_pUIPropertiesBox->Hide();

	SetCurrentItem(NULL);
	m_pUIStaticDesc->TextItemControl()->SetText(NULL);

	m_pUITakeAll					= xr_new <CUI3tButton>(); m_pUITakeAll->SetAutoDelete(true);
	AttachChild						(m_pUITakeAll);
	xml_init.Init3tButton			(uiXml, "take_all_btn", 0, m_pUITakeAll);

	// Init item category filter tabs
	AttachChild(&UIFilterTabsOurs);
	UIFilterTabsOurs.InitFromXml(uiXml, "our_category_tabs");

	AttachChild(&UIFilterTabsTheir);
	UIFilterTabsTheir.InitFromXml(uiXml, "their_category_tabs");

	BindDragDropListEvents			(m_pUIOurBagList);
	BindDragDropListEvents			(m_pUIOthersBagList);
}


void CUIStashWnd::InitInventoryBox(CInventoryOwner* pOur, CInventoryBox* pInvBox)
{
	m_pOurOwner = pOur;
	m_pOthersOwner = m_pCar = NULL;
	m_pInventoryBox	= pInvBox;
	m_pInventoryBox->m_in_use = true;

	u16 our_id = smart_cast<CGameObject*>(m_pOurOwner)->ID();

	m_pUICharacterInfoLeft->InitCharacter(our_id);
	m_pUIOthersIcon->Show(false);
	m_pUICharacterInfoRight->ClearInfo();
	m_pUIPropertiesBox->Hide();
	EnableAll();

	UpdateLists();

}


void CUIStashWnd::InitCustomInventory(CInventoryOwner* pOur, CInventoryOwner* pOthers)
{
	m_pOurOwner = pOur;
	m_pOthersOwner = pOthers;
	m_pInventoryBox	= NULL;
	
	u16 our_id = smart_cast<CGameObject*>(m_pOurOwner)->ID();
	u16 other_id = smart_cast<CGameObject*>(m_pOthersOwner)->ID();

	m_pUICharacterInfoLeft->InitCharacter(our_id);
	m_pUIOthersIcon->Show(true);
	
	CBaseMonster *monster = NULL;
	
	if (m_pOthersOwner)
	{
		monster										= smart_cast<CBaseMonster*>(m_pOthersOwner);
		m_pCar										= smart_cast<CCar*>(m_pOthersOwner);
		if (monster || m_pCar || m_pOthersOwner->use_simplified_visual())
		{
			m_pUICharacterInfoRight->ClearInfo		();
			m_pUICharacterInfoRight->SetOwnerID		(other_id);
			m_pUICharacterInfoRight->SetForceUpdate	(true);

			if (monster)
			{
				shared_str monster_tex_name = pSettings->r_string(monster->SectionName(),"icon");
				m_pUICharacterInfoRight->UIIcon().InitTexture(monster_tex_name.c_str());
				m_pUICharacterInfoRight->UIIcon().SetStretchTexture(true);

				if (pSettings->line_exist(monster->SectionName(),"stash_name"))  //skyloader: stash name
				{
					CStringTable stbl;
					string256 str;
					xr_sprintf(str, "%s", *stbl.translate(pSettings->r_string(monster->SectionName(),"stash_name")));
					m_pUICharacterInfoRight->UIName().Show(true);				
					m_pUICharacterInfoRight->UIName().SetText(str);
				}
			} 
			else 
			{
				shared_str car_tex_name = pSettings->r_string(m_pCar->SectionName(),"icon");

				m_pUICharacterInfoRight->UIIcon().InitTexture(car_tex_name.c_str());
				m_pUICharacterInfoRight->UIIcon().SetStretchTexture(true);
			}
		}
		else
		{
			m_pUICharacterInfoRight->InitCharacter(other_id);
		}
	}

	m_pUIPropertiesBox->Hide();
	EnableAll();
	UpdateLists();

	if(!monster && !m_pCar)
	{
		CInfoPortionWrapper	*known_info_registry = xr_new <CInfoPortionWrapper>();
		known_info_registry->registry().init(other_id);
		KNOWN_INFO_VECTOR& known_info = known_info_registry->registry().objects();

		KNOWN_INFO_VECTOR_IT it = known_info.begin();
		for(int i=0;it!=known_info.end();++it,++i)
		{
			(*it).info_id; //??

			m_pOurOwner->OnReceiveInfo((*it).info_id);
		}
		known_info.clear	();
		xr_delete			(known_info_registry);
	}
}  


void CUIStashWnd::BindDragDropListEvents(CUIDragDropListEx* lst)
{
	lst->m_f_item_drop				= CUIDragDropListEx::DRAG_CELL_EVENT(this,&CUIStashWnd::OnItemDragDrop);
	lst->m_f_item_start_drag		= CUIDragDropListEx::DRAG_CELL_EVENT(this,&CUIStashWnd::OnItemStartDrag);
	lst->m_f_item_db_click			= CUIDragDropListEx::DRAG_CELL_EVENT(this,&CUIStashWnd::OnItemDbClick);
	lst->m_f_item_selected			= CUIDragDropListEx::DRAG_CELL_EVENT(this,&CUIStashWnd::OnItemSelected);
	lst->m_f_item_rbutton_click		= CUIDragDropListEx::DRAG_CELL_EVENT(this,&CUIStashWnd::OnItemRButtonClick);
}

void CUIStashWnd::SendMessage(CUIWindow *pWnd, s16 msg, void *pData)
{
	if (msg == BUTTON_CLICKED)
	{
		if(m_pUITakeAll == pWnd)
			TakeAll();
		// our tabs
		else if (pWnd == UIFilterTabsOurs.UITabButton1)
			SetCategoryFilter(icAll, true);
		else if (pWnd == UIFilterTabsOurs.UITabButton2)
			SetCategoryFilter(icWeapons, true);
		else if (pWnd == UIFilterTabsOurs.UITabButton3)
			SetCategoryFilter(icOutfits, true);
		else if (pWnd == UIFilterTabsOurs.UITabButton4)
			SetCategoryFilter(icAmmo, true);
		else if (pWnd == UIFilterTabsOurs.UITabButton5)
			SetCategoryFilter(icMedicine, true);
		else if (pWnd == UIFilterTabsOurs.UITabButton5_2)
			SetCategoryFilter(icFood, true);
		else if (pWnd == UIFilterTabsOurs.UITabButton6)
			SetCategoryFilter(icDevices, true);
		else if (pWnd == UIFilterTabsOurs.UITabButton7)
			SetCategoryFilter(icArtefacts, true);
		else if (pWnd == UIFilterTabsOurs.UITabButton8)
			SetCategoryFilter(icMutantParts, true);
		else if (pWnd == UIFilterTabsOurs.UITabButton9)
			SetCategoryFilter(icMisc, true);
		// their tabs
		else if (pWnd == UIFilterTabsTheir.UITabButton1)
			SetCategoryFilter(icAll, false);
		else if (pWnd == UIFilterTabsTheir.UITabButton2)
			SetCategoryFilter(icWeapons, false);
		else if (pWnd == UIFilterTabsTheir.UITabButton3)
			SetCategoryFilter(icOutfits, false);
		else if (pWnd == UIFilterTabsTheir.UITabButton4)
			SetCategoryFilter(icAmmo, false);
		else if (pWnd == UIFilterTabsTheir.UITabButton5)
			SetCategoryFilter(icMedicine, false);
		else if (pWnd == UIFilterTabsTheir.UITabButton5_2)
			SetCategoryFilter(icFood, false);
		else if (pWnd == UIFilterTabsTheir.UITabButton6)
			SetCategoryFilter(icDevices, false);
		else if (pWnd == UIFilterTabsTheir.UITabButton7)
			SetCategoryFilter(icArtefacts, false);
		else if (pWnd == UIFilterTabsTheir.UITabButton8)
			SetCategoryFilter(icMutantParts, false);
		else if (pWnd == UIFilterTabsTheir.UITabButton9)
			SetCategoryFilter(icMisc, false);
	}
	else if (pWnd == m_pUIPropertiesBox &&	msg == PROPERTY_CLICKED)
	{
		if (m_pUIPropertiesBox->GetClickedItem())
		{
			switch (m_pUIPropertiesBox->GetClickedItem()->GetTAG())
			{
			case INVENTORY_EAT_ACTION:	//съесть объект
				EatItem();
				break;
			case INVENTORY_UNLOAD_MAGAZINE:
			{
				CUICellItem * itm = CurrentItem();
				(smart_cast<CWeaponMagazined*>((CWeapon*)itm->m_pData))->UnloadMagazine(true, m_pOurOwner->object_id(), true);

				for (u32 i = 0; i<itm->ChildsCount(); ++i)
				{
					CUICellItem * child_itm = itm->Child(i);
					(smart_cast<CWeaponMagazined*>((CWeapon*)child_itm->m_pData))->UnloadMagazine(true, m_pOurOwner->object_id(), true);
				}
			}break;
			}
		}
	}

	inherited::SendMessage(pWnd, msg, pData);
}


void CUIStashWnd::UpdateLists_delayed()
{
	m_b_need_update = true;
}

void CUIStashWnd::ShowDialog(bool bDoHideIndicators)
{
	SendInfoToActor("ui_stash_open");

	inherited::ShowDialog(bDoHideIndicators);

	SetCurrentItem(NULL);
	UpdateWeight(*m_pUIOurBagWnd, m_pOurOwner);

	if (m_pUIOthersBagWnd)
		UpdateWeight(*m_pUIOthersBagWnd, m_pOthersOwner);
	else
		UpdateWeight(*m_pUIOthersBagWnd, m_pInventoryBox);
}

void CUIStashWnd::HideDialog()
{
	SendInfoToActor("ui_stash_close");

	m_pUIOurBagList->ClearAll(true);
	m_pUIOthersBagList->ClearAll(true);

	inherited::HideDialog();

	if(m_pInventoryBox)
		m_pInventoryBox->m_in_use = false;

	if (m_pCar)
		m_pCar->CloseTrunkBone();
}

void CUIStashWnd::DisableAll()
{
	m_pUIOurBagWnd->Enable(false);
	m_pUIOthersBagWnd->Enable(false);
}

void CUIStashWnd::EnableAll()
{
	m_pUIOurBagWnd->Enable(true);
	m_pUIOthersBagWnd->Enable(true);
}

CUICellItem* CUIStashWnd::CurrentItem()
{
	return m_pCurrentCellItem;
}

PIItem CUIStashWnd::CurrentIItem()
{
	return	(m_pCurrentCellItem) ? (PIItem)m_pCurrentCellItem->m_pData : NULL;
}

void CUIStashWnd::SetCurrentItem(CUICellItem* itm)
{
	if (m_pCurrentCellItem == itm) return;
	m_pCurrentCellItem = itm;
	m_pUIItemInfo->InitItem(CurrentIItem());
}


void CUIStashWnd::UpdateLists()
{
	int pos = m_pUIOthersBagList->ScrollPos();

	Msg("%s %s %s", m_pOurOwner->cast_game_object()->SectionNameStr(), m_pOthersOwner ? m_pOthersOwner->cast_game_object()->SectionNameStr() : "", m_pInventoryBox ? m_pInventoryBox->SectionNameStr() : "");

	m_pUIOurBagList->ClearAll(true);
	m_pUIOthersBagList->ClearAll(true);

	//Наш рюкзак
	ruck_list.clear();
	m_pOurOwner->inventory().AddAvailableItems(ruck_list, false);

	UpdateBagList(ruck_list, *m_pUIOurBagList, currentFilterOur_, true, true);

	//Чужой рюкзак
	ruck_list.clear();

	if (m_pOthersOwner)
		m_pOthersOwner->inventory().AddAvailableItems(ruck_list, false);
	else
		m_pInventoryBox->AddAvailableItems(ruck_list);

	std::sort(ruck_list.begin(),ruck_list.end(), GreaterRoomInRuck);

	UpdateBagList(ruck_list, *m_pUIOthersBagList, currentFilterTheir_, true, false);

	m_pUIOthersBagList->SetScrollPos(pos);

	UpdateWeight(*m_pUIOurBagWnd, m_pOurOwner);

	if(m_pUIOthersBagWnd)
		UpdateWeight(*m_pUIOthersBagWnd, m_pOthersOwner);
	else
		UpdateWeight(*m_pUIOthersBagWnd, m_pInventoryBox);

	m_b_need_update	= false;

	UIFilterTabsOurs.SetHighlitedTab(currentFilterOur_.get());
	UIFilterTabsTheir.SetHighlitedTab(currentFilterTheir_.get());
}

void CUIStashWnd::UpdateBagList(TIItemContainer& item_list, CUIDragDropListEx& dd, Flags32& filter, bool sort, bool colorize)
{
	dd.ClearAll(true);

	item_list.erase(std::remove_if(item_list.begin(), item_list.end(), SLeaveCategory(filter)), item_list.end());

	if (sort)
		std::sort(item_list.begin(), item_list.end(), GreaterRoomInRuck);

	TIItemContainer::iterator it, it_e;

	for (it = item_list.begin(), it_e = item_list.end(); it != it_e; ++it)
	{
		CUICellItem* cell_itm = create_cell_item(*it);
		dd.SetItem(cell_itm);

		if (colorize)
			ColorizeItem(cell_itm);
	}
}


void CUIStashWnd::Update()
{
	if(	m_b_need_update||
		m_pOurOwner->inventory().ModifyFrame() == CurrentFrame() ||
		(m_pOthersOwner && m_pOthersOwner->inventory().ModifyFrame() == CurrentFrame()))
	{

		int pos1 = m_pUIOurBagList->ScrollPos();
		int pos2 = m_pUIOthersBagList->ScrollPos();

		UpdateLists		();

		m_pUIOurBagList->SetScrollPos(pos1);
		m_pUIOthersBagList->SetScrollPos(pos2);
	}
	
	if (m_pOthersOwner && (smart_cast<CGameObject*>(m_pOurOwner))->Position().distance_to((smart_cast<CGameObject*>(m_pOthersOwner))->Position()) > 3.0f)
	{
		HideDialog();
	}
	inherited::Update();
}

void CUIStashWnd::TakeAll()
{
	SetCategoryFilter(icAll, false);

	u32 cnt = m_pUIOthersBagList->ItemsCount();
	u16 tmp_id = 0;

	if (m_pInventoryBox)
	{
		tmp_id = (smart_cast<CGameObject*>(m_pOurOwner))->ID();
	}

	for (u32 i = 0; i<cnt; ++i)
	{
		CUICellItem* ci = m_pUIOthersBagList->GetItemIdx(i);
		for (u32 j = 0; j<ci->ChildsCount(); ++j)
		{
			PIItem _itm = (PIItem)(ci->Child(j)->m_pData);
			if (m_pOthersOwner)
				TransferItem(_itm, m_pOthersOwner, m_pOurOwner, false);
			else{
				move_item(m_pInventoryBox->ID(), tmp_id, _itm->object().ID());
				//.				Actor()->callback(GameObject::eInvBoxItemTake)( m_pInventoryBox->lua_game_object(), _itm->object().lua_game_object() );
			}

		}
		PIItem itm = (PIItem)(ci->m_pData);
		if (m_pOthersOwner)
			TransferItem(itm, m_pOthersOwner, m_pOurOwner, false);
		else
		{
			move_item(m_pInventoryBox->ID(), tmp_id, itm->object().ID());
			//.			Actor()->callback(GameObject::eInvBoxItemTake)(m_pInventoryBox->lua_game_object(), itm->object().lua_game_object() );
		}

	}
}

#include "../Medkit.h"
#include "../Antirad.h"
#include "../battery.h"

void CUIStashWnd::ActivatePropertiesBox()
{
	if(m_pInventoryBox)	return;

	m_pUIPropertiesBox->RemoveAll();

	CWeaponMagazined*		pWeapon			= smart_cast<CWeaponMagazined*>	(CurrentIItem());
	CEatableItem*			pEatableItem	= smart_cast<CEatableItem*>		(CurrentIItem());
	CMedkit*				pMedkit			= smart_cast<CMedkit*>			(CurrentIItem());
	CAntirad*				pAntirad		= smart_cast<CAntirad*>			(CurrentIItem());
	CBottleItem*			pBottleItem		= smart_cast<CBottleItem*>		(CurrentIItem());
	CBattery*				pBattery		= smart_cast<CBattery*>			(CurrentIItem());

    bool					show_properties = false;
	
	LPCSTR _action = NULL;

	if(pMedkit || pAntirad || pBattery)
	{
		_action	= "st_use";
		show_properties = true;
	}
	else if(pEatableItem)
	{
		if(pBottleItem)
			_action	= "st_drink";
		else
			_action	= "st_eat";

		show_properties = true;
	}
	else if (pWeapon)
	{
		if (smart_cast<CWeaponMagazined*>(pWeapon))
		{
			bool has_ammo = (pWeapon->GetAmmoElapsed() != 0);

			if (!has_ammo)
			{
				CUICellItem* itm = CurrentItem();

				for (u32 i = 0; i<itm->ChildsCount(); ++i)
				{
					pWeapon = smart_cast<CWeaponMagazined*>((CWeapon*)itm->Child(i)->m_pData);

					if (pWeapon->GetAmmoElapsed())
					{
						has_ammo = true;
						break;
					}
				}
			}

			if (has_ammo)
			{
				m_pUIPropertiesBox->AddItem("st_unload_magazine", NULL, INVENTORY_UNLOAD_MAGAZINE);
				show_properties = true;
			}
		}
	}

	if(_action)
		m_pUIPropertiesBox->AddItem(_action,  NULL, INVENTORY_EAT_ACTION);

	if (show_properties)
	{
		m_pUIPropertiesBox->AutoUpdateSize	();
		m_pUIPropertiesBox->BringAllToTop	();

		Fvector2 cursor_pos;
		Frect vis_rect;

		GetAbsoluteRect(vis_rect);
		cursor_pos = GetUICursor().GetCursorPosition();
		cursor_pos.sub(vis_rect.lt);

		m_pUIPropertiesBox->Show(vis_rect, cursor_pos);
	}
}

void CUIStashWnd::EatItem()
{
	if (!CurrentIItem()->Useful())
		return;

	CUIDragDropListEx* owner_list = CurrentItem()->CurrentDDList();

	if (owner_list == m_pUIOthersBagList)
	{
		u16 owner_id = (m_pInventoryBox) ? m_pInventoryBox->ID() : smart_cast<CGameObject*>(m_pOthersOwner)->ID();
		Level().DirectMoveItem(owner_id, Actor()->ID(), CurrentIItem()->object().ID());
	}

	m_pOurOwner->inventory().Eat(CurrentIItem());
}

bool CUIStashWnd::OnItemDragDrop(CUICellItem* itm)
{
	CUIDragDropListEx* old_ddlist = itm->CurrentDDList();
	CUIDragDropListEx* new_ddlist = CUIDragDropListEx::m_drag_item->BackList();

	if (old_ddlist == new_ddlist || !old_ddlist || !new_ddlist || (false && new_ddlist == m_pUIOthersBagList && m_pInventoryBox))
		return true;

	bool res = false;

	if (m_pOthersOwner)
	{
		if(TransferItem
			(CurrentIItem(),
			(old_ddlist == m_pUIOthersBagList) ? m_pOthersOwner : m_pOurOwner,
			(old_ddlist == m_pUIOurBagList) ? m_pOthersOwner : m_pOurOwner,
			(old_ddlist == m_pUIOurBagList)
			)
		)
		{
			res = true;
		}
	}
	else
	{
		u16 tmp_id = (smart_cast<CGameObject*>(m_pOurOwner))->ID();

		bool bMoveDirection = (old_ddlist == m_pUIOthersBagList);

		move_item
			(
			bMoveDirection ? m_pInventoryBox->ID() : tmp_id,
			bMoveDirection ? tmp_id : m_pInventoryBox->ID(),
			CurrentIItem()->object().ID()
			);

		res = true;
		//		Actor()->callback		(GameObject::eInvBoxItemTake)(m_pInventoryBox->lua_game_object(), CurrentIItem()->object().lua_game_object() );
	}

	if (res)
	{
		CUICellItem* ci = old_ddlist->RemoveItem(CurrentItem(), false);

		if (old_ddlist == m_pUIOurBagList)
		{
			if (currentFilterOur_.test(CurrentIItem()->GetInventoryCategory()))
				new_ddlist->SetItem(ci);
		}
		else if (old_ddlist == m_pUIOthersBagList)
		{
			if (currentFilterTheir_.test(CurrentIItem()->GetInventoryCategory()))
				new_ddlist->SetItem(ci);
		}
	}

	SetCurrentItem(NULL);

	return true;
}

bool CUIStashWnd::OnItemStartDrag(CUICellItem* itm)
{
	return false; //default behaviour
}

bool CUIStashWnd::OnItemDbClick(CUICellItem* itm)
{
	CUIDragDropListEx*	old_owner = itm->CurrentDDList();
	CUIDragDropListEx*	new_owner = (old_owner == m_pUIOthersBagList) ? m_pUIOurBagList : m_pUIOthersBagList;

	if (m_pOthersOwner)
	{
		if (TransferItem
			(CurrentIItem(),
			(old_owner == m_pUIOthersBagList) ? m_pOthersOwner : m_pOurOwner,
			(old_owner == m_pUIOurBagList) ? m_pOthersOwner : m_pOurOwner,
			(old_owner == m_pUIOurBagList)
			)
			)
		{
			CUICellItem* ci = old_owner->RemoveItem(CurrentItem(), false);

			if (old_owner == m_pUIOurBagList)
			{
				if (currentFilterOur_.test(CurrentIItem()->GetInventoryCategory()))
					new_owner->SetItem(ci);
			}
			else if (old_owner == m_pUIOthersBagList)
			{
				if (currentFilterTheir_.test(CurrentIItem()->GetInventoryCategory()))
					new_owner->SetItem(ci);
			}
		}
	}
	else
	{
		if (false && old_owner == m_pUIOurBagList)
			return true;

		bool bMoveDirection = (old_owner == m_pUIOthersBagList);

		u16 tmp_id = (smart_cast<CGameObject*>(m_pOurOwner))->ID();
		move_item(bMoveDirection ? m_pInventoryBox->ID() : tmp_id,	bMoveDirection ? tmp_id : m_pInventoryBox->ID(),	CurrentIItem()->object().ID());
		//.		Actor()->callback		(GameObject::eInvBoxItemTake)(m_pInventoryBox->lua_game_object(), CurrentIItem()->object().lua_game_object() );
	}

	SetCurrentItem(NULL);

	return true;
}

bool CUIStashWnd::OnItemSelected(CUICellItem* itm)
{
	if (m_pCurrentCellItem)
		m_pCurrentCellItem->Mark(false);

	if (!itm) return false;

	SetCurrentItem(itm);

	MarkApplicableItems(itm);

	itm->Mark(true);

	return false;
}

bool CUIStashWnd::OnItemRButtonClick(CUICellItem* itm)
{
	SetCurrentItem(itm);
	ActivatePropertiesBox();

	return false;
}

void move_item (u16 from_id, u16 to_id, u16 what_id)
{
	NET_Packet P;
  	CGameObject::u_EventGen(P, GE_OWNERSHIP_REJECT, from_id);

	P.w_u16(what_id);
	P.w_u8(1); // send just_before_destroy flag, so physical shell does not activates and disrupts nearby objects
	CGameObject::u_EventSend(P);

	//другому инвентарю - взять вещь 
	CGameObject::u_EventGen(P, GE_OWNERSHIP_TAKE, to_id);
	P.w_u16(what_id);
	CGameObject::u_EventSend(P);

}

#include "../UIGameCustom.h"

bool CUIStashWnd::TransferItem(PIItem itm, CInventoryOwner* owner_from, CInventoryOwner* owner_to, bool b_check)
{
	VERIFY									(NULL==m_pInventoryBox);
	CGameObject* go_from					= smart_cast<CGameObject*>(owner_from);
	CGameObject* go_to						= smart_cast<CGameObject*>(owner_to);

	if(smart_cast<CBaseMonster*>(go_to))	return false;
	if(b_check)
	{
		float invWeight						= owner_to->inventory().CalcTotalWeight();
		float maxWeight						= owner_to->inventory().GetMaxWeight();
		float itmWeight						= itm->Weight();

		if (invWeight + itmWeight > maxWeight + EPS_S)
		{
			SDrawStaticStruct* HudMessage = CurrentGameUI()->AddCustomStatic("inv_hud_message", true);
			HudMessage->m_endTime = EngineTime() + 3.0f;

			string1024 str;
			xr_sprintf(str, "%s", *CStringTable().translate("st_weight_refuse"));

			HudMessage->wnd()->TextItemControl()->SetText(str);

			return false;
		}
	}

	move_item(go_from->ID(), go_to->ID(), itm->object().ID());

	return true;
}

void CUIStashWnd::MarkApplicableItems(CUICellItem* itm)
{
	CInventoryItem* inventoryitem = (CInventoryItem*)itm->m_pData;
	if (!inventoryitem) return;

	u32 color = pSettings->r_color("inventory_color_ammo", "color");

	CWeaponMagazined* weapon = smart_cast<CWeaponMagazined*>(inventoryitem);
	xr_vector<shared_str> addons;

	if (weapon)
		addons = GetAddonsForWeapon(weapon);

	MarkItems(weapon, addons, *m_pUIOurBagList, color, true);
	MarkItems(weapon, addons, *m_pUIOthersBagList, color);
}

void CUIStashWnd::MarkItems(CWeaponMagazined* weapon, const xr_vector<shared_str>& addons, CUIDragDropListEx& list, u32 ammoColor, bool colorize)
{
	u32 item_count = list.ItemsCount();

	CWeaponMagazinedWGrenade* weapon_with_gl = smart_cast<CWeaponMagazinedWGrenade*>(weapon);

	for (u32 i = 0; i<item_count; ++i)
	{
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
		if (colorize)
		{
			ColorizeItem(cell_item);
		}
	}
}

void CUIStashWnd::ColorizeItem(CUICellItem* itm)
{
	PIItem iitem = (PIItem)itm->m_pData;
	if (iitem->m_eItemPlace == eItemPlaceSlot || iitem->m_eItemPlace == eItemPlaceBelt)
		itm->SetTextureColor(color_rgba(100,255,100,255));
}

#include "../xr_level_controller.h"

bool CUIStashWnd::OnKeyboardAction(int dik, EUIMessages keyboard_action)
{
	if (inherited::OnKeyboardAction(dik, keyboard_action))return true;

	if (keyboard_action == WINDOW_KEY_PRESSED)
	{
		if (is_binded(kUSE, dik) || is_binded(kQUIT, dik))
		{
			HideDialog();
			return true;
		}
		if (DIK_LSHIFT == dik)
		{
			TakeAll();
			return true;
		}
	}
	return false;
}

void CUIStashWnd::SetCategoryFilter(u32 mask, bool our)
{
	Flags32& filter = our ? currentFilterOur_ : currentFilterTheir_;

	if (our)
	{
		ruck_list.clear();

		m_pOurOwner->inventory().AddAvailableItems(ruck_list, false);
	}
	else
	{
		ruck_list.clear();

		if (m_pOthersOwner)
			m_pOthersOwner->inventory().AddAvailableItems(ruck_list, false);
		else
			m_pInventoryBox->AddAvailableItems(ruck_list);
	}

	CUIDragDropListEx& dd = our ? *m_pUIOurBagList : *m_pUIOthersBagList;

	filter.zero();
	filter.set(mask, true);

	if (IsShown())
		UpdateBagList(ruck_list, dd, filter, true, our);

	if (our)
		UIFilterTabsOurs.SetHighlitedTab(mask);
	else
		UIFilterTabsTheir.SetHighlitedTab(mask);
}