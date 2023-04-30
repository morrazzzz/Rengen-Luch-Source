#pragma once
#include "UIWindow.h"
#include "../inventory_space.h"
#include "../ui_base.h"
#include "../WeaponMagazined.h"
#include "UIInventoryCategoryTabs.h"

class CInventoryOwner;
class CEatableItem;
class CTrade;
struct CUITradeInternal;

class CUIDragDropListEx;
class CUICellItem;

class CUITradeWnd: public CUIWindow
{
private:
	typedef CUIWindow inherited;

	void				ColorizeItem				(CUICellItem* itm, bool b);

	void				MarkApplicableItems			(CUICellItem* itm);
	void				MarkItems					(CWeaponMagazined* weapon, const xr_vector<shared_str>& addons, CUIDragDropListEx&, u32 color, bool colorize = false);
public:
						CUITradeWnd					();
	virtual				~CUITradeWnd				();

	virtual void		Init						();

	virtual void		SendMessage					(CUIWindow *pWnd, s16 msg, void *pData);

	void				InitTrade					(CInventoryOwner* pOur, CInventoryOwner* pOthers);
	
	virtual void 		Draw						();
	virtual void 		Update						();
	virtual void 		Show						();
	virtual void 		Hide						();

	void 				DisableAll					();
	void 				EnableAll					();

	void 				SwitchToTalk				();
	void 				StartTrade					();
	void 				StopTrade					();

	Flags32				currentFilterOur_;
	Flags32				currentFilterTheir_;

	void				SetCategoryFilter			(u32 mask, bool our);
protected:

	CUITradeInternal*	m_uidata;

	// Tab buttons holder ours
	CUIInvCategoryTabs	UIFilterTabsOurs;
	// Tab buttons holder their
	CUIInvCategoryTabs	UIFilterTabsTheir;

	bool				bStarted;
	bool 				ToOurTrade					();
	bool 				ToOthersTrade				();
	bool 				ToOurBag					();
	bool 				ToOthersBag					();
	void 				SendEvent_ItemDrop			(PIItem pItem);
	
	u32					CalcItemsPrice				(CUIDragDropListEx* pList, CTrade* pTrade, bool bBuying);
	float				CalcItemsWeight				(CUIDragDropListEx* pList);

	void				TransferItems				(CUIDragDropListEx* pSellList, CUIDragDropListEx* pBuyList, CTrade* pTrade, bool bBuying);

	void				PerformTrade				();
	void				UpdatePrices				();

	void				UpdateBagList				(TIItemContainer& item_list, CUIDragDropListEx& dd, Flags32& filter, bool sort = true, bool colorize = true);

	enum EListType{eNone,e1st,e2nd,eBoth};
	void				UpdateLists					(EListType);

	void				FillList					(TIItemContainer& cont, CUIDragDropListEx& list, bool do_colorize);

	bool				m_bDealControlsVisible;

	enum ETradeItemStatus{eAllow,eDeny,eWeight,eCondition};
	ETradeItemStatus	CheckCanSellToOther			(PIItem pItem);
	bool				CanMoveToOther				(PIItem pItem);
	LPCSTR				GetItemHint					(ETradeItemStatus);

	void				ShowCantDealMsg				(LPCSTR text, float time = 2.0f);

	//указатели игрока и того с кем торгуем
	CInventory*			Our_Physical_Inv;
	CInventory*			Others_Physical_Inv;
	CInventoryOwner*	m_pInvOwner;
	CInventoryOwner*	m_pOthersInvOwner;
	CTrade*				m_pTrade;
	CTrade*				m_pOthersTrade;

	u32					m_iOurTradePrice;
	u32					m_iOthersTradePrice;


	CUICellItem*		m_pCurrentCellItem;
	TIItemContainer		ruck_list;

	void				SetCurrentItem				(CUICellItem* itm);
	CUICellItem*		CurrentItem					();
	PIItem				CurrentIItem				();

	bool		xr_stdcall		OnItemDragDrop			(CUICellItem* itm);
	bool		xr_stdcall		OnItemStartDrag		(CUICellItem* itm);
	bool		xr_stdcall		OnItemDbClick		(CUICellItem* itm);
	bool		xr_stdcall		OnItemSelected		(CUICellItem* itm);
	bool		xr_stdcall		OnItemRButtonClick	(CUICellItem* itm);

	void				BindDragDropListEvents		(CUIDragDropListEx* lst);

};