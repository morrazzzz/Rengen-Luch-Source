#include "stdafx.h"
#include "UIWeaponSelectWnd.h"

#include "UIXmlInit.h"
#include "UIHelper.h"
#include "../hudmanager.h"
#include "../Actor.h"
#include "../Inventory.h"
#include "UICellItem.h"
#include "UICellItemFactory.h"
#include "UICellCustomItems.h"

float CURSOR_MAX_DIST_CENTER_X = 90.f;
float CURSOR_MAX_DIST_CENTER_Y = 90.f;
float CENTER_X = 512.0f;
float CENTER_Y = 384.0f;
float SELECTED_ICON_SIZE_K = 1.4f;
float ICON_CELL_SIZE_X = 40.f;
float ICON_CELL_SIZE_Y = 40.f;
float INACTIVE_CENTER_RADIUS = 30.f;

#define SCALE_ICON_SPEED 6.f

u32 high_light_color = 0;
u32 default_light_color = 0;

CUIWeaponSelectWnd::CUIWeaponSelectWnd()
{
	Init();

	lastCursorPos_ = { 0, 0 };

	topLeftSlot_ = KNIFE_SLOT;
	topSlot_ = PISTOL_SLOT;
	topRightSlot_ = DETECTOR_SLOT;
	leftSlot_ = RIFLE_SLOT;
	rightSlot_ = RIFLE_2_SLOT;
	bottomLeftSlot_ = BOLT_SLOT;
	bottomSlot_ = GRENADE_SLOT;
	bottomRightSlot_ = APPARATUS_SLOT;

	activateSlot_ = NO_ACTIVE_SLOT;
	prevActivateSlot_ = NO_ACTIVE_SLOT;

	topLeftIcon_ = nullptr;
	topIcon_ = nullptr;
	topRightIcon_ = nullptr;
	leftIcon_ = nullptr;
	rightIcon_ = nullptr;
	bottomLeftIcon_ = nullptr;
	bottomIcon_ = nullptr;
	bottomRightIcon_ = nullptr;

	activeCell_ = nullptr;
	prevActiveCell_ = nullptr;

	scale_ = 1.f;
}

CUIWeaponSelectWnd::~CUIWeaponSelectWnd()
{
	xr_delete(topLeftIcon_);
	xr_delete(topIcon_);
	xr_delete(topRightIcon_);
	xr_delete(leftIcon_);
	xr_delete(rightIcon_);
	xr_delete(bottomLeftIcon_);
	xr_delete(bottomIcon_);
	xr_delete(bottomRightIcon_);
}

