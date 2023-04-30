#include "stdafx.h"
#include "../xr_ioconsole.h"
#include "../fdemorecord.h"
#include "level.h"
#include "xr_level_controller.h"
#include "xrServer.h"
#include "actor.h"
#include "UIGameCustom.h"
#include "ui/UIDialogWnd.h"
#include "pch_script.h"
#include "ui/UIGameTutorial.h"
#include "../xr_input.h"
#include "saved_game_wrapper.h"
#include "ai_space.h"
#include "script_engine.h"
#include "gamepersistent.h"

#ifdef LOG_PLANNER
	extern void try_change_current_entity();
	extern void restore_actor();
#endif

bool g_bDisableAllInput			= false;
bool g_bDisableKeyboardInput	= false;

extern int quick_save_counter;
extern int max_quick_saves;

#define CURRENT_ENTITY()	(level_game_cl_base?CurrentEntity():NULL)

void CLevel::IR_OnMouseWheel( int direction )
{
	if (g_bDisableAllInput || !CurrentGameUI()->InputAllowed()) return;

	if (CurrentGameUI()->IR_UIOnMouseWheel(direction)) return;

	if( Device.Paused()		) return;
	if (CURRENT_ENTITY())		{
			IInputReceiver*		IR	= smart_cast<IInputReceiver*>	(smart_cast<CGameObject*>(CURRENT_ENTITY()));
			if (IR)				IR->IR_OnMouseWheel(direction);
		}

}

static int mouse_button_2_key []	=	{MOUSE_1,MOUSE_2,MOUSE_3};

void CLevel::IR_OnMousePress(int btn)
{	IR_OnKeyboardPress(mouse_button_2_key[btn]);}
void CLevel::IR_OnMouseRelease(int btn)
{	IR_OnKeyboardRelease(mouse_button_2_key[btn]);}
void CLevel::IR_OnMouseHold(int btn)
{	IR_OnKeyboardHold(mouse_button_2_key[btn]);}

void CLevel::IR_OnMouseMove( int dx, int dy )
{
	if (g_bDisableAllInput || !CurrentGameUI()->InputAllowed())
		return;

	if (CurrentGameUI()->IR_UIOnMouseMove(dx,dy))		return;
	if (Device.Paused())							return;
	if (CURRENT_ENTITY())		{
		IInputReceiver*		IR	= smart_cast<IInputReceiver*>	(smart_cast<CGameObject*>(CURRENT_ENTITY()));
		if (IR)				IR->IR_OnMouseMove					(dx,dy);
	}
}

extern CUISequencer* g_tutorial;


void CLevel::IR_OnKeyboardPress	(int key)
{
	bool b_ui_exist = (!!CurrentGameUI());

	if (DIK_NUMPAD5 == key)
	{
		bool state = Console->GetBool("rs_global_statistics_switch");

		if (state)
			Console->Execute("rs_global_statistics_switch 0");
		else
			Console->Execute("rs_global_statistics_switch 1");

		return;
	}

	EGameActions _curr = get_binded_action(key);
	switch ( _curr ) 
	{

	case kSCREENSHOT:
		string256 scr_additional_name;

		xr_sprintf(scr_additional_name, sizeof(scr_additional_name), "w_%s_to_%s_gt_%s", g_pGamePersistent->Environment().Current[0]->m_identifier.c_str(),
			g_pGamePersistent->Environment().Current[1]->m_identifier.c_str(), GetGameTimeAsString(ETimePrecision::etpTimeToMinutes, '-').c_str());

		Msg("^ Additional screenshot descr.: %s", scr_additional_name);

		Render->Screenshot(IRender_interface::SM_NORMAL, scr_additional_name);
		return;
		break;

	case kCONSOLE:
		Console->Show				();
		return;
		break;

	case kQUIT: 
		{
			if (b_ui_exist && CurrentGameUI()->TopInputReceiver())
			{
				if (CurrentGameUI()->IR_UIOnKeyboardPress(key))
					return; //special case for mp and main_menu

				if (CurrentGameUI()->EscapePressed())
					return;
			}
			else
			{
				Console->Execute("main_menu");
			}return;
		}break;
	case kPAUSE:
			Device.Pause(!Device.Paused(), TRUE, TRUE, "li_pause_key");
		return;

		break;

	};
	
	if (g_bDisableAllInput || g_bDisableKeyboardInput)
		return;

	if(CurrentGameUI() && (!CurrentGameUI()->InputAllowed() || !CurrentGameUI()->KeyboardAllowed()))
		return;

	if ( !bReady || !b_ui_exist )			return;

	if ( b_ui_exist && CurrentGameUI()->IR_UIOnKeyboardPress(key)) return;

	if( Device.Paused() )		return;


	if(_curr == kQUICK_SAVE)
	{
		luabind::functor<bool> lua_bool;
		bool script_handeled = false;

		R_ASSERT2(ai().script_engine().functor("input.quick_save", lua_bool), "input.quick_save");

		if (lua_bool.is_valid()){
			script_handeled = lua_bool();
		}

		if (script_handeled) { Msg("# Quick Save handeled by script"); return; } // Return if script handeled quick save

		if (Actor() && Actor()->b_saveAllowed && Actor()->g_Alive())
		{
			quick_save_counter++;
			if (quick_save_counter > max_quick_saves) //bimd the index to max_quick_saves
			{
				quick_save_counter = 0;
			}

			string_path					saved_game, command;

			xr_sprintf(saved_game, "%s_quicksave_%i", Core.UserName, quick_save_counter); //make a savefile name using user name and save index

			xr_sprintf(command, "save %s", saved_game); //make a command for console
			Msg("%s", command); //temporary

			Console->Execute(command);
		}
		else
		{
			// Calls refuse message on hud from script
			luabind::functor<void> lua_func;
			R_ASSERT2(ai().script_engine().functor("input.quicksave_refuse", lua_func), "Can't find input.quicksave_refuse");
			lua_func();
		}
		return;
	}


	if(_curr == kQUICK_LOAD)
	{
		luabind::functor<bool> lua_bool;
		bool script_handeled = false;

		R_ASSERT2(ai().script_engine().functor("input.quick_load", lua_bool), "input.quick_load");

		if (lua_bool.is_valid()){
			script_handeled = lua_bool();
		}

		if (script_handeled) { Msg("# Quick Load handeled by script"); return; } // Return if script handeled quick load


		string_path					saved_game, command;

		xr_sprintf(saved_game, "%s_quicksave_%i", Core.UserName, quick_save_counter); //make a savefile name using user name and save index

		if (!CSavedGameWrapper::valid_saved_game(saved_game)){
			Msg("!Invalid save %s", saved_game);
			return;
		}

		if (g_tutorial && g_tutorial->IsActive()) {
			g_tutorial->Stop();
		}

		xr_sprintf(command, "load %s", saved_game); //make a command for console
		Msg("%s", command); //temporary

		Console->Execute			(command);
		return;
	}

	if (bindConsoleCmds.execute(key))
		return;

	if (CURRENT_ENTITY())
	{
			IInputReceiver*		IR	= smart_cast<IInputReceiver*>	(smart_cast<CGameObject*>(CURRENT_ENTITY()));
			if (IR)				IR->IR_OnKeyboardPress(get_binded_action(key));
	}
}

void CLevel::IR_OnKeyboardRelease(int key)
{
	if (!bReady || g_bDisableAllInput || (CurrentGameUI() && !CurrentGameUI()->InputAllowed()))
		return;
	if ( CurrentGameUI() && CurrentGameUI()->IR_UIOnKeyboardRelease(key)) return;
	if (Device.Paused() )				return;

	if (CURRENT_ENTITY())		
	{
		IInputReceiver*		IR	= smart_cast<IInputReceiver*>	(smart_cast<CGameObject*>(CURRENT_ENTITY()));
		if (IR)				IR->IR_OnKeyboardRelease			(get_binded_action(key));
	}
}

void CLevel::IR_OnKeyboardHold(int key)
{
	if (g_bDisableAllInput || g_bDisableKeyboardInput || (CurrentGameUI() && (!CurrentGameUI()->InputAllowed() || !CurrentGameUI()->KeyboardAllowed()))) return;
	if (CurrentGameUI() && CurrentGameUI()->IR_UIOnKeyboardHold(key)) return;
	if (Device.Paused()) return;
	if (CURRENT_ENTITY())
	{
		IInputReceiver*		IR	= smart_cast<IInputReceiver*>	(smart_cast<CGameObject*>(CURRENT_ENTITY()));
		if (IR)				IR->IR_OnKeyboardHold				(get_binded_action(key));
	}
}

void CLevel::IR_OnMouseStop( int /**axis/**/, int /**value/**/)
{
}

void CLevel::IR_OnActivate()
{
	if(!pInput) return;
	int i;
	for (i = 0; i < CInput::COUNT_KB_BUTTONS; i++ )
	{
		if(IR_GetKeyState(i))
		{

			EGameActions action = get_binded_action(i);
			switch (action){
			case kFWD			:
			case kBACK			:
			case kL_STRAFE		:
			case kR_STRAFE		:
			case kLEFT			:
			case kRIGHT			:
			case kUP			:
			case kDOWN			:
			case kCROUCH		:
			case kACCEL			:
			case kL_LOOKOUT		:
			case kR_LOOKOUT		:	
			case kWPN_FIRE		:
				{
					IR_OnKeyboardPress	(i);
				}break;
			};
		};
	}
}