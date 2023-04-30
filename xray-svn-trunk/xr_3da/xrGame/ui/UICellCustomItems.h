#pragma once
#include "UICellItem.h"
#include "../Weapon.h"


class CUIInventoryCellItem :public CUICellItem
{
	typedef  CUICellItem	inherited;
protected:
				CUIStatic*		m_upgrade;
				CUIStatic*		m_unique_mark;
				CUIStatic*		m_quest_mark;
				CUIStatic* 		CreateSpecialIcon			(LPCSTR name, bool smaller);
				float			UpdateSpecialIcon			(CUIStatic* icon, Fvector2 pos, bool enable);

				bool			needPortionsResize_;
				bool			needDrawPortion_;

				void			ResizePortionDisplay();
				void			UpdatePortions				(CInventoryItem* item);

	xr_vector<CUIStatic*>		portionsDisplayActive_;
	xr_vector<CUIStatic*>		portionsDisplayInactive_;
public:
								CUIInventoryCellItem		(CInventoryItem* itm, bool quick_slot = false);
	virtual		bool			EqualTo						(CUICellItem* itm);
				CInventoryItem* object						() {return (CInventoryItem*)m_pData;}
				
				void			Update						() override;
};

class CUIAmmoCellItem :public CUIInventoryCellItem
{
	typedef  CUIInventoryCellItem	inherited;
protected:
	virtual		void			UpdateItemText			();
public:
								CUIAmmoCellItem				(CWeaponAmmo* itm);
	virtual		void			Update						();
	virtual		bool			EqualTo						(CUICellItem* itm);
				CWeaponAmmo*	object						() {return (CWeaponAmmo*)m_pData;}
};


class CUIWeaponCellItem :public CUIInventoryCellItem
{
	typedef  CUIInventoryCellItem	inherited;
public:
	enum eAddonType{	eSilencer=0, eScope, eLauncher, eMaxAddon};
	struct SAddon {
					SAddon() { icon = nullptr; }
		CUIStatic*	icon;
		Fvector2	offset;
		bool		drawBehind;
		shared_str	section;
	};

	bool						needAttachablesUpdate_;
protected:
	SAddon						m_addons					[eMaxAddon];
	void						CreateIcon					(eAddonType);
	void						DestroyIcon					(eAddonType);
	void						RefreshOffset				();
	virtual		void			InitAddonForDrag			(CUIDragItem*, const SAddon&, int pos = -1);
	virtual		void			InitAddonsForDrag			(CUIDragItem*, bool behind);
	bool						is_scope					();
	bool						is_silencer					();
	bool						is_launcher					();
				void			DrawAddons					(bool behind);
public:
								CUIWeaponCellItem			(CWeapon* itm);
				virtual			~CUIWeaponCellItem			();
	virtual		void			Update						();
	virtual		void			SetTextureColor				(u32 color);
				void			DrawTexture					() override;

	void						UpdateAddonOrientation		(SAddon& addon, LPCSTR section, bool use_heading);
	void						UpdateAddonOrientation		(CUIStatic*, LPCSTR section, Fvector2 offset, bool use_heading);

	CUIStatic*					GetIcon						(eAddonType);

				CWeapon*		object						() {return (CWeapon*)m_pData;}
	virtual		void 			OnAfterChild				(CUIDragDropListEx* parent_list) override;
	virtual		CUIDragItem*	CreateDragItem				();
	virtual		bool			EqualTo						(CUICellItem* itm);

	SAddon&						GetAttachedAddon			(eAddonType ind) { return m_addons[ind]; };
};

class CBuyItemCustomDrawCell :public ICustomDrawCellItem
{
	CGameFont*			m_pFont;
	string16			m_string;
public:
						CBuyItemCustomDrawCell	(LPCSTR str, CGameFont* pFont);
	virtual void		OnDraw					(CUICellItem* cell);

};
