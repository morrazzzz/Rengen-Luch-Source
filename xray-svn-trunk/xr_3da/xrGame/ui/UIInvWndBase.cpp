#include "pch_script.h"
#include "UIInventoryWnd.h"

#include "xrUIXmlParser.h"
#include "UIXmlInit.h"
#include "UIHelper.h"

#include "../InventoryOwner.h"
#include "../uigamesp.h"
#include "../hudmanager.h"

#include "../inventory.h"

#include "UIInventoryUtilities.h"

#include "../level.h"

#include "UIDragDropListEx.h"
#include "UIDragDropReferenceList.h"

#include "UIOutfitSlot.h"
#include "UI3tButton.h"

#include "UICellItemFactory.h"

#include "../string_table.h"
#include "../actor.h"


#ifdef DEBUG
	#include "../xr_level_controller.h"
	#include <dinput.h>
#endif

using namespace InventoryUtilities;

CUIInventoryWnd::CUIInventoryWnd()
{
	m_iCurrentActiveSlot = NO_ACTIVE_SLOT;
	Init();
	SetCurrentItem(NULL);

	m_b_need_reinit = false;
	wasCrouchedBeforeOpen_ = false;
	wasSprintingBeforeOpen_ = false;
	needWepaonUnholster_ = false;

	currentFilter_.set(icAll, true);
}

CUIInventoryWnd::~CUIInventoryWnd()
{
	ClearAllLists();
}