void CUIWeaponSelectWnd::Init()
{
	CUIXml uiXml;

	string128 XML_NAME;
	xr_sprintf(XML_NAME, "weapon_select_%d.xml", ui_hud_type);

	bool xml_result = uiXml.Load(CONFIG_PATH, UI_PATH, XML_NAME);
	R_ASSERT3(xml_result, "file parsing error ", uiXml.m_xml_file_name);

	CUIXmlInit xml_init;

	xml_init.InitWindow(uiXml, "main", 0, this);

	// background texture holder
	AttachChild(&UIBack);
	xml_init.InitStatic(uiXml, "back", 0, &UIBack);

	// Read basic coordinates and limits
	CENTER_X = uiXml.ReadFlt("center_x", 0, 512.f);
	CENTER_Y = uiXml.ReadFlt("center_y", 0, 384.f);

	CURSOR_MAX_DIST_CENTER_X = uiXml.ReadFlt("cursor_max_area_x", 0, 90.f);
	CURSOR_MAX_DIST_CENTER_Y = uiXml.ReadFlt("cursor_max_area_y", 0, 90.f);

	INACTIVE_CENTER_RADIUS = uiXml.ReadFlt("inactive_center_area_radius", 0, 30.f);

	// Init mouse areas
	InitializeSlotAreas();

	// Center texture
	UIBack.SetWndPos(CENTER_X - UIBack.GetWndSize().x / 2, CENTER_Y - UIBack.GetWndSize().y / 2);

	// Read colors for default and highlighted icons
	int r = uiXml.ReadInt("high_light_color_r", 0, 255);
	int g = uiXml.ReadInt("high_light_color_g", 0, 255);
	int b = uiXml.ReadInt("high_light_color_b", 0, 255);
	int a = uiXml.ReadInt("high_light_color_a", 0, 255);

	high_light_color = color_rgba(r, g, b, a);

	r = uiXml.ReadInt("default_light_color_r", 0, 255);
	g = uiXml.ReadInt("default_light_color_g", 0, 255);
	b = uiXml.ReadInt("default_light_color_b", 0, 255);
	a = uiXml.ReadInt("default_light_color_a", 0, 255);

	default_light_color = color_rgba(r, g, b, a);

	// Scale selected slot icon bythis value
	SELECTED_ICON_SIZE_K = uiXml.ReadFlt("selected_icon_zoom", 0, 1.4f);

	// Size of cells of icon
	ICON_CELL_SIZE_X = uiXml.ReadFlt("icon_cell_size_x", 0, 40.f);
	ICON_CELL_SIZE_Y = uiXml.ReadFlt("icon_cell_size_y", 0, 40.f);

	// Assign slots to real inventory slots
	topLeftSlot_ = (TSlotId)uiXml.ReadInt("top_left_inv_slot", 0, KNIFE_SLOT);
	topSlot_ = (TSlotId)uiXml.ReadInt("top_inv_slot", 0, PISTOL_SLOT);
	topRightSlot_ = (TSlotId)uiXml.ReadInt("top_right_inv_slot", 0, DETECTOR_SLOT);
	leftSlot_ = (TSlotId)uiXml.ReadInt("left_inv_slot", 0, RIFLE_SLOT);
	rightSlot_ = (TSlotId)uiXml.ReadInt("right_inv_slot", 0, RIFLE_2_SLOT);
	bottomLeftSlot_ = (TSlotId)uiXml.ReadInt("bot_left_inv_slot", 0, BOLT_SLOT);
	bottomSlot_ = (TSlotId)uiXml.ReadInt("bot_right_inv_slot", 0, GRENADE_SLOT);
	bottomRightSlot_ = (TSlotId)uiXml.ReadInt("bot_inv_slot", 0, APPARATUS_SLOT);

	activateSlot_ = NO_ACTIVE_SLOT;
	prevActivateSlot_ = NO_ACTIVE_SLOT;
	activeCell_ = nullptr;
	prevActiveCell_ = nullptr;

	// Load sounds
	uiXml.SetLocalRoot(uiXml.NavigateToNode("action_sounds", 0));

	::Sound->create(sounds[eSndOpen], uiXml.Read("snd_open", 0, NULL), st_Effect, sg_SourceType);
	::Sound->create(sounds[eSndClose], uiXml.Read("snd_close", 0, NULL), st_Effect, sg_SourceType);
	::Sound->create(sounds[eSlotHighlight], uiXml.Read("snd_selected", 0, NULL), st_Effect, sg_SourceType);
	::Sound->create(sounds[eSlotUnhighlight], uiXml.Read("snd_unselected_any", 0, NULL), st_Effect, sg_SourceType);
}

void CUIWeaponSelectWnd::InitializeSlotAreas()
{
	float total_rect_X_length = CURSOR_MAX_DIST_CENTER_X * 2.f;
	float total_rect_Y_length = CURSOR_MAX_DIST_CENTER_Y * 2.f;

	Fvector2 A, B, C, D;

	A.set(CENTER_X - CURSOR_MAX_DIST_CENTER_X, CENTER_Y - CURSOR_MAX_DIST_CENTER_Y);
	B.set(CENTER_X - CURSOR_MAX_DIST_CENTER_X, CENTER_Y - CURSOR_MAX_DIST_CENTER_Y + (total_rect_Y_length / 3));
	C.set(CENTER_X, CENTER_Y);
	D.set(CENTER_X - CURSOR_MAX_DIST_CENTER_X + (total_rect_X_length / 3), CENTER_Y - CURSOR_MAX_DIST_CENTER_Y);

	trapezoidTopLeft_.set(A, B, C, D);

	A.set(CENTER_X - CURSOR_MAX_DIST_CENTER_X + (total_rect_X_length / 3), CENTER_Y - CURSOR_MAX_DIST_CENTER_Y);
	B.set(CENTER_X + CURSOR_MAX_DIST_CENTER_X - (total_rect_X_length / 3), CENTER_Y - CURSOR_MAX_DIST_CENTER_Y);
	C.set(CENTER_X, CENTER_Y);
	D.set(CENTER_X, CENTER_Y);

	trapezoidTop_.set(A, B, C, D);

	A.set(CENTER_X + CURSOR_MAX_DIST_CENTER_X, CENTER_Y - CURSOR_MAX_DIST_CENTER_Y);
	B.set(CENTER_X + CURSOR_MAX_DIST_CENTER_X, CENTER_Y - CURSOR_MAX_DIST_CENTER_Y + (total_rect_Y_length / 3));
	C.set(CENTER_X, CENTER_Y);
	D.set(CENTER_X + CURSOR_MAX_DIST_CENTER_X - (total_rect_X_length / 3), CENTER_Y - CURSOR_MAX_DIST_CENTER_Y);

	trapezoidTopRight_.set(A, B, C, D);

	A.set(CENTER_X - CURSOR_MAX_DIST_CENTER_X, CENTER_Y - CURSOR_MAX_DIST_CENTER_Y + (total_rect_Y_length / 3));
	B.set(CENTER_X - CURSOR_MAX_DIST_CENTER_X, CENTER_Y + CURSOR_MAX_DIST_CENTER_Y - (total_rect_Y_length / 3));
	C.set(CENTER_X, CENTER_Y);
	D.set(CENTER_X, CENTER_Y);

	trapezoidLeft_.set(A, B, C, D);

	A.set(CENTER_X + CURSOR_MAX_DIST_CENTER_X, CENTER_Y - CURSOR_MAX_DIST_CENTER_Y + (total_rect_Y_length / 3));
	B.set(CENTER_X + CURSOR_MAX_DIST_CENTER_X, CENTER_Y + CURSOR_MAX_DIST_CENTER_Y - (total_rect_Y_length / 3));
	C.set(CENTER_X, CENTER_Y);
	D.set(CENTER_X, CENTER_Y);

	trapezoidRight_.set(A, B, C, D);

	A.set(CENTER_X - CURSOR_MAX_DIST_CENTER_X, CENTER_Y + CURSOR_MAX_DIST_CENTER_Y);
	B.set(CENTER_X - CURSOR_MAX_DIST_CENTER_X, CENTER_Y + CURSOR_MAX_DIST_CENTER_Y - (total_rect_Y_length / 3));
	C.set(CENTER_X, CENTER_Y);
	D.set(CENTER_X - CURSOR_MAX_DIST_CENTER_X + (total_rect_X_length / 3), CENTER_Y + CURSOR_MAX_DIST_CENTER_Y);

	trapezoidBotLeft_.set(A, B, C, D);

	A.set(CENTER_X - CURSOR_MAX_DIST_CENTER_X + (total_rect_X_length / 3), CENTER_Y + CURSOR_MAX_DIST_CENTER_Y);
	B.set(CENTER_X + CURSOR_MAX_DIST_CENTER_X - (total_rect_X_length / 3), CENTER_Y + CURSOR_MAX_DIST_CENTER_Y);
	C.set(CENTER_X, CENTER_Y);
	D.set(CENTER_X, CENTER_Y);

	trapezoidBot_.set(A, B, C, D);

	A.set(CENTER_X + CURSOR_MAX_DIST_CENTER_X, CENTER_Y + CURSOR_MAX_DIST_CENTER_Y);
	B.set(CENTER_X + CURSOR_MAX_DIST_CENTER_X, CENTER_Y + CURSOR_MAX_DIST_CENTER_Y - (total_rect_Y_length / 3));
	C.set(CENTER_X, CENTER_Y);
	D.set(CENTER_X + CURSOR_MAX_DIST_CENTER_X - (total_rect_X_length / 3), CENTER_Y + CURSOR_MAX_DIST_CENTER_Y);

	trapezoidBotRight_.set(A, B, C, D);
}

void CUIWeaponSelectWnd::Update()
{
	UpdateScaling();

	if (topLeftIcon_)
		topLeftIcon_->Update();

	if (topIcon_)
		topIcon_->Update();

	if (topRightIcon_)
		topRightIcon_->Update();

	if (leftIcon_)
		leftIcon_->Update();

	if (rightIcon_)
		rightIcon_->Update();

	if (bottomLeftIcon_)
		bottomLeftIcon_->Update();

	if (bottomIcon_)
		bottomIcon_->Update();

	if (bottomRightIcon_)
		bottomRightIcon_->Update();
}

void CUIWeaponSelectWnd::ShowDialog(bool bDoHideIndicators)
{
	CUIDialogWnd::ShowDialog(bDoHideIndicators);

	if (sounds[eSndOpen]._handle())
		sounds[eSndOpen].play(NULL, sm_2D);

	activateSlot_ = NO_ACTIVE_SLOT;
	prevActivateSlot_ = NO_ACTIVE_SLOT;
	lastCursorPos_.set(GetUICursor().GetCursorPosition());

	CreateIconForSelect(topLeftIcon_, baseTopLeftIconSize_, topLeftSlot_, -150.f, -120);
	CreateIconForSelect(topIcon_, baseTopIconSize_, topSlot_, 0.f, -120);
	CreateIconForSelect(topRightIcon_, baseTopRightIconSize_, topRightSlot_, 150.f, -120);
	CreateIconForSelect(leftIcon_, baseLeftIconSize_, leftSlot_, -150.f, 0.f);
	CreateIconForSelect(rightIcon_, baseRightIconSize_, rightSlot_, 150.f, 0.f);
	CreateIconForSelect(bottomLeftIcon_, baseBottomLeftIconSize_, bottomLeftSlot_, -150.f, 120);
	CreateIconForSelect(bottomIcon_, baseBottomIconSize_, bottomSlot_, 0.f, 120);
	CreateIconForSelect(bottomRightIcon_, baseBottomRightIconSize_, bottomRightSlot_, 150.f, 120);

	Update();
}

void CUIWeaponSelectWnd::HideDialog()
{
	CUIDialogWnd::HideDialog();

	if (sounds[eSndClose]._handle())
		sounds[eSndClose].play(NULL, sm_2D);

	ApplySelected();

	xr_delete(topLeftIcon_);
	xr_delete(topIcon_);
	xr_delete(topRightIcon_);
	xr_delete(leftIcon_);
	xr_delete(rightIcon_);
	xr_delete(bottomLeftIcon_);
	xr_delete(bottomIcon_);
	xr_delete(bottomRightIcon_);

	activeCell_ = nullptr;
	prevActiveCell_ = nullptr;
}

void CUIWeaponSelectWnd::Draw()
{
	CUIDialogWnd::Draw();

	if (topLeftIcon_)
		topLeftIcon_->Draw();
	if (topIcon_)
		topIcon_->Draw();
	if (topRightIcon_)
		topRightIcon_->Draw();
	if (leftIcon_)
		leftIcon_->Draw();
	if (rightIcon_)
		rightIcon_->Draw();
	if (bottomLeftIcon_)
		bottomLeftIcon_->Draw();
	if (bottomIcon_)
		bottomIcon_->Draw();
	if (bottomRightIcon_)
		bottomRightIcon_->Draw();
}

void CUIWeaponSelectWnd::ApplySelected()
{
	if (activateSlot_ != NO_ACTIVE_SLOT)
		Actor()->cast_inventory_owner()->inventory().ActivateSlot(activateSlot_);
}

void CUIWeaponSelectWnd::SendMessage(CUIWindow *pWnd, s16 msg, void *pData)
{
	CUIWindow::SendMessage(pWnd, msg, pData);
}

bool CUIWeaponSelectWnd::OnMouseAction(float x, float y, EUIMessages mouse_action)
{
	if (mouse_action == WINDOW_CBUTTON_UP)
		return false;

	UpdateMouse(x, y, mouse_action);

	HighLightCurrentSlot();

	CUIWindow::OnMouseAction(x, y, mouse_action);

	return true; // always returns true, to avoid input for other receivers
}

bool CUIWeaponSelectWnd::OnKeyboardAction(int dik, EUIMessages keyboard_action)
{
	if (CUIDialogWnd::OnKeyboardAction(dik, keyboard_action))
		return true;

	return false;
}

void CUIWeaponSelectWnd::UpdateMouse(float x, float y, EUIMessages mouse_action)
{
	Fvector2 center_p = { CENTER_X, CENTER_Y };

	Fvector2 cp = GetUICursor().GetCursorPosition();

	// Check if cursor is not out of needed radius
	float dist_from_center_x = abs(center_p.x - x);
	float dist_from_center_y = abs(center_p.y - y);

	if (dist_from_center_x > CURSOR_MAX_DIST_CENTER_X - 5.f)
		GetUICursor().SetUICursorPosition(Fvector2().set(lastCursorPos_.x, y));

	cp = GetUICursor().GetCursorPosition();

	if (dist_from_center_y > CURSOR_MAX_DIST_CENTER_Y - 5.f)
		GetUICursor().SetUICursorPosition(Fvector2().set(cp.x, lastCursorPos_.y));

	cp = GetUICursor().GetCursorPosition();

	// Store last position, and apply it, if cursor gets out of needed radius
	lastCursorPos_ = cp;

	float dist_from_center = center_p.distance_to(cp);

	// Detect in what selection rect we are
	//Msg("%f %f  %f %f   %f %f", center.x1, center.x2, center.y1, center.y2, cp.x, cp.y);
	
	TSlotId active_slot_before = activateSlot_;
	CUICellItem* active_cell_before = activeCell_;
	Fvector2 active_cell_size_before = activateSlotBS_;

	if (dist_from_center <= INACTIVE_CENTER_RADIUS)
	{
		activateSlot_ = NO_ACTIVE_SLOT;
		activeCell_ = nullptr;
		activateSlotBS_ = Fvector2().set(0, 0);
	}
	else if (trapezoidTopLeft_.point_belongs(cp))
	{
		activateSlot_ = topLeftSlot_;
		activeCell_ = topLeftIcon_;
		activateSlotBS_ = baseTopLeftIconSize_;
	}
	else if (trapezoidTopRight_.point_belongs(cp))
	{
		activateSlot_ = topRightSlot_;
		activeCell_ = topRightIcon_;
		activateSlotBS_ = baseTopRightIconSize_;
	}
	else if (trapezoidBotLeft_.point_belongs(cp))
	{
		activateSlot_ = bottomLeftSlot_;
		activeCell_ = bottomLeftIcon_;
		activateSlotBS_ = baseBottomLeftIconSize_;
	}
	else if (trapezoidBotRight_.point_belongs(cp))
	{
		activateSlot_ = bottomRightSlot_;
		activeCell_ = bottomRightIcon_;
		activateSlotBS_ = baseBottomRightIconSize_;
	}
	else if (trapezoidLeft_.point_belongs(cp))
	{
		activateSlot_ = leftSlot_;
		activeCell_ = leftIcon_;
		activateSlotBS_ = baseLeftIconSize_;
	}
	else if (trapezoidRight_.point_belongs(cp))
	{
		activateSlot_ = rightSlot_;
		activeCell_ = rightIcon_;
		activateSlotBS_ = baseRightIconSize_;
	}
	else if (trapezoidTop_.point_belongs(cp))
	{
		activateSlot_ = topSlot_;
		activeCell_ = topIcon_;
		activateSlotBS_ = baseTopIconSize_;
	}
	else if (trapezoidBot_.point_belongs(cp))
	{
		activateSlot_ = bottomSlot_;
		activeCell_ = bottomIcon_;
		activateSlotBS_ = baseBottomIconSize_;
	}

	if (active_slot_before != activateSlot_) // changed
	{
		scale_ = 1.f;

		if (activateSlot_ == NO_ACTIVE_SLOT)
		{
			if (sounds[eSlotUnhighlight]._handle())
				sounds[eSlotUnhighlight].play(NULL, sm_2D);
		}
		else if (sounds[eSlotHighlight]._handle())
			sounds[eSlotHighlight].play(NULL, sm_2D);

		prevActivateSlot_ = active_slot_before;
		prevActiveCell_ = active_cell_before;
		prevActivateSlotBS_ = active_cell_size_before;
	}
}

void CUIWeaponSelectWnd::CreateIconForSelect(CUICellItem*& cell_icon, Fvector2& size_storage, TSlotId slot, float x_offset, float y_offset)
{
	CInventory& inventory = Actor()->cast_inventory_owner()->inventory();
	CInventoryItem* item = inventory.ItemFromSlot(slot);

	if (slot == GRENADE_SLOT)
	{
		PIItem grenade_item = inventory.GetNotSameBySlotNum(GRENADE_SLOT, NULL, inventory.belt_);

		if (grenade_item)
			item = grenade_item;
	}

	if (!item)
		return;

	cell_icon = create_cell_item(item);

	// get size from item cfg and multiply by coeficient
	float x_size = item->GetGridWidth() * ICON_CELL_SIZE_X;
	float y_size = item->GetGridHeight() * ICON_CELL_SIZE_Y;

	size_storage = Fvector2().set(x_size, y_size);

	// Set position
	float x_position = (CENTER_X + x_offset) - (x_size / 2);
	float y_position = (CENTER_Y + y_offset) - (y_size / 2);;

	cell_icon->SetWndSize(x_size, y_size);
	cell_icon->SetWndPos(x_position, y_position);

	Update();
}

void CUIWeaponSelectWnd::HighLightCurrentSlot()
{
	// Outline the selected cell icon

	if (activeCell_)
	{
		activeCell_->SetTextureColor(high_light_color);
	}

	if (prevActiveCell_)
	{
		prevActiveCell_->SetTextureColor(default_light_color);
	}
}

void CUIWeaponSelectWnd::UpdateScaling()
{
	if (activeCell_)
	{
		if (scale_ < SELECTED_ICON_SIZE_K)
			scale_ += SCALE_ICON_SPEED * TimeDelta();

		activeCell_->SetWndSize(activateSlotBS_.x * scale_, activateSlotBS_.y * scale_);

		// Update attached addons position
		CUIWeaponCellItem* wpn_cell = smart_cast<CUIWeaponCellItem*>(activeCell_);

		if (wpn_cell)
		{
			wpn_cell->needAttachablesUpdate_ = true;
		}
	}

	if (prevActiveCell_)
	{
		prevActiveCell_->SetWndSize(prevActivateSlotBS_.x, prevActivateSlotBS_.y);

		// Update attached addons position
		CUIWeaponCellItem* wpn_cell = smart_cast<CUIWeaponCellItem*>(prevActiveCell_);

		if (wpn_cell)
		{
			wpn_cell->needAttachablesUpdate_ = true;
		}
	}
}
