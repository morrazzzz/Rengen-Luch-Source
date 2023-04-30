#pragma once

class CInventory;

#include "UIDialogWnd.h"
#include "UIStatic.h"

#include "UIProgressBar.h"

#include "UIPropertiesBox.h"
#include "UIOutfitSlot.h"

#include "UIActorProtectionInfo.h"
#include "UIItemInfo.h"
#include "../inventory_space.h"
#include "../WeaponMagazined.h"
#include "UIInventoryCategoryTabs.h"

class CUI3tButton;
class CUIDragDropListEx;
class CUICellItem;

class CUIInventoryWnd: public CUIDialogWnd
{
private:
	typedef CUIDialogWnd	inherited;
	bool					m_b_need_reinit;
	xr_vector<ref_sound*>	use_sounds;
	
	string256				lua_is_reapirable_func; //check is in script
	string256				lua_on_repair_wpn_func; //function is in script

	void					MarkApplicableItems			(CUICellItem* itm);
	void					MarkItems					(CWeaponMagazined*, const xr_vector<shared_str>& addons, CUIDragDropListEx&, u32 color);

	bool					wasCrouchedBeforeOpen_;
	bool					wasSprintingBeforeOpen_;
	bool					needWepaonUnholster_;

	Flags32					currentFilter_;
public:
							CUIInventoryWnd				();
	virtual					~CUIInventoryWnd			();

	virtual void			Init						();

	void					InitInventory				();
	void					ReinitInventoryWnd();
	virtual bool			StopAnyMove()				{return false;}

	virtual void			SendMessage					(CUIWindow *pWnd, s16 msg, void *pData);
	virtual bool			OnMouseAction				(float x, float y, EUIMessages mouse_action);
	virtual bool			OnKeyboardAction			(int dik, EUIMessages keyboard_action);


	IC CInventory*			GetInventory()				{return Physical_Inventory;}

	virtual void			Update						();
	// Update fake blocking UI blockers
	void					UpdateSlotBlockers();
	// Update belt items position
	void					UpdateBeltUI();

	void					UpdateBagList				(bool sort = true);

	virtual void			Draw()						{inherited::Draw(); };

	virtual void			ShowDialog					(bool bDoHideIndicators);
	virtual void			HideDialog					();

	void					AddItemToBag				(PIItem pItem);
	void					DeleteFromInventory			(PIItem PIItem);


	void					PlayUseSound(LPCSTR sound_path);

	// Слоты быстрого использования
	CUIDragDropReferenceList*	quickUseSlots_;

	void					SetCategoryFilter			(u32 mask);
	u32						GetCategoryFilter			()			{ return currentFilter_.get(); };

	bool					DropAllItemsFromRuck		(bool quest_force);
protected:
	enum eInventorySndAction
	{
		eInvSndOpen	=0,
		eInvSndClose,
		eInvItemToSlot,
		eInvItemToBelt,
		eInvItemToRuck,
		eInvProperties,
		eInvDropItem,
		eInvAttachAddon,
		eInvDetachAddon,
		eInvItemUse,
		eInvSndMax
	};

	ref_sound					sounds					[eInvSndMax];
	void						PlaySnd					(eInventorySndAction a);

	CUIStatic					UIBeltSlots;
	CUIStatic					UIBack;

	CUIStatic					UIBagWnd;
	CUIStatic					UIMoneyWnd;
	CUIStatic					UIDescrWnd;
	CUIFrameWindow				UIPersonalWnd;

	// Tab buttons holder
	CUIInvCategoryTabs			UIFilterTabs;

	CUI3tButton*				UIExitButton;
	CUIStatic					UIStaticBottom;
	CUIStatic					UIStaticTime;
	CUITextWnd					UIStaticTimeString;

	CUIStatic					UIStaticPersonal;
		
	CUIDragDropListEx*			m_pUIBagList;
	CUIDragDropListEx*			m_pUIBeltList;
	CUIDragDropListEx*			m_pUIArtefactBeltList;
	CUIDragDropListEx*			m_pUIPistolList;
	CUIDragDropListEx*			m_pUIAutomaticList;
	CUIDragDropListEx*			m_pUIAutomatic2List;
	CUIDragDropListEx*			m_pUIKnifeList;
	CUIDragDropListEx*			m_pUIBinocularList;
	CUIDragDropListEx*			m_pUITorchList;
	CUIDragDropListEx*			m_pUIOutfitList;
	CUIDragDropListEx*			m_pUIDetectorList;
	CUIDragDropListEx*			m_pUIHelmetList;
	CUIDragDropListEx*			m_pUIPNVList;
	CUIDragDropListEx*			m_pUIAnomDetectorList;

	CUIDragDropListEx*			InitDragDropList			(CUIXml&, LPCSTR name, int index, CUIWindow* parent = nullptr, CUIDragDropListEx* list = nullptr);
	void						InitSlotItem				(CUIDragDropListEx* list, TSlotId slot);
	// Init all fake slot blockers
	void						InitSlotBlockers			(CUIXml* xml, CUIXmlInit* xml_init);
	// Create an array of fake slot blocking textures for dragdrop list with size greater than 1
	void						CreateSlotBlockersFor		(CUIDragDropListEx* dd_list, string256 blocker_node, xr_vector<CUIStatic*>& where_to_store, CUIXml* xml, CUIXmlInit* xml_init);
	// Create an array of fake slot blocking textures for dragdrop list with size 1
	void						CreateSlotBlockerFor		(CUIDragDropListEx* dd_list, string256 blocker_node, CUIStatic*& ui_static, CUIXml* xml, CUIXmlInit* xml_init);

	// Создать слоты для быстрого использования
	void						CreateQuickSlots			(CUIXml& xml, CUIXmlInit* xml_init);

	void						ClearAllLists				();
	void						BindDragDropListEvents		(CUIDragDropListEx* lst);

	EListType					GetType						(CUIDragDropListEx* l);
	TSlotId						GetSlot						(CUIDragDropListEx* l, CUICellItem* itm);
	CUIDragDropListEx*			GetSlotList					(TSlotId slot_idx);
	bool						SlotIsCompatible			(TSlotId desiredSlot, CUICellItem* itm);
	bool						SecondRifleSlotAvailable	() { return m_pUIAutomatic2List != nullptr; }
	bool						CanPutInSlot				(TSlotId desiredSlot, CUICellItem* itm);

	CUIStatic					UIProgressBack;

	xr_vector<CUIStatic*>		UI_artSlotBlockers;
	xr_vector<CUIStatic*>		UI_ammoSlotBlockers;
	CUIStatic*					UI_helmetSlotBlocker;
	CUIStatic*					UI_pnvSlotBlocker;

	xr_vector<CUIStatic*>		quickSlotsHotKeyIndicators_;

	CUIProgressBar				UIProgressBarHealth;
	CUIProgressBar				UIProgressBarBleeding;
	CUIProgressBar				UIProgressBarStamina;
	CUIProgressBar				UIProgressBarRadiation;
	CUIProgressBar				UIProgressBarHunger;
	CUIProgressBar				UIProgressBarThirst;
	CUIProgressBar				UIProgressBarArmor;
	CUIProgressBar				UIProgressBarMozg;

	CUIPropertiesBox			UIPropertiesBox;
	
	//информация о персонаже
	UIActorProtectionInfo		UIOutfitInfo;
	CUIItemInfo					UIItemInfo;

	CInventory*					Physical_Inventory;

	CUICellItem*				m_pCurrentCellItem;

	bool						DragDropItem				(PIItem itm, CUIDragDropListEx* ddlist);
	void						EatItem						(CEatableItem* ItemToEat);
	void						DropCurrentItem				(bool b_all);

	void						AttachAddon					(PIItem item_to_upgrade);
	void						DetachAddon					(const char* addon_name);

	void						ProcessPropertiesBoxClicked	();
	void						ActivatePropertiesBox		();
	bool						AttachActionToPropertyBox	(TSlotId slot, CInventoryItem* addon, LPCSTR text);
	
	bool						ToSlot						(CUICellItem* itm, bool force_place, TSlotId slot);
	bool						ToBag						(CUICellItem* itm, bool b_use_cursor_pos);
	bool						ToBelt						(CUICellItem* itm, bool b_use_cursor_pos);
	bool						ToArtefactBelt				(CUICellItem* itm, bool b_use_cursor_pos);
	bool						ToQuickSlot					(CUICellItem* itm, u8 particular_slot);

	bool		xr_stdcall		OnItemDragDrop				(CUICellItem* itm);
	bool		xr_stdcall		OnItemStartDrag				(CUICellItem* itm);
	bool		xr_stdcall		OnItemDbClick				(CUICellItem* cellitem);
	bool		xr_stdcall		OnItemSelected				(CUICellItem* itm);
	bool		xr_stdcall		OnItemRButtonClick			(CUICellItem* itm);

	void						SumAmmoByDrop				(CUICellItem* cell_itm, CUIDragDropListEx* ddlist);

	void						SetCurrentItem				(CUICellItem* itm);
	CUICellItem*				CurrentItem					();
	PIItem						CurrentIItem				();

	TIItemContainer				ruck_list;
	TSlotId						m_iCurrentActiveSlot;
};