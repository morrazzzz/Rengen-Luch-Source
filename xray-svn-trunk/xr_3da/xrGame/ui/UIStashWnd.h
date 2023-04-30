#pragma once

#include "UIDialogWnd.h"
#include "UIEditBox.h"
#include "../inventory_space.h"
#include "../WeaponMagazined.h"
#include "UIInventoryCategoryTabs.h"

class CUIDragDropListEx;
class CUIItemInfo;
class CUITextWnd;
class CUICharacterInfo;
class CUIPropertiesBox;
class CUI3tButton;
class CUICellItem;
class CInventoryBox;
class CInventoryOwner;
class CCar;

class CUIStashWnd: public CUIDialogWnd
{
private:
	typedef CUIDialogWnd	inherited;
	bool					m_b_need_update;

	void					ColorizeItem				(CUICellItem* itm);

	void					MarkApplicableItems			(CUICellItem* itm);
	void					MarkItems					(CWeaponMagazined*, const xr_vector<shared_str>& addons, CUIDragDropListEx&, u32 color, bool colorize = false);

	TIItemContainer			ruck_list;
public:
	CUIStashWnd();
	virtual					~CUIStashWnd();

	virtual void			Init						();
	virtual bool			StopAnyMove					(){return true;}

	virtual void			SendMessage					(CUIWindow *pWnd, s16 msg, void *pData);

	void					InitCustomInventory			(CInventoryOwner* pOurInv, CInventoryOwner* pOthersInv);
	void					InitInventoryBox			(CInventoryOwner* pOur, CInventoryBox* pInvBox);

	virtual void			Draw()						{inherited::Draw();};
	virtual void			Update						();

	virtual void			ShowDialog					(bool bDoHideIndicators);
	virtual void			HideDialog					();

	void					DisableAll					();
	void					EnableAll					();
	virtual bool			OnKeyboardAction			(int dik, EUIMessages keyboard_action);

	void					UpdateLists_delayed			();

	Flags32					currentFilterOur_;
	Flags32					currentFilterTheir_;

	void					SetCategoryFilter			(u32 mask, bool our);
protected:
	CInventoryOwner*		m_pOurOwner;
	CInventoryOwner*		m_pOthersOwner;

	CInventoryBox*			m_pInventoryBox;
	CInventory*				m_pInventory;

	CCar*					m_pCar;

	CUIDragDropListEx*		m_pUIOurBagList;
	CUIDragDropListEx*		m_pUIOthersBagList;

	CUIStatic*				m_pUIStaticTop;
	CUIStatic*				m_pUIStaticBottom;

	CUIFrameWindow*			m_pUIDescWnd;
	CUIStatic*				m_pUIStaticDesc;
	CUIItemInfo*			m_pUIItemInfo;

	CUIStatic*				m_pUIOurBagWnd;
	CUIStatic*				m_pUIOthersBagWnd;
	LPCSTR					m_othersBagWndPrefix;

	//информация о персонажах 
	CUIStatic*				m_pUIOurIcon;
	CUIStatic*				m_pUIOthersIcon;
	CUICharacterInfo*		m_pUICharacterInfoLeft;
	CUICharacterInfo*		m_pUICharacterInfoRight;
	CUIPropertiesBox*		m_pUIPropertiesBox;
	CUI3tButton*			m_pUITakeAll;

	CUICellItem*			m_pCurrentCellItem;

	// Tab buttons holder ours
	CUIInvCategoryTabs		UIFilterTabsOurs;
	// Tab buttons holder their
	CUIInvCategoryTabs		UIFilterTabsTheir;

	void					UpdateLists					();

	void					ActivatePropertiesBox		();
	void					EatItem						();

	bool					ToOurBag					();
	bool					ToOthersBag					();
	
	void					SetCurrentItem				(CUICellItem* itm);
	CUICellItem*			CurrentItem					();
	PIItem					CurrentIItem				();

	// Взять все
	void					TakeAll						();

	void					UpdateBagList				(TIItemContainer& item_list, CUIDragDropListEx& dd, Flags32& filter, bool sort = true, bool colorize = true);

	bool		xr_stdcall	OnItemDragDrop				(CUICellItem* itm);
	bool		xr_stdcall	OnItemStartDrag				(CUICellItem* itm);
	bool		xr_stdcall	OnItemDbClick				(CUICellItem* itm);
	bool		xr_stdcall	OnItemSelected				(CUICellItem* itm);
	bool		xr_stdcall	OnItemRButtonClick			(CUICellItem* itm);

	bool					TransferItem				(PIItem itm, CInventoryOwner* owner_from, CInventoryOwner* owner_to, bool b_check);
	void					BindDragDropListEvents		(CUIDragDropListEx* lst);
};