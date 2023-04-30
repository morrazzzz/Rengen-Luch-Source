#include "stdafx.h"
#include "UITradeWnd.h"

#include "xrUIXmlParser.h"
#include "UIXmlInit.h"

#include "../Entity.h"
#include "../HUDManager.h"
#include "../Actor.h"
#include "../Trade.h"
#include "../UIGameSP.h"
#include "UIInventoryUtilities.h"
#include "../inventoryowner.h"
#include "../inventory.h"
#include "../level.h"
#include "../string_table.h"
#include "UIMultiTextStatic.h"
#include "UI3tButton.h"
#include "UIItemInfo.h"

#include "UICharacterInfo.h"
#include "UIDragDropListEx.h"
#include "UICellItem.h"
#include "UICellItemFactory.h"

#include "../WeaponMagazined.h"
#include "../weaponmagazinedwgrenade.h"
#include "../GameConstants.h"

using namespace InventoryUtilities;

struct CUITradeInternal
{
	CUIStatic			UIStaticTop;
	CUIStatic			UIStaticBottom;

	CUIStatic			UIOurBagWnd;
	CUIStatic			UIOurMoneyStatic;
	CUIStatic			UIOthersBagWnd;
	CUIStatic			UIOtherMoneyStatic;
	CUIDragDropListEx	UIOurBagList;
	CUIDragDropListEx	UIOthersBagList;

	CUIStatic			UIOurTradeWnd;
	CUIStatic			UIOthersTradeWnd;
	CUIMultiTextStatic	UIOurPriceCaption;
	CUIMultiTextStatic	UIOthersPriceCaption;
	CUIDragDropListEx	UIOurTradeList;
	CUIDragDropListEx	UIOthersTradeList;

	//кнопки
	CUI3tButton			UIPerformTradeButton;
	CUI3tButton			UIToTalkButton;

	//информация о персонажах 
	CUIStatic			UIOurIcon;
	CUIStatic			UIOthersIcon;
	CUICharacterInfo	UICharacterInfoLeft;
	CUICharacterInfo	UICharacterInfoRight;

	//информация о перетаскиваемом предмете
	CUIStatic			UIDescWnd;
	CUIItemInfo			UIItemInfo;

	SDrawStaticStruct*	UIDealMsg;
};

CUITradeWnd::CUITradeWnd()
	:	m_bDealControlsVisible	(false),
		m_pTrade(NULL),
		m_pOthersTrade(NULL),
		bStarted(false)
{
	m_uidata = xr_new <CUITradeInternal>();
	Init();
	SetCurrentItem(NULL);

	currentFilterOur_.set(icAll, true);
	currentFilterTheir_.set(icAll, true);
}

CUITradeWnd::~CUITradeWnd()
{
	m_uidata->UIOurBagList.ClearAll(true);
	m_uidata->UIOurTradeList.ClearAll(true);
	m_uidata->UIOthersBagList.ClearAll(true);
	m_uidata->UIOthersTradeList.ClearAll(true);
	xr_delete(m_uidata);
}

