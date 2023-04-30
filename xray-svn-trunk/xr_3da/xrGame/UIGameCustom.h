#ifndef __XR_UIGAMECUSTOM_H__
#define __XR_UIGAMECUSTOM_H__
#pragma once

#include "script_export_space.h"
#include "object_interfaces.h"
#include "inventory_space.h"
#include "UIDialogHolder.h"
#include "../CustomHUD.h"
#include "ui/UIMessages.h"
#include "ui/UIWeatherEditor.h"

// refs
class CUI;
class game_cl_GameState;
class CUIDialogWnd;
class CUICaption;
class CUIStatic;
class CUIWindow;
class CUIXml;
class CUIInventoryWnd;
class CUITradeWnd;
class CUIPdaWnd;
class CUIStashWnd;
class CUIMainIngameWnd;
class CUIMessagesWindow;
class CUIWeatherEditor;
class CUIWeaponSelectWnd;

enum EGameTypes;

enum IndicatorsBlocks
{
		IB_DIALOG_1 = (1 << 0),
		IB_BLOODSUCKER_ATTACK = (1 << 1),
		IB_ZOMBIE_ATTACK = (1 << 2),
		IB_WEAPON = (1 << 3),
		IB_BLOODSUCKER_ALIEN = (1 << 4),
		IB_EMPTY_5 = (1 << 5),
		IB_EMPTY_6 = (1 << 6),
		IB_EMPTY_7 = (1 << 7),
		IB_EMPTY_8 = (1 << 8),
		IB_EMPTY_9 = (1 << 9),
		IB_DEVELOPER = (1 << 10),
		IB_SCRIPT_1 = (1 << 11),
		IB_SCRIPT_2 = (1 << 12),
		IB_SCRIPT_3 = (1 << 13),
		IB_SCRIPT_4 = (1 << 14),
		IB_SCRIPT_5 = (1 << 15)
};

enum HudEffectsBlocks
{
		HUDEFFB_SCRIPT_1 = (1 << 0),
		HUDEFFB_SCRIPT_2 = (1 << 1),
		HUDEFFB_SCRIPT_3 = (1 << 2),
		HUDEFFB_SCRIPT_4 = (1 << 3),
		HUDEFFB_SCRIPT_5 = (1 << 4)
};

enum CrosshairsBlocks
{
		CB_DIALOG_1 = (1 << 0),
		CB_BLOODSUCKER_ATTACK = (1 << 1),
		CB_ZOMBIE_ATTACK = (1 << 2),
		CB_BLOODSUCKER_ALIEN = (1 << 3),
		CB_WEAPON = (1 << 4),
		CB_EMPTY_5 = (1 << 5),
		CB_EMPTY_6 = (1 << 6),
		CB_EMPTY_7 = (1 << 7),
		CB_EMPTY_8 = (1 << 8),
		CB_EMPTY_9 = (1 << 9),
		CB_DEVELOPER = (1 << 10),
		CB_SCRIPT_1 = (1 << 11),
		CB_SCRIPT_2 = (1 << 12),
		CB_SCRIPT_3 = (1 << 13),
		CB_SCRIPT_4 = (1 << 14),
		CB_SCRIPT_5 = (1 << 15)
};

enum GlobalInputBlocks
{
		GINPUT_BLOODSUCKER_ATTACK = (1 << 0),
		GINPUT_ZOMBIE_ATTACK = (1 << 1),
		GINPUT_BLOODSUCKER_ALIEN = (1 << 2),
		GINPUT_SCRIPT_1 = (1 << 3),
		GINPUT_SCRIPT_2 = (1 << 4),
		GINPUT_SCRIPT_3 = (1 << 5),
		GINPUT_SCRIPT_4 = (1 << 6),
		GINPUT_SCRIPT_5 = (1 << 7)
};

enum KeyboardBlocks
{
		KEYB_SCRIPT_1 = (1 << 0),
		KEYB_SCRIPT_2 = (1 << 1),
		KEYB_SCRIPT_3 = (1 << 2),
		KEYB_SCRIPT_4 = (1 << 3),
		KEYB_SCRIPT_5 = (1 << 4)
};

struct SDrawStaticStruct :public IPureDestroyableObject
{
	SDrawStaticStruct	();
	virtual	void	destroy			();
	CUIStatic*		m_static;
	float			m_endTime;
	shared_str		m_name;
	void			Draw();
	void			Update();
	CUIStatic*		wnd()		{return m_static;}
	bool			IsActual()	const;
	void			SetText		(LPCSTR);

	bool			onTop;
};

class CUIGameCustom :public DLL_Pure, public CDialogHolder
{
protected:
	CUIWindow*			m_window;
	CUICaption*			GameCaptions			() {return m_pgameCaptions;}
	CUICaption*			m_pgameCaptions;
	CUIXml*				m_msgs_xml;

	typedef xr_vector<SDrawStaticStruct*>	st_vec;
	typedef st_vec::iterator				st_vec_it;
	st_vec									m_custom_statics;