void CUIInventoryWnd::Init()
{
	CUIXml uiXml;

	if (!ui_hud_type)
		ui_hud_type = 1;

	string128		INVENTORY_ITEM_XML;
	xr_sprintf		(INVENTORY_ITEM_XML, "inventory_item_%d.xml", ui_hud_type);

	string128		INVENTORY_XML;
	xr_sprintf		(INVENTORY_XML, "inventory_new_%d.xml", ui_hud_type);

	bool xml_result	= uiXml.Load(CONFIG_PATH, UI_PATH, INVENTORY_XML);
	R_ASSERT3(xml_result, "file parsing error ", uiXml.m_xml_file_name);

	CUIXmlInit xml_init;

	xml_init.InitWindow(uiXml, "main", 0, this);

	AttachChild(&UIBeltSlots);
	xml_init.InitStatic(uiXml, "belt_slots", 0, &UIBeltSlots);

	AttachChild(&UIBack);
	xml_init.InitStatic(uiXml, "back", 0, &UIBack);

	AttachChild(&UIStaticBottom);
	xml_init.InitStatic(uiXml, "bottom_static", 0, &UIStaticBottom);

	AttachChild(&UIBagWnd);
	xml_init.InitStatic(uiXml, "bag_static", 0, &UIBagWnd);

	AttachChild(&UIMoneyWnd);
	xml_init.InitStatic(uiXml, "money_static", 0, &UIMoneyWnd);

	AttachChild(&UIDescrWnd);
	xml_init.InitStatic(uiXml, "descr_static", 0, &UIDescrWnd);

	UIDescrWnd.AttachChild(&UIItemInfo);
	UIItemInfo.InitItemInfo(Fvector2().set(0, 0), Fvector2().set(UIDescrWnd.GetWidth(), UIDescrWnd.GetHeight()), INVENTORY_ITEM_XML);

	AttachChild(&UIPersonalWnd);
	xml_init.InitFrameWindow(uiXml, "character_frame_window", 0, &UIPersonalWnd);

	AttachChild(&UIProgressBack);
	xml_init.InitStatic(uiXml, "progress_background", 0, &UIProgressBack);

	UIProgressBack.AttachChild(&UIProgressBarHealth);
	xml_init.InitProgressBar(uiXml, "progress_bar_health", 0, &UIProgressBarHealth);

	UIProgressBack.AttachChild(&UIProgressBarBleeding);
	xml_init.InitProgressBar(uiXml, "progress_bar_bleed", 0, &UIProgressBarBleeding);

	UIProgressBack.AttachChild(&UIProgressBarStamina);
	xml_init.InitProgressBar(uiXml, "progress_bar_stamina", 0, &UIProgressBarStamina);

	UIProgressBack.AttachChild(&UIProgressBarArmor);
	xml_init.InitProgressBar(uiXml, "progress_bar_armor", 0, &UIProgressBarArmor);

	UIProgressBack.AttachChild(&UIProgressBarRadiation);
	xml_init.InitProgressBar(uiXml, "progress_bar_radiation", 0, &UIProgressBarRadiation);

	UIProgressBack.AttachChild(&UIProgressBarHunger);
	xml_init.InitProgressBar(uiXml, "progress_bar_hunger", 0, &UIProgressBarHunger);

	UIProgressBack.AttachChild(&UIProgressBarMozg);
	xml_init.InitProgressBar(uiXml, "progress_bar_mozg", 0, &UIProgressBarMozg);

	UIProgressBack.AttachChild(&UIProgressBarThirst);
	xml_init.InitProgressBar(uiXml, "progress_bar_thirst", 0, &UIProgressBarThirst);

	UIPersonalWnd.AttachChild(&UIStaticPersonal);
	xml_init.InitStatic(uiXml, "static_personal",0, &UIStaticPersonal);

	AttachChild(&UIOutfitInfo);
	UIOutfitInfo.InitFromXml(uiXml);

	//Ёлементы автоматического добавлени€
	xml_init.InitAutoStatic(uiXml, "auto_static", this);

	shared_str nodevalue = uiXml.Read("build_style_outfit_slot", 0, "false");
	bool buildStyleOutfitSlot = nodevalue == "true" ? 1 : 0;

	m_pUIOutfitList =		InitDragDropList(uiXml, "dragdrop_outfit", 0, this, buildStyleOutfitSlot ? xr_new <CUIOutfitDragDropList>() : nullptr);

	m_pUIBagList =			InitDragDropList(uiXml, "dragdrop_bag", 0, &UIBagWnd);
	m_pUIBeltList =			InitDragDropList(uiXml, "dragdrop_belt", 0);
	m_pUIArtefactBeltList =	InitDragDropList(uiXml, "dragdrop_artefact_belt", 0);
	m_pUIPistolList =		InitDragDropList(uiXml, "dragdrop_pistol", 0);
	m_pUIAutomaticList =	InitDragDropList(uiXml, "dragdrop_automatic", 0);

	// —оздать быстрые слоты
	CreateQuickSlots(uiXml, &xml_init);

	// load second rifle slot if it is defined in XML (removing it will effectively disable the second rifle slot feature)
	int numRifleSLots = uiXml.GetNodesNum(uiXml.GetRoot(), "dragdrop_automatic");
	if (numRifleSLots > 1)
	{
		m_pUIAutomatic2List = InitDragDropList(uiXml, "dragdrop_automatic", 1);
	}
	else
		m_pUIAutomatic2List = nullptr;

	m_pUIKnifeList =		InitDragDropList(uiXml, "dragdrop_knife", 0);
	m_pUIBinocularList =	InitDragDropList(uiXml, "dragdrop_binocular", 0);
	m_pUITorchList =		InitDragDropList(uiXml, "dragdrop_torch", 0);
	m_pUIDetectorList =		InitDragDropList(uiXml, "dragdrop_detecor", 0);
	m_pUIHelmetList =		InitDragDropList(uiXml, "dragdrop_helmets", 0);
	m_pUIPNVList =			InitDragDropList(uiXml, "dragdrop_pnv", 0);
	m_pUIAnomDetectorList = InitDragDropList(uiXml, "dragdrop_anomaly_detector", 0);

	//pop-up menu
	AttachChild(&UIPropertiesBox);
	UIPropertiesBox.InitPropertiesBox(Fvector2().set(0, 0), Fvector2().set(300, 300));
	UIPropertiesBox.Hide();

	AttachChild(&UIStaticTime);
	xml_init.InitStatic(uiXml, "time_static", 0, &UIStaticTime);

	UIStaticTime.AttachChild(&UIStaticTimeString);
	xml_init.InitTextWnd(uiXml, "time_static_str", 0, &UIStaticTimeString);

	UIExitButton = xr_new <CUI3tButton>(); UIExitButton->SetAutoDelete(true);
	AttachChild(UIExitButton);
	xml_init.Init3tButton(uiXml, "exit_button", 0, UIExitButton);

	// Init item category filter tabs
	AttachChild(&UIFilterTabs);
	UIFilterTabs.InitFromXml(uiXml, "filter_tabs_holder");

	//Init all fake slot blocking textures
	InitSlotBlockers(&uiXml, &xml_init);

	//Load sounds
	XML_NODE* stored_root = uiXml.GetLocalRoot();
	uiXml.SetLocalRoot(uiXml.NavigateToNode("action_sounds", 0));

	::Sound->create(sounds[eInvSndOpen],		uiXml.Read("snd_open", 0, NULL), st_Effect, sg_SourceType);
	::Sound->create(sounds[eInvSndClose],		uiXml.Read("snd_close", 0, NULL), st_Effect, sg_SourceType);
	::Sound->create(sounds[eInvItemToSlot],		uiXml.Read("snd_item_to_slot", 0, NULL), st_Effect, sg_SourceType);
	::Sound->create(sounds[eInvItemToBelt],		uiXml.Read("snd_item_to_belt", 0, NULL), st_Effect, sg_SourceType);
	::Sound->create(sounds[eInvItemToRuck],		uiXml.Read("snd_item_to_ruck", 0, NULL), st_Effect, sg_SourceType);
	::Sound->create(sounds[eInvProperties],		uiXml.Read("snd_properties", 0, NULL), st_Effect, sg_SourceType);
	::Sound->create(sounds[eInvDropItem],		uiXml.Read("snd_drop_item", 0, NULL), st_Effect, sg_SourceType);
	::Sound->create(sounds[eInvAttachAddon],	uiXml.Read("snd_attach_addon", 0, NULL), st_Effect, sg_SourceType);
	::Sound->create(sounds[eInvDetachAddon],	uiXml.Read("snd_detach_addon", 0, NULL), st_Effect, sg_SourceType);
	::Sound->create(sounds[eInvItemUse],		uiXml.Read("snd_item_use", 0, NULL), st_Effect, sg_SourceType);

	uiXml.SetLocalRoot(stored_root);

	xr_strcpy(lua_is_reapirable_func, pSettings->r_string("lost_alpha_cfg", "on_checking_repair_wpn"));
	xr_strcpy(lua_on_repair_wpn_func, pSettings->r_string("lost_alpha_cfg", "on_repair_wpn_clicked"));
}


CUIDragDropListEx* CUIInventoryWnd::InitDragDropList(CUIXml& uiXml, LPCSTR name, int index, CUIWindow* parent, CUIDragDropListEx* list)
{
	if (!list) list = xr_new <CUIDragDropListEx>();
	if (!parent) parent = this;
	parent->AttachChild(list);
	list->SetAutoDelete(true);
	CUIXmlInit::InitDragDropListEx(uiXml, name, index, list);
	BindDragDropListEvents(list);

	return list;
}


void CUIInventoryWnd::InitInventory()
{
	CInventoryOwner *pInvOwner = smart_cast<CInventoryOwner*>(Level().CurrentEntity());

	if (!pInvOwner)
		return;

	Physical_Inventory = &pInvOwner->inventory();

	UIPropertiesBox.Hide();
	ClearAllLists();
	m_pMouseCapturer = NULL;
	SetCurrentItem(NULL);

	// Slots
	InitSlotItem(m_pUIPistolList, PISTOL_SLOT);
	InitSlotItem(m_pUIAutomaticList, RIFLE_SLOT);
	if (m_pUIAutomatic2List)
	{
		InitSlotItem(m_pUIAutomatic2List, RIFLE_2_SLOT);
	}
	InitSlotItem(m_pUIKnifeList, KNIFE_SLOT);
	InitSlotItem(m_pUIBinocularList, APPARATUS_SLOT);
	InitSlotItem(m_pUITorchList, TORCH_SLOT);
	InitSlotItem(m_pUIDetectorList, DETECTOR_SLOT);
	InitSlotItem(m_pUIHelmetList, HELMET_SLOT);
	InitSlotItem(m_pUIPNVList, PNV_SLOT);
	InitSlotItem(m_pUIAnomDetectorList, ANOM_DET_SLOT);
	InitSlotItem(m_pUIOutfitList, OUTFIT_SLOT);

	// Belt
	TIItemContainer::iterator it, it_e;
	for (it = Physical_Inventory->belt_.begin(), it_e = Physical_Inventory->belt_.end(); it != it_e; ++it)
	{
		CUICellItem* cell_itm = create_cell_item(*it);
		m_pUIBeltList->SetItem(cell_itm);
	}

	// Artefact Belt
	for (it = Physical_Inventory->artefactBelt_.begin(), it_e = Physical_Inventory->artefactBelt_.end(); it != it_e; ++it)
	{
		CUICellItem* cell_itm = create_cell_item(*it);
		m_pUIArtefactBeltList->SetItem(cell_itm);
	}

	// Ruck
	UpdateBagList();

	UpdateSlotBlockers();

	quickUseSlots_->ReloadReferences();
	quickUseSlots_->Show(true);

	m_b_need_reinit = false;

	//Update quick slots hot key indicators
	quickSlotsHotKeyIndicators_[0]->SetText(CStringTable().translate("st_qs_hot_key_indicator_1").c_str());
	quickSlotsHotKeyIndicators_[1]->SetText(CStringTable().translate("st_qs_hot_key_indicator_2").c_str());
	quickSlotsHotKeyIndicators_[2]->SetText(CStringTable().translate("st_qs_hot_key_indicator_3").c_str());
	quickSlotsHotKeyIndicators_[3]->SetText(CStringTable().translate("st_qs_hot_key_indicator_4").c_str());

	UIFilterTabs.SetHighlitedTab(currentFilter_.get());
}


void CUIInventoryWnd::ReinitInventoryWnd()
{
	m_b_need_reinit = true;
}

void CUIInventoryWnd::InitSlotItem(CUIDragDropListEx* list, TSlotId slot)
{
	PIItem _itm = Physical_Inventory->ItemFromSlot(slot);
	if (_itm)
	{
		list->SetItem(create_cell_item(_itm));
	}
}

void CUIInventoryWnd::InitSlotBlockers(CUIXml* xml, CUIXmlInit* xml_init)
{
	CreateSlotBlockersFor(m_pUIArtefactBeltList, "art_slot_blocker_template", UI_artSlotBlockers, xml, xml_init);
	CreateSlotBlockersFor(m_pUIBeltList, "ammo_slot_blocker_template", UI_ammoSlotBlockers, xml, xml_init);
	CreateSlotBlockerFor(m_pUIHelmetList, "helmet_slot_blocker", UI_helmetSlotBlocker, xml, xml_init);
	CreateSlotBlockerFor(m_pUIPNVList, "pnv_slot_blocker", UI_pnvSlotBlocker, xml, xml_init);
}

void CUIInventoryWnd::CreateSlotBlockersFor(CUIDragDropListEx* dd_list, string256 blocker_node, xr_vector<CUIStatic*>& where_to_store, CUIXml* xml, CUIXmlInit* xml_init)
{
	//Note: No scroll bar support. Exact fit only. No support for items with inv_grid_height more than "1"

	for (int y = 0; y < dd_list->CellsCapacity().y; y++)
	{
		for (int x = 0; x < dd_list->CellsCapacity().x; x++)
		{
			CUIStatic* slot_blocker = xr_new <CUIStatic>();
			AttachChild(slot_blocker);
			slot_blocker->SetAutoDelete(true);
			xml_init->InitStatic(*xml, blocker_node, 0, slot_blocker);

			//Get position of cell
			Fvector2 position;
			position.x = (dd_list->CellSize().x + dd_list->CellsSpacing().x) * x + dd_list->GetWndPos().x;
			position.y = (dd_list->CellSize().y + dd_list->CellsSpacing().y) * y + dd_list->GetWndPos().y;

			//Get size of cell
			Fvector2 cell_size;
			cell_size.x = (float)dd_list->CellSize().x;
			cell_size.y = (float)dd_list->CellSize().y;

			//Assign cell position and size to slot blocker
			slot_blocker->SetWndPos(position);
			slot_blocker->SetWndSize(cell_size);

			where_to_store.push_back(slot_blocker);
		}
	}

}

void CUIInventoryWnd::CreateSlotBlockerFor(CUIDragDropListEx* dd_list, string256 blocker_node, CUIStatic*& ui_static, CUIXml* xml, CUIXmlInit* xml_init)
{
	CUIStatic* slot_blocker = xr_new <CUIStatic>();
	AttachChild(slot_blocker);
	slot_blocker->SetAutoDelete(true);
	xml_init->InitStatic(*xml, blocker_node, 0, slot_blocker);

	//Get size of cell
	Fvector2 cell_size;

	cell_size.x = (float)dd_list->GetWndSize().x;
	cell_size.y = (float)dd_list->GetWndSize().y;

	//Assign cell position and size to slot blocker
	slot_blocker->SetWndPos(dd_list->GetWndPos());
	slot_blocker->SetWndSize(cell_size);

	ui_static = slot_blocker;
}

void CUIInventoryWnd::CreateQuickSlots(CUIXml& xml, CUIXmlInit* xml_init)
{
	quickUseSlots_ = UIHelper::CreateDragDropReferenceList(xml, "dragdrop_quick_slots", this);
	quickUseSlots_->Initialize();
	
	BindDragDropListEvents(quickUseSlots_);

	// Init hot key indicators
	for (u8 i = 0; i < quickUseSlots_->CellsCapacity().x; ++i)
	{
		CUIStatic* hot_key_indicator = xr_new <CUIStatic>();
		AttachChild(hot_key_indicator);
		hot_key_indicator->SetAutoDelete(true);

		string256 node_name;
		xr_sprintf(node_name, "hot_key_indicator_%u", i);
		xml_init->InitStatic(xml, node_name, 0, hot_key_indicator);

		quickSlotsHotKeyIndicators_.push_back(hot_key_indicator);
	}
}

void CUIInventoryWnd::BindDragDropListEvents(CUIDragDropListEx* lst)
{
	lst->m_f_item_drop				= CUIDragDropListEx::DRAG_CELL_EVENT(this,&CUIInventoryWnd::OnItemDragDrop);
	lst->m_f_item_start_drag		= CUIDragDropListEx::DRAG_CELL_EVENT(this,&CUIInventoryWnd::OnItemStartDrag);
	lst->m_f_item_db_click			= CUIDragDropListEx::DRAG_CELL_EVENT(this,&CUIInventoryWnd::OnItemDbClick);
	lst->m_f_item_selected			= CUIDragDropListEx::DRAG_CELL_EVENT(this,&CUIInventoryWnd::OnItemSelected);
	lst->m_f_item_rbutton_click		= CUIDragDropListEx::DRAG_CELL_EVENT(this,&CUIInventoryWnd::OnItemRButtonClick);
}

void CUIInventoryWnd::SendMessage(CUIWindow *pWnd, s16 msg, void *pData)
{
	if (pWnd == &UIPropertiesBox &&	msg == PROPERTY_CLICKED)
	{
		ProcessPropertiesBoxClicked();
	}
	else if (msg == BUTTON_CLICKED)
	{
		if (pWnd == UIExitButton)
			HideDialog();
		else if (pWnd == UIFilterTabs.UITabButton1)
			SetCategoryFilter(icAll);
		else if (pWnd == UIFilterTabs.UITabButton2)
			SetCategoryFilter(icWeapons);
		else if (pWnd == UIFilterTabs.UITabButton3)
			SetCategoryFilter(icOutfits);
		else if (pWnd == UIFilterTabs.UITabButton4)
			SetCategoryFilter(icAmmo);
		else if (pWnd == UIFilterTabs.UITabButton5)
			SetCategoryFilter(icMedicine);
		else if (pWnd == UIFilterTabs.UITabButton5_2)
			SetCategoryFilter(icFood);
		else if (pWnd == UIFilterTabs.UITabButton6)
			SetCategoryFilter(icDevices);
		else if (pWnd == UIFilterTabs.UITabButton7)
			SetCategoryFilter(icArtefacts);
		else if (pWnd == UIFilterTabs.UITabButton8)
			SetCategoryFilter(icMutantParts);
		else if (pWnd == UIFilterTabs.UITabButton9)
			SetCategoryFilter(icMisc);
	}

	CUIWindow::SendMessage(pWnd, msg, pData);
}

void CUIInventoryWnd::ClearAllLists()
{
	m_pUIBagList->ClearAll(true);
	m_pUIBeltList->ClearAll(true);
	m_pUIArtefactBeltList->ClearAll(true);

	m_pUIOutfitList->ClearAll(true);

	m_pUIPistolList->ClearAll(true);
	m_pUIAutomaticList->ClearAll(true);
	if (m_pUIAutomatic2List)
	{
		m_pUIAutomatic2List->ClearAll(true);
	}
	m_pUIKnifeList->ClearAll(true);
	m_pUIBinocularList->ClearAll(true);
	m_pUITorchList->ClearAll(true);
	m_pUIDetectorList->ClearAll(true);
	m_pUIHelmetList->ClearAll(true);
	m_pUIPNVList->ClearAll(true);
	m_pUIAnomDetectorList->ClearAll(true);
	quickUseSlots_->ClearAll(true);
}

void CUIInventoryWnd::UpdateBagList(bool sort)
{
	m_pUIBagList->ClearAll(true);

	ruck_list = Physical_Inventory->ruck_;

	ruck_list.erase(std::remove_if(ruck_list.begin(), ruck_list.end(), SLeaveCategory(currentFilter_)), ruck_list.end());

	if (sort)
		std::sort(ruck_list.begin(), ruck_list.end(), GreaterRoomInRuck);

	TIItemContainer::iterator it, it_e;

	for (it = ruck_list.begin(), it_e = ruck_list.end(); it != it_e; ++it)
	{
		CUICellItem* cell_itm = create_cell_item(*it);
		m_pUIBagList->SetItem(cell_itm);
	}
}

CUIDragDropListEx* CUIInventoryWnd::GetSlotList(TSlotId slot_idx)
{
	if (slot_idx == NO_ACTIVE_SLOT || Physical_Inventory->m_slots[slot_idx].m_bPersistent) return NULL;
	switch (slot_idx)
	{
	case PISTOL_SLOT:
		return m_pUIPistolList;
		break;

	case RIFLE_SLOT:
		return m_pUIAutomaticList;
		break;

	case RIFLE_2_SLOT:
		return m_pUIAutomatic2List;
		break;

	case KNIFE_SLOT:
		return m_pUIKnifeList;
		break;

	case APPARATUS_SLOT:
		return m_pUIBinocularList;
		break;

	case TORCH_SLOT:
		return m_pUITorchList;
		break;

	case OUTFIT_SLOT:
		return m_pUIOutfitList;
		break;

	case DETECTOR_SLOT:
		return m_pUIDetectorList;
		break;

	case HELMET_SLOT:
		return m_pUIHelmetList;
		break;

	case PNV_SLOT:
		return m_pUIPNVList;
		break;

	case ANOM_DET_SLOT:
		return m_pUIAnomDetectorList;
		break;
	};
	return NULL;
}


CUICellItem* CUIInventoryWnd::CurrentItem()
{
	return m_pCurrentCellItem;
}

PIItem CUIInventoryWnd::CurrentIItem()
{
	return	(m_pCurrentCellItem) ? (PIItem)m_pCurrentCellItem->m_pData : NULL;
}

void CUIInventoryWnd::SetCurrentItem(CUICellItem* itm)
{
	if (m_pCurrentCellItem == itm) return;

	m_pCurrentCellItem = itm;

	UIItemInfo.InitItem(CurrentIItem());
}

void CUIInventoryWnd::ShowDialog(bool bDoHideIndicators)
{
	InitInventory();
	inherited::ShowDialog(bDoHideIndicators);

	SendInfoToActor("ui_inventory");

	Update();
	PlaySnd(eInvSndOpen);

	if ((Actor()->MovingState()&mcSprint))
		wasSprintingBeforeOpen_ = true;

	Actor()->BreakSprint();

	if (!(Actor()->MovingState()&mcCrouch))
	{
		Actor()->IR_OnKeyboardPress(kCROUCH_TOGGLE);

		wasCrouchedBeforeOpen_ = false;
	}
	else
		wasCrouchedBeforeOpen_ = true;

	Actor()->inventoryLimitsMoving_ = true;

	if (Physical_Inventory->GetActiveSlot() != NO_ACTIVE_SLOT)
		needWepaonUnholster_ = true;
	else
		needWepaonUnholster_ = false;

	Actor()->SetWeaponHideState(INV_STATE_BLOCK_ALL, true);
}

void CUIInventoryWnd::HideDialog()
{
	PlaySnd(eInvSndClose);
	inherited::HideDialog();

	SendInfoToActor("ui_inventory_hide");
	ClearAllLists();

	Actor()->SetWeaponHideState(INV_STATE_BLOCK_ALL, false, needWepaonUnholster_);

	//достать вещь в активный слот
	if (m_iCurrentActiveSlot != NO_ACTIVE_SLOT && Physical_Inventory->m_slots[m_iCurrentActiveSlot].m_pIItem)
	{
		Physical_Inventory->ActivateSlot(m_iCurrentActiveSlot);
		m_iCurrentActiveSlot = NO_ACTIVE_SLOT;
	}

	Actor()->inventoryLimitsMoving_ = false;

	if (!wasCrouchedBeforeOpen_)
		Actor()->IR_OnKeyboardPress(kCROUCH_TOGGLE);

	if (wasSprintingBeforeOpen_)
	{
		Actor()->IR_OnKeyboardPress(kSPRINT_TOGGLE);

		wasSprintingBeforeOpen_ = false;
	}
}

TSlotId CUIInventoryWnd::GetSlot(CUIDragDropListEx* l, CUICellItem* itm)
{
	if (l == m_pUIAutomatic2List)
		return RIFLE_2_SLOT;
	auto iitm = (PIItem)itm->m_pData;

	return iitm->BaseSlot();
}

bool CUIInventoryWnd::SlotIsCompatible(TSlotId desiredSlot, CUICellItem* itm)
{
	auto iitm = (PIItem)itm->m_pData;
	auto baseSlot = iitm->BaseSlot();

	return baseSlot == desiredSlot
		|| (baseSlot == RIFLE_SLOT && desiredSlot == RIFLE_2_SLOT)
		|| (baseSlot == RIFLE_2_SLOT && desiredSlot == RIFLE_SLOT);
}

bool CUIInventoryWnd::CanPutInSlot(TSlotId desiredSlot, CUICellItem* itm)
{
	auto iitem = (PIItem)itm->m_pData;
	return !Physical_Inventory->m_slots[desiredSlot].m_bPersistent && Physical_Inventory->CanPutInSlot(iitem, desiredSlot);
}

EListType CUIInventoryWnd::GetType(CUIDragDropListEx* l)
{
	if (l == m_pUIBagList)					return iwBag;
	if (l == m_pUIBeltList)					return iwBelt;
	if (l == m_pUIArtefactBeltList)			return iwArtefactBelt;

	if (l == m_pUIAutomaticList)			return iwSlot;
	if (l == m_pUIAutomatic2List)			return iwSlot;
	if (l == m_pUIPistolList)				return iwSlot;
	if (l == m_pUIOutfitList)				return iwSlot;
	if (l == m_pUIKnifeList)				return iwSlot;
	if (l == m_pUIBinocularList)			return iwSlot;
	if (l == m_pUITorchList)				return iwSlot;
	if (l == m_pUIDetectorList)				return iwSlot;
	if (l == m_pUIHelmetList)				return iwSlot;
	if (l == m_pUIPNVList)					return iwSlot;
	if (l == m_pUIAnomDetectorList)			return iwSlot;
	if (l == quickUseSlots_)				return iwQuickSlot;

	NODEFAULT;
#ifdef DEBUG
	return iwSlot;
#endif
}

void CUIInventoryWnd::DeleteFromInventory(PIItem pIItem)
{
	NET_Packet P;
	pIItem->object().u_EventGen	(P, GE_OWNERSHIP_REJECT, pIItem->object().H_Parent()->ID());
	P.w_u16(u16(pIItem->object().ID()));
	P.w_u8(1); // send just_before_destroy flag, so physical shell does not activates and disrupts nearby objects
	pIItem->object().u_EventSend(P);

	pIItem->object().u_EventGen(P, GE_DESTROY, u16(pIItem->object().ID()));
	pIItem->object().u_EventSend(P);
}

bool CUIInventoryWnd::OnMouseAction(float x, float y, EUIMessages mouse_action)
{
	if (m_b_need_reinit)
		return true;

	//вызов дополнительного меню по правой кнопке
	if (mouse_action == WINDOW_RBUTTON_DOWN)
	{
		if (UIPropertiesBox.IsShown())
		{
			UIPropertiesBox.Hide();
			return true;
		}
	}

	CUIWindow::OnMouseAction(x, y, mouse_action);

	return true; // always returns true, because ::StopAnyMove() == true;
}

bool CUIInventoryWnd::OnKeyboardAction(int dik, EUIMessages keyboard_action)
{
	if(m_b_need_reinit)
		return true;

	if (UIPropertiesBox.GetVisible())
		UIPropertiesBox.OnKeyboardAction(dik, keyboard_action);

	if ( is_binded(kDROP, dik) )
	{
		if(WINDOW_KEY_PRESSED==keyboard_action)
			DropCurrentItem(false);
		return true;
	}

	if (WINDOW_KEY_PRESSED == keyboard_action)
	{
#ifdef DEBUG
		if(DIK_NUMPAD7 == dik && CurrentIItem())
		{
			CurrentIItem()->ChangeCondition(-0.05f);
			UIItemInfo.InitItem(CurrentIItem());
		}
		else if(DIK_NUMPAD8 == dik && CurrentIItem())
		{
			CurrentIItem()->ChangeCondition(0.05f);
			UIItemInfo.InitItem(CurrentIItem());
		}
#endif
	}
	if( inherited::OnKeyboardAction(dik,keyboard_action) )return true;

	return false;
}