void CUITradeWnd::Init()
{
	CUIXml uiXml;

	if (!ui_hud_type)
		ui_hud_type = 1;

	string128 TRADE_XML;
	xr_sprintf(TRADE_XML, "trade_%d.xml", ui_hud_type);

	string128 TRADE_CHARACTER_XML;
	xr_sprintf(TRADE_CHARACTER_XML, "trade_character_%d.xml", ui_hud_type);

	string128 TRADE_ITEM_XML;
	xr_sprintf(TRADE_ITEM_XML, "trade_item_%d.xml", ui_hud_type);

	bool xml_result	= uiXml.Load(CONFIG_PATH, UI_PATH, TRADE_XML);
	R_ASSERT3(xml_result, "xml file not found", TRADE_XML);
	CUIXmlInit xml_init;

	xml_init.InitWindow					(uiXml, "main", 0, this);

	//статические элементы интерфейса
	AttachChild							(&m_uidata->UIStaticTop);
	xml_init.InitStatic					(uiXml, "top_background", 0, &m_uidata->UIStaticTop);
	AttachChild							(&m_uidata->UIStaticBottom);
	xml_init.InitStatic					(uiXml, "bottom_background", 0, &m_uidata->UIStaticBottom);

	//иконки с изображение нас и партнера по торговле
	AttachChild							(&m_uidata->UIOurIcon);
	xml_init.InitStatic					(uiXml, "static_icon", 0, &m_uidata->UIOurIcon);
	AttachChild							(&m_uidata->UIOthersIcon);
	xml_init.InitStatic					(uiXml, "static_icon", 1, &m_uidata->UIOthersIcon);
	m_uidata->UIOurIcon.AttachChild		(&m_uidata->UICharacterInfoLeft);
	m_uidata->UICharacterInfoLeft.Init	(0,0, m_uidata->UIOurIcon.GetWidth(), m_uidata->UIOurIcon.GetHeight(), TRADE_CHARACTER_XML);
	m_uidata->UIOthersIcon.AttachChild	(&m_uidata->UICharacterInfoRight);
	m_uidata->UICharacterInfoRight.Init	(0,0, m_uidata->UIOthersIcon.GetWidth(), m_uidata->UIOthersIcon.GetHeight(), TRADE_CHARACTER_XML);


	//Списки торговли
	AttachChild							(&m_uidata->UIOurBagWnd);
	xml_init.InitStatic					(uiXml, "our_bag_static", 0, &m_uidata->UIOurBagWnd);
	AttachChild							(&m_uidata->UIOthersBagWnd);
	xml_init.InitStatic					(uiXml, "others_bag_static", 0, &m_uidata->UIOthersBagWnd);

	m_uidata->UIOurBagWnd.AttachChild	(&m_uidata->UIOurMoneyStatic);
	xml_init.InitStatic					(uiXml, "our_money_static", 0, &m_uidata->UIOurMoneyStatic);

	m_uidata->UIOthersBagWnd.AttachChild(&m_uidata->UIOtherMoneyStatic);
	xml_init.InitStatic					(uiXml, "other_money_static", 0, &m_uidata->UIOtherMoneyStatic);

	AttachChild							(&m_uidata->UIOurTradeWnd);
	xml_init.InitStatic					(uiXml, "static", 0, &m_uidata->UIOurTradeWnd);
	AttachChild							(&m_uidata->UIOthersTradeWnd);
	xml_init.InitStatic					(uiXml, "static", 1, &m_uidata->UIOthersTradeWnd);

	m_uidata->UIOurTradeWnd.AttachChild	(&m_uidata->UIOurPriceCaption);
	xml_init.InitMultiTextStatic		(uiXml, "price_mt_static", 0, &m_uidata->UIOurPriceCaption);
	
	m_uidata->UIOthersTradeWnd.AttachChild(&m_uidata->UIOthersPriceCaption);
	xml_init.InitMultiTextStatic		(uiXml, "price_mt_static", 1, &m_uidata->UIOthersPriceCaption);
	

	//Списки Drag&Drop
	m_uidata->UIOurBagWnd.AttachChild	(&m_uidata->UIOurBagList);	
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_list", 0, &m_uidata->UIOurBagList);

	m_uidata->UIOthersBagWnd.AttachChild(&m_uidata->UIOthersBagList);	
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_list", 1, &m_uidata->UIOthersBagList);

	m_uidata->UIOurTradeWnd.AttachChild	(&m_uidata->UIOurTradeList);	
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_list", 2, &m_uidata->UIOurTradeList);

	m_uidata->UIOthersTradeWnd.AttachChild(&m_uidata->UIOthersTradeList);	
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_list", 3, &m_uidata->UIOthersTradeList);

	
	AttachChild							(&m_uidata->UIDescWnd);
	xml_init.InitStatic					(uiXml, "desc_static", 0, &m_uidata->UIDescWnd);
	m_uidata->UIDescWnd.AttachChild		(&m_uidata->UIItemInfo);
	m_uidata->UIItemInfo.Init			(TRADE_ITEM_XML); //(0,0, m_uidata->UIDescWnd.GetWidth(), m_uidata->UIDescWnd.GetHeight(), TRADE_ITEM_XML);
	m_uidata->UIItemInfo.SetWndRect		(Frect().set(0,0, m_uidata->UIDescWnd.GetWidth(), m_uidata->UIDescWnd.GetHeight()));

	xml_init.InitAutoStatic				(uiXml, "auto_static", this);


	AttachChild							(&m_uidata->UIPerformTradeButton);
	xml_init.Init3tButton					(uiXml, "button", 0, &m_uidata->UIPerformTradeButton);

	AttachChild							(&m_uidata->UIToTalkButton);
	xml_init.Init3tButton					(uiXml, "button", 1, &m_uidata->UIToTalkButton);

	// Init item category filter tabs
	AttachChild(&UIFilterTabsOurs);
	UIFilterTabsOurs.InitFromXml(uiXml, "our_category_tabs");

	AttachChild(&UIFilterTabsTheir);
	UIFilterTabsTheir.InitFromXml(uiXml, "their_category_tabs");

	m_uidata->UIDealMsg					= NULL;

	BindDragDropListEvents				(&m_uidata->UIOurBagList);
	BindDragDropListEvents				(&m_uidata->UIOthersBagList);
	BindDragDropListEvents				(&m_uidata->UIOurTradeList);
	BindDragDropListEvents				(&m_uidata->UIOthersTradeList);
}

void CUITradeWnd::BindDragDropListEvents(CUIDragDropListEx* lst)
{
	lst->m_f_item_drop				= CUIDragDropListEx::DRAG_CELL_EVENT(this,&CUITradeWnd::OnItemDragDrop);
	lst->m_f_item_start_drag		= CUIDragDropListEx::DRAG_CELL_EVENT(this,&CUITradeWnd::OnItemStartDrag);
	lst->m_f_item_db_click			= CUIDragDropListEx::DRAG_CELL_EVENT(this,&CUITradeWnd::OnItemDbClick);
	lst->m_f_item_selected			= CUIDragDropListEx::DRAG_CELL_EVENT(this,&CUITradeWnd::OnItemSelected);
	lst->m_f_item_rbutton_click		= CUIDragDropListEx::DRAG_CELL_EVENT(this,&CUITradeWnd::OnItemRButtonClick);
}

void CUITradeWnd::InitTrade(CInventoryOwner* pOur, CInventoryOwner* pOthers)
{
	VERIFY(pOur);
	VERIFY(pOthers);

	m_pInvOwner	= pOur;
	m_pOthersInvOwner = pOthers;

	m_uidata->UICharacterInfoLeft.InitCharacter(m_pInvOwner->object_id());
	m_uidata->UICharacterInfoRight.InitCharacter(m_pOthersInvOwner->object_id());
	m_uidata->UIOthersPriceCaption.GetPhraseByIndex(0)->str = *CStringTable().translate("ui_st_buy_items");

	m_uidata->UIOurIcon.Show(GameConstants::GetUseCharInfoInWnds());
	m_uidata->UIOthersIcon.Show(GameConstants::GetUseCharInfoInWnds());

	Our_Physical_Inv = &m_pInvOwner->inventory();
	Others_Physical_Inv = pOur->GetTrade()->GetPartnerInventory();
		
	m_pTrade = pOur->GetTrade();
	m_pOthersTrade = pOur->GetTrade()->GetPartnerTrade();
    	
	EnableAll();

	UpdateLists(eBoth);
}  