	Flags32				gameIndicatorsBlocks_;
	Flags32				gameHUDEffectsBlocks_;
	Flags32				crosshairBlocks_;
	Flags32				ginputBlocks_;
	Flags32				keyboardBlocks_;

public:
	CUIMainIngameWnd*	UIMainIngameWnd;
	CUIInventoryWnd*	m_InventoryMenu;
	CUIMessagesWindow*	m_pMessagesWnd;
	CUIPdaWnd*			m_PdaMenu;
	CUIStashWnd*		m_UIStashWnd;
	CUIWeatherEditor*	m_WeatherEditor;
	CUIWeaponSelectWnd*	m_WeaponSelect;

	virtual	void		ScheduledUpdate			(u32 dt) {};
	
						CUIGameCustom			();
	virtual				~CUIGameCustom			();

	virtual	void		Init					(int stage)	{};
	
	virtual void		Render					();
	virtual void		OnFrame					();

			bool		ShowActorMenu			();
			void		HideActorMenu			();
			bool		ShowPdaMenu				();
			void		HidePdaMenu				();
			void		ShowMessagesWindow		();
			void		HideMessagesWindow		();
	void				UpdatePda				();
	void				update_fake_indicators	(u8 type, float power);
	void				enable_fake_indicators	(bool enable);

	void			ShowGameIndicators		(IndicatorsBlocks block_id, bool show_ind)
	{
		gameIndicatorsBlocks_.set(block_id, !show_ind);
#if 0
		Msg("Block hud indicators. Current block masks = %u", gameIndicatorsBlocks_.get());
#endif
	};

	bool			GameIndicatorsShown		()														{ return gameIndicatorsBlocks_.flags == 0; };
	bool			HasGameIndicatorsBlock	(IndicatorsBlocks block_id)								{ return !!gameIndicatorsBlocks_.test(block_id); };
	void			ShowCrosshair			(CrosshairsBlocks block_id, bool shwo_cr)				{ crosshairBlocks_.set(block_id, !shwo_cr); }
	bool			CrosshairShown			()														{ return crosshairBlocks_.flags == 0; };
	bool			HasCrosshairBlock		(CrosshairsBlocks block_id)								{ return !!crosshairBlocks_.test(block_id); };
	void			AllowInput				(GlobalInputBlocks block_id, bool allow)				{ ginputBlocks_.set(block_id, !allow); }
	bool			InputAllowed			()														{ return ginputBlocks_.flags == 0; };
	bool			HasInputBlock			(GlobalInputBlocks block_id)							{ return !!ginputBlocks_.test(block_id); };
	void			AllowKeyboard			(KeyboardBlocks block_id, bool allow)					{ keyboardBlocks_.set(block_id, !allow); }
	bool			KeyboardAllowed			()														{ return keyboardBlocks_.flags == 0; };
	bool			HasKeyboardBlock		(KeyboardBlocks block_id)								{ return !!keyboardBlocks_.test(block_id);};
	void			ShowHUDEffects			(HudEffectsBlocks eff_block_id, bool show_effects)		{ gameHUDEffectsBlocks_.set(eff_block_id, !show_effects); g_hud->showGameHudEffects_ = gameHUDEffectsBlocks_.flags == 0; };
	bool			HUDEffectsShown			()														{ return g_hud->showGameHudEffects_; };
	bool			HasHUDEffectsBlock		(HudEffectsBlocks eff_block_id)							{ return !!gameHUDEffectsBlocks_.test(eff_block_id); };

	virtual void		HideShownDialogs		(){};
	virtual void		HideInputReceavingDialog(){};
	virtual void		ReInitShownUI			(){};
	virtual void		ReinitDialogs			(){};

	virtual void		AddCustomMessage		(LPCSTR id, float x, float y, float font_size, CGameFont *pFont, u16 alignment, u32 color);
	virtual void		AddCustomMessage		(LPCSTR id, float x, float y, float font_size, CGameFont *pFont, u16 alignment, u32 color, float flicker );
	virtual void		CustomMessageOut		(LPCSTR id, LPCSTR msg, u32 color);
	virtual void		RemoveCustomMessage		(LPCSTR id);

	SDrawStaticStruct*	AddCustomStatic			(LPCSTR id, bool bSingleInstance);
	SDrawStaticStruct*	GetCustomStatic			(LPCSTR id);
	void				RemoveCustomStatic		(LPCSTR id);

	void				CommonMessageOut		(LPCSTR text);

	IC CUIPdaWnd&		PdaMenu					() const { return *m_PdaMenu;   }

	virtual bool		OnKeyboardAction					(int dik, EUIMessages keyboard_action) {return false;};
	virtual bool		OnMouseAction					(float x, float y, EUIMessages mouse_action) {return false;};
	
	virtual void		UnLoad					();
	void				Load					();

	virtual	void					reset_ui				();
	void				OnConnected				();

	CUIXml*				m_actor_menu_item;

	virtual bool		EscapePressed			()		{return false;};

	DECLARE_SCRIPT_REGISTER_FUNCTION
}; // class CUIGameCustom
extern CUIGameCustom*		CurrentGameUI();


#endif