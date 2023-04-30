#pragma once

#include "UIDialogWnd.h"
#include "UIStatic.h"
#include "../inventory_space.h"
#include "../../_trapezoid.h"

class CUICellItem;

class CUIWeaponSelectWnd : public CUIDialogWnd
{
public:
	CUIWeaponSelectWnd();
	virtual						~CUIWeaponSelectWnd();

	virtual void			Update						();

	virtual void			Draw						();

	virtual void			ShowDialog					(bool bDoHideIndicators);
	virtual void			HideDialog					();

	virtual void			Init						();

	virtual void			SendMessage					(CUIWindow *pWnd, s16 msg, void *pData);
	virtual bool			OnMouseAction				(float x, float y, EUIMessages mouse_action);
	virtual bool			OnKeyboardAction			(int dik, EUIMessages keyboard_action);


	virtual bool			StopAnyMove					() { return false; } // disables exclusive input
#ifndef DEBUG
	virtual bool			NeedDrawCursor				() const { return false; } // don't draw cursor
#endif
			void			UpdateMouse					(float x, float y, EUIMessages mouse_action);

	void					UpdateScaling				();

	enum EWeaponSelectWndSnds
	{
		eSndOpen = 0,
		eSndClose,
		eSlotHighlight,
		eSlotUnhighlight,
		eSlotMax
	};

	ref_sound				sounds[eSlotMax];
protected:

	void					InitializeSlotAreas();

	CUIStatic				UIBack;

	Fvector2				lastCursorPos_;

	CUICellItem*			activeCell_;
	CUICellItem*			prevActiveCell_; // Stores CellIcon of prev active slot

	TSlotId					activateSlot_;
	TSlotId					prevActivateSlot_; // Stores ID of prev active slot

	Fvector2				activateSlotBS_; // stored base size of active cell
	Fvector2				prevActivateSlotBS_; // stored base size of prev active cell

	TSlotId					topLeftSlot_;
	TSlotId					topSlot_;
	TSlotId					topRightSlot_;
	TSlotId					leftSlot_;
	TSlotId					rightSlot_;
	TSlotId					bottomLeftSlot_;
	TSlotId					bottomSlot_;
	TSlotId					bottomRightSlot_;

	CUICellItem*			topLeftIcon_;
	CUICellItem*			topIcon_;
	CUICellItem*			topRightIcon_;
	CUICellItem*			leftIcon_;
	CUICellItem*			rightIcon_;
	CUICellItem*			bottomLeftIcon_;
	CUICellItem*			bottomIcon_;
	CUICellItem*			bottomRightIcon_;

	// For controlling scaling of icon, when it is mouse overed
	float					scale_;

	Fvector2				baseTopLeftIconSize_;
	Fvector2				baseTopIconSize_;
	Fvector2				baseTopRightIconSize_;
	Fvector2				baseLeftIconSize_;
	Fvector2				baseRightIconSize_;
	Fvector2				baseBottomLeftIconSize_;
	Fvector2				baseBottomIconSize_;
	Fvector2				baseBottomRightIconSize_;

	Ftrapezoid				trapezoidTopLeft_;
	Ftrapezoid				trapezoidTop_;
	Ftrapezoid				trapezoidTopRight_;
	Ftrapezoid				trapezoidLeft_;
	Ftrapezoid				trapezoidRight_;
	Ftrapezoid				trapezoidBotLeft_;
	Ftrapezoid				trapezoidBot_;
	Ftrapezoid				trapezoidBotRight_;

	void					ApplySelected		();
	void					CreateIconForSelect(CUICellItem*& cell_icon, Fvector2& size_storage, TSlotId slot, float x_offset, float y_offset);

	void					HighLightCurrentSlot();
};