void CUITradeWnd::SendMessage(CUIWindow *pWnd, s16 msg, void *pData)
{
	if(pWnd == &m_uidata->UIToTalkButton && msg == BUTTON_CLICKED)
	{
		SwitchToTalk();
	}
	else if(msg == BUTTON_CLICKED)
	{
		if (pWnd == &m_uidata->UIPerformTradeButton)
			PerformTrade();
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

	CUIWindow::SendMessage(pWnd, msg, pData);
}

void CUITradeWnd::Draw()
{
	inherited::Draw();
	if(m_uidata->UIDealMsg)
		m_uidata->UIDealMsg->Draw();
}

void CUITradeWnd::Show()
{
	SendInfoToActor("ui_trade");

	inherited::Show(true);
	inherited::Enable(true);

	SetCurrentItem(NULL);
	ResetAll();
	m_uidata->UIDealMsg = NULL;
}

void CUITradeWnd::Hide()
{
	SendInfoToActor("ui_trade_hide");

	inherited::Show(false);
	inherited::Enable(false);
	if (bStarted)
		StopTrade();

	m_uidata->UIDealMsg = NULL;

	if (CurrentGameUI())
	{
		CurrentGameUI()->RemoveCustomStatic("trade_deal_msg");
	}

	m_uidata->UIOurBagList.ClearAll(true);
	m_uidata->UIOurTradeList.ClearAll(true);
	m_uidata->UIOthersBagList.ClearAll(true);
	m_uidata->UIOthersTradeList.ClearAll(true);
}

void CUITradeWnd::StartTrade()
{
	if (m_pTrade)
		m_pTrade->TradeCB(true);

	if (m_pOthersTrade)
		m_pOthersTrade->TradeCB(true);

	bStarted = true;
}

void CUITradeWnd::StopTrade()
{
	if (m_pTrade)
		m_pTrade->TradeCB(false);

	if (m_pOthersTrade)
		m_pOthersTrade->TradeCB(false);

	bStarted = false;
}

void CUITradeWnd::DisableAll()
{
	m_uidata->UIOurBagWnd.Enable(false);
	m_uidata->UIOthersBagWnd.Enable(false);
	m_uidata->UIOurTradeWnd.Enable(false);
	m_uidata->UIOthersTradeWnd.Enable(false);
}

void CUITradeWnd::EnableAll()
{
	m_uidata->UIOurBagWnd.Enable(true);
	m_uidata->UIOthersBagWnd.Enable(true);
	m_uidata->UIOurTradeWnd.Enable(true);
	m_uidata->UIOthersTradeWnd.Enable(true);
}

void CUITradeWnd::SwitchToTalk()
{
	GetMessageTarget()->SendMessage(this, TRADE_WND_CLOSED);
}


extern void UpdateCameraDirection(CGameObject* pTo);

void CUITradeWnd::Update()
{
	EListType et = eNone;

	if (Our_Physical_Inv->ModifyFrame() == CurrentFrame() && Others_Physical_Inv->ModifyFrame() == CurrentFrame()){
		et = eBoth;
	}
	else if (Our_Physical_Inv->ModifyFrame() == CurrentFrame()){
		et = e1st;
	}
	else if (Others_Physical_Inv->ModifyFrame() == CurrentFrame()){
		et = e2nd;
	}
	if(et!=eNone)
		UpdateLists(et);

	inherited::Update();
	UpdateCameraDirection(smart_cast<CGameObject*>(m_pOthersInvOwner));

	if(m_uidata->UIDealMsg)
	{
		m_uidata->UIDealMsg->Update();

		if(!m_uidata->UIDealMsg->IsActual())
		{
			CurrentGameUI()->RemoveCustomStatic("trade_deal_msg");
			m_uidata->UIDealMsg			= NULL;
		}
	}

	//обновление веса
	UpdateWeight(m_uidata->UIOurBagWnd, m_pInvOwner, true);
}


void CUITradeWnd::UpdateLists(EListType mode)
{
	if (mode == eBoth || mode == e1st){
		m_uidata->UIOurBagList.ClearAll(true);
		m_uidata->UIOurTradeList.ClearAll(true);
	}

	if (mode == eBoth || mode == e2nd){
		m_uidata->UIOthersBagList.ClearAll(true);
		m_uidata->UIOthersTradeList.ClearAll(true);
	}

	UpdatePrices();

	if (mode == eBoth || mode == e1st){
		ruck_list.clear();
		Our_Physical_Inv->AddAvailableItems(ruck_list, true);

		UpdateBagList(ruck_list, m_uidata->UIOurBagList, currentFilterOur_, true, true);
	}

	if (mode == eBoth || mode == e2nd){
		ruck_list.clear();
		Others_Physical_Inv->AddAvailableItems(ruck_list, true);

		UpdateBagList(ruck_list, m_uidata->UIOthersBagList, currentFilterTheir_, true, false);
	}

	UIFilterTabsOurs.SetHighlitedTab(currentFilterOur_.get());
	UIFilterTabsTheir.SetHighlitedTab(currentFilterTheir_.get());
}

void CUITradeWnd::FillList(TIItemContainer& cont, CUIDragDropListEx& dragDropList, bool do_colorize)
{
	TIItemContainer::iterator it = cont.begin();
	TIItemContainer::iterator it_e = cont.end();

	for (; it != it_e; ++it)
	{
		CUICellItem* itm = create_cell_item(*it);
		if (do_colorize)
		{
			ColorizeItem(itm, CanMoveToOther(*it));
		}
		dragDropList.SetItem(itm);
	}
}

void CUITradeWnd::UpdatePrices()
{
	m_iOurTradePrice = CalcItemsPrice(&m_uidata->UIOurTradeList, m_pOthersTrade, true);
	m_iOthersTradePrice = CalcItemsPrice(&m_uidata->UIOthersTradeList, m_pOthersTrade, false);

	string256 buf;

	LPCSTR currency_str = *CStringTable().translate("ui_st_currency");

	xr_sprintf(buf, "%d %s", m_iOurTradePrice, currency_str);
	m_uidata->UIOurPriceCaption.GetPhraseByIndex(2)->str = buf;

	xr_sprintf(buf, "%d %s", m_iOthersTradePrice, currency_str);
	m_uidata->UIOthersPriceCaption.GetPhraseByIndex(2)->str = buf;

	xr_sprintf(buf, "%d %s", m_pInvOwner->get_money(), currency_str);
	m_uidata->UIOurMoneyStatic.SetText(buf);

	if (!m_pOthersInvOwner->InfinitiveMoney())
	{
		xr_sprintf(buf, "%d %s", m_pOthersInvOwner->get_money(), currency_str);
		m_uidata->UIOtherMoneyStatic.SetText(buf);
	}
	else
	{
		m_uidata->UIOtherMoneyStatic.SetText("---");
	}
}

void CUITradeWnd::UpdateBagList(TIItemContainer& item_list, CUIDragDropListEx& dd, Flags32& filter, bool sort, bool colorize)
{
	dd.ClearAll(true);

	item_list.erase(std::remove_if(item_list.begin(), item_list.end(), SLeaveCategory(filter)), item_list.end());

	if (sort)
		std::sort(item_list.begin(), item_list.end(), GreaterRoomInRuck);

	FillList(item_list, dd, colorize);
}

void CUITradeWnd::PerformTrade()
{
	if (m_uidata->UIOurTradeList.ItemsCount() == 0 && m_uidata->UIOthersTradeList.ItemsCount() == 0)
		return;

	int our_money = (int)m_pInvOwner->get_money();
	int others_money = (int)m_pOthersInvOwner->get_money();

	int delta_price = int(m_iOurTradePrice - m_iOthersTradePrice);

	our_money += delta_price;
	others_money -= delta_price;

	if (our_money >= 0 && others_money >= 0 && (m_iOurTradePrice >= 0 || m_iOthersTradePrice>0))
	{
		m_pOthersTrade->OnPerformTrade(m_iOthersTradePrice, m_iOurTradePrice);

		TransferItems(&m_uidata->UIOurTradeList, &m_uidata->UIOthersBagList, m_pOthersTrade, true);
		TransferItems(&m_uidata->UIOthersTradeList, &m_uidata->UIOurBagList, m_pOthersTrade, false);
	}
	else
	{
		if (others_money<0)
			ShowCantDealMsg("st_not_enough_money_npc");
		else
			ShowCantDealMsg("st_not_enough_money");
	}
	SetCurrentItem(NULL);
}

void move_item(CUICellItem* itm, CUIDragDropListEx* from, CUIDragDropListEx* to)
{
	CUICellItem* _itm = from->RemoveItem(itm, false);
	to->SetItem(_itm);
}

bool CUITradeWnd::ToOurTrade()
{
	auto tradeStatus = CheckCanSellToOther(CurrentIItem());
	if (tradeStatus != eAllow)
	{
		ShowCantDealMsg(GetItemHint(tradeStatus), 3.f);

		return false;
	}

	move_item(CurrentItem(), &m_uidata->UIOurBagList, &m_uidata->UIOurTradeList);
	UpdatePrices();

	return true;
}

bool CUITradeWnd::ToOthersTrade()
{
	move_item(CurrentItem(), &m_uidata->UIOthersBagList, &m_uidata->UIOthersTradeList);
	UpdatePrices();

	return true;
}

bool CUITradeWnd::ToOurBag()
{
	move_item(CurrentItem(), &m_uidata->UIOurTradeList, &m_uidata->UIOurBagList);
	UpdatePrices();
	
	return true;
}

bool CUITradeWnd::ToOthersBag()
{
	move_item(CurrentItem(), &m_uidata->UIOthersTradeList, &m_uidata->UIOthersBagList);
	UpdatePrices();

	return true;
}

bool CUITradeWnd::OnItemStartDrag(CUICellItem* itm)
{
	return false; //default behaviour
}

bool CUITradeWnd::OnItemSelected(CUICellItem* itm)
{
	if (m_pCurrentCellItem)
		m_pCurrentCellItem->Mark(false);

	if (!itm) return false;

	SetCurrentItem(itm);

	MarkApplicableItems(itm);
	itm->Mark(true);

	return false;
}

bool CUITradeWnd::OnItemRButtonClick(CUICellItem* itm)
{
	SetCurrentItem(itm);
	return false;
}

bool CUITradeWnd::OnItemDragDrop(CUICellItem* itm)
{
	CUIDragDropListEx* old_ddlist = itm->CurrentDDList();
	CUIDragDropListEx* new_ddlist = CUIDragDropListEx::m_drag_item->BackList();
	if (old_ddlist == new_ddlist || !old_ddlist || !new_ddlist)
		return false;

	if (old_ddlist == &m_uidata->UIOurBagList && new_ddlist == &m_uidata->UIOurTradeList)
		ToOurTrade();

	else if (old_ddlist == &m_uidata->UIOurTradeList && new_ddlist == &m_uidata->UIOurBagList)
		ToOurBag();

	else if (old_ddlist == &m_uidata->UIOthersBagList && new_ddlist == &m_uidata->UIOthersTradeList)
		ToOthersTrade();

	else if (old_ddlist == &m_uidata->UIOthersTradeList && new_ddlist == &m_uidata->UIOthersBagList)
		ToOthersBag();

	return true;
}

bool CUITradeWnd::OnItemDbClick(CUICellItem* itm)
{
	SetCurrentItem(itm);
	CUIDragDropListEx* old_ddlist = itm->CurrentDDList();

	if (old_ddlist == &m_uidata->UIOurBagList)
		ToOurTrade();

	else if (old_ddlist == &m_uidata->UIOurTradeList)
		ToOurBag();

	else if (old_ddlist == &m_uidata->UIOthersBagList)
		ToOthersTrade();

	else if (old_ddlist == &m_uidata->UIOthersTradeList)
		ToOthersBag();

	else
		R_ASSERT2(false, "wrong parent for cell item");

	return true;
}

#include "../trade_parameters.h"
CUITradeWnd::ETradeItemStatus CUITradeWnd::CheckCanSellToOther(PIItem pItem)
{
	if (!m_pOthersInvOwner->trade_parameters().enabled(CTradeParameters::action_buy(0), pItem->object().SectionName()))
		return ETradeItemStatus::eDeny;

	if (pItem->GetCondition() < m_pOthersInvOwner->trade_parameters().buy_item_condition_factor && !pItem->TradeIgnoreCondition())
		return ETradeItemStatus::eCondition;

	float our_trade_list_weight = CalcItemsWeight(&m_uidata->UIOurTradeList);
	float others_trade_list_weight = CalcItemsWeight(&m_uidata->UIOthersTradeList);

	float itmWeight = pItem->Weight();
	float otherInvWeight = Others_Physical_Inv->CalcTotalWeight();
	float otherMaxWeight = Others_Physical_Inv->GetMaxWeight();

	if (otherInvWeight - others_trade_list_weight + our_trade_list_weight + itmWeight > otherMaxWeight)
		return ETradeItemStatus::eWeight;

	return ETradeItemStatus::eAllow;
}

LPCSTR CUITradeWnd::GetItemHint(ETradeItemStatus tradeStatus)
{
	switch (tradeStatus)
	{
	case ETradeItemStatus::eDeny:
		return "st_no_trade_tip_1";
	case ETradeItemStatus::eCondition:
		return "st_no_trade_tip_2";
	case ETradeItemStatus::eWeight:
		return "st_no_trade_tip_3";
	default:
		return "";
	}
}

bool CUITradeWnd::CanMoveToOther(PIItem pItem)
{
	return CheckCanSellToOther(pItem) == ETradeItemStatus::eAllow;
}

float CUITradeWnd::CalcItemsWeight(CUIDragDropListEx* pList)
{
	float res = 0.0f;

	for(u32 i=0; i<pList->ItemsCount(); ++i)
	{
		CUICellItem* itm	= pList->GetItemIdx	(i);
		PIItem	iitem		= (PIItem)itm->m_pData;
		res					+= iitem->Weight();
		for(u32 j=0; j<itm->ChildsCount(); ++j){
			PIItem	jitem		= (PIItem)itm->Child(j)->m_pData;
			res					+= jitem->Weight();
		}
	}
	return res;
}

u32 CUITradeWnd::CalcItemsPrice(CUIDragDropListEx* pList, CTrade* pTrade, bool bBuying)
{
	u32 iPrice = 0;

	for (u32 i = 0; i<pList->ItemsCount(); ++i)
	{
		CUICellItem* itm = pList->GetItemIdx(i);
		PIItem iitem = (PIItem)itm->m_pData;

		iPrice += pTrade->GetItemPrice(iitem, bBuying);

		for (u32 j = 0; j<itm->ChildsCount(); ++j){
			PIItem jitem = (PIItem)itm->Child(j)->m_pData;

			iPrice += pTrade->GetItemPrice(jitem, bBuying);
		}

	}

	return iPrice;
}

void CUITradeWnd::ShowCantDealMsg(LPCSTR text, float time)
{
	auto msg = m_uidata->UIDealMsg = CurrentGameUI()->AddCustomStatic("trade_deal_msg", true);
	msg->SetText(text);	
	msg->m_endTime = EngineTime() + time;// sec

#pragma todo("Implement proper UI for messages or use hints")
	// A hack so we can actually see the message..
	SetCurrentItem(NULL);
}

void CUITradeWnd::TransferItems(CUIDragDropListEx* pSellList,
								CUIDragDropListEx* pBuyList,
								CTrade* pTrade,
								bool bBuying)
{
	while(pSellList->ItemsCount())
	{
		CUICellItem* itm = pSellList->RemoveItem(pSellList->GetItemIdx(0),false);
		pTrade->TransferItem((PIItem)itm->m_pData, bBuying);
		pBuyList->SetItem(itm);
	}

	// send info about money to SO
	m_pInvOwner->set_money(m_pInvOwner->get_money(), true);
	m_pOthersInvOwner->set_money(m_pOthersInvOwner->get_money(), true);
}


void CUITradeWnd::MarkApplicableItems(CUICellItem* itm)
{
	CInventoryItem* inventoryitem = (CInventoryItem*)itm->m_pData;
	if (!inventoryitem) return;

	u32 color = pSettings->r_color("inventory_color_ammo", "color");

	CWeaponMagazined* weapon = smart_cast<CWeaponMagazined*>(inventoryitem);

	xr_vector<shared_str> addons;

	if (weapon)
		addons = GetAddonsForWeapon(weapon);

	MarkItems(weapon, addons, m_uidata->UIOurBagList, color, true);
	MarkItems(weapon, addons, m_uidata->UIOthersBagList, color, false);
	MarkItems(weapon, addons, m_uidata->UIOurTradeList, color, true);
	MarkItems(weapon, addons, m_uidata->UIOthersTradeList, color, false);
}

void CUITradeWnd::MarkItems(CWeaponMagazined* weapon, const xr_vector<shared_str>& addons, CUIDragDropListEx& list, u32 ammoColor, bool colorize)
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

			switch (type) {
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
			ColorizeItem(cell_item, CanMoveToOther(invitem));
		}
	}
}

void CUITradeWnd::ColorizeItem(CUICellItem* itm, bool b)
{
	PIItem iitem = (PIItem)itm->m_pData;
	if (!b)
		itm->SetTextureColor(color_rgba(255, 100, 100, 255));

	else if (iitem->m_eItemPlace == eItemPlaceSlot || iitem->m_eItemPlace == eItemPlaceBelt)
		itm->SetTextureColor(color_rgba(100, 255, 100, 255));
}

CUICellItem* CUITradeWnd::CurrentItem()
{
	return m_pCurrentCellItem;
}

PIItem CUITradeWnd::CurrentIItem()
{
	return(m_pCurrentCellItem)?(PIItem)m_pCurrentCellItem->m_pData : NULL;
}

void CUITradeWnd::SetCurrentItem(CUICellItem* itm)
{
	if(m_pCurrentCellItem == itm) return;

	m_pCurrentCellItem = itm;
	m_uidata->UIItemInfo.InitItem(CurrentIItem());
	
	if(!m_pCurrentCellItem) return;

	if (m_uidata->UIDealMsg && m_uidata->UIDealMsg->IsActual())
	{
		// Hide message, so it's not obstructing the item description
		m_uidata->UIDealMsg->m_endTime = EngineTime();
	}

	CUIDragDropListEx* owner = itm->CurrentDDList();
	bool bBuying = (owner==&m_uidata->UIOurBagList) || (owner==&m_uidata->UIOurTradeList);

	if (itm && m_uidata->UIItemInfo.UICost)
	{
		string256			str;

		if (!bBuying || CanMoveToOther(CurrentIItem()))
		{
			xr_sprintf(str, "%d %s", m_pOthersTrade->GetItemPrice(CurrentIItem(), bBuying), *CStringTable().translate("ui_st_currency"));
		}
		else 
		{
			xr_strcpy(str, "---");
		}

		m_uidata->UIItemInfo.UICost->SetText(str);
	}
}

void CUITradeWnd::SetCategoryFilter(u32 mask, bool our)
{
	Flags32& filter = our ? currentFilterOur_ : currentFilterTheir_;

	if (our)
	{
		ruck_list.clear();
		Our_Physical_Inv->AddAvailableItems(ruck_list, true);
	}
	else
	{
		ruck_list.clear();
		Others_Physical_Inv->AddAvailableItems(ruck_list, true);
	}

	CUIDragDropListEx& dd = our ? m_uidata->UIOurBagList : m_uidata->UIOthersBagList;

	filter.zero();
	filter.set(mask, true);

	if (IsShown())
		UpdateBagList(ruck_list, dd, filter, true, our);

	if (our)
		UIFilterTabsOurs.SetHighlitedTab(mask);
	else
		UIFilterTabsTheir.SetHighlitedTab(mask);
}