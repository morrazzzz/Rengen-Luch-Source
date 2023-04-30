#include "pch_script.h"
#include "UIGameCustom.h"
#include "ui/uistatic.h"

using namespace luabind;

CUIGameCustom* get_hud(){
	return CurrentGameUI();
}

#pragma optimize("s",on)
void CUIGameCustom::script_register(lua_State *L)
{
	module(L)
		[
			class_<enum_exporter<IndicatorsBlocks> >("ind_blocks")
			.enum_("indicators_blocks")
			[
				value("IB_DIALOG_1",				int(IB_DIALOG_1)),
				value("IB_BLOODSUCKER_ATTACK",		int(IB_BLOODSUCKER_ATTACK)),
				value("IB_ZOMBIE_ATTACK",			int(IB_ZOMBIE_ATTACK)),
				value("IB_WEAPON",					int(IB_WEAPON)),
				value("IB_BLOODSUCKER_ALIEN",		int(IB_BLOODSUCKER_ALIEN)),
				value("IB_EMPTY_5",					int(IB_EMPTY_5)),
				value("IB_EMPTY_6",					int(IB_EMPTY_6)),
				value("IB_EMPTY_7",					int(IB_EMPTY_7)),
				value("IB_EMPTY_8",					int(IB_EMPTY_8)),
				value("IB_EMPTY_9",					int(IB_EMPTY_9)),
				value("IB_DEVELOPER",				int(IB_DEVELOPER)),
				value("IB_SCRIPT_1",				int(IB_SCRIPT_1)),
				value("IB_SCRIPT_2",				int(IB_SCRIPT_2)),
				value("IB_SCRIPT_3",				int(IB_SCRIPT_3)),
				value("IB_SCRIPT_4",				int(IB_SCRIPT_4)),
				value("IB_SCRIPT_5",				int(IB_SCRIPT_5))
			],

			class_<enum_exporter<HudEffectsBlocks> >("hud_eff_blocks")
			.enum_("hud_effects_blocks")
			[
				value("HUDEFFB_SCRIPT_1",			int(HUDEFFB_SCRIPT_1)),
				value("HUDEFFB_SCRIPT_2",			int(HUDEFFB_SCRIPT_2)),
				value("HUDEFFB_SCRIPT_3",			int(HUDEFFB_SCRIPT_3)),
				value("HUDEFFB_SCRIPT_4",			int(HUDEFFB_SCRIPT_4)),
				value("HUDEFFB_SCRIPT_5",			int(HUDEFFB_SCRIPT_5))
			],

			class_<enum_exporter<CrosshairsBlocks> >("crosshair_blocks")
			.enum_("crosshair_blocks")
			[
				value("CB_DIALOG_1",				int(CB_DIALOG_1)),
				value("CB_BLOODSUCKER_ATTACK",		int(CB_BLOODSUCKER_ATTACK)),
				value("CB_ZOMBIE_ATTACK",			int(CB_ZOMBIE_ATTACK)),
				value("CB_BLOODSUCKER_ALIEN",		int(CB_BLOODSUCKER_ALIEN)),
				value("CB_WEAPON",					int(CB_WEAPON)),
				value("CB_EMPTY_5",					int(CB_EMPTY_5)),
				value("CB_EMPTY_6",					int(CB_EMPTY_6)),
				value("CB_EMPTY_7",					int(CB_EMPTY_7)),
				value("CB_EMPTY_8",					int(CB_EMPTY_8)),
				value("CB_EMPTY_9",					int(CB_EMPTY_9)),
				value("CB_DEVELOPER",				int(CB_DEVELOPER)),
				value("CB_SCRIPT_1",				int(CB_SCRIPT_1)),
				value("CB_SCRIPT_2",				int(CB_SCRIPT_2)),
				value("CB_SCRIPT_3",				int(CB_SCRIPT_3)),
				value("CB_SCRIPT_4",				int(CB_SCRIPT_4)),
				value("CB_SCRIPT_5",				int(CB_SCRIPT_5))
			],

			class_<enum_exporter<GlobalInputBlocks> >("ginput_blocks")
			.enum_("global_input_blocks")
			[
				value("GINPUT_BLOODSUCKER_ATTACK",	int(GINPUT_BLOODSUCKER_ATTACK)),
				value("GINPUT_ZOMBIE_ATTACK",		int(GINPUT_ZOMBIE_ATTACK)),
				value("GINPUT_BLOODSUCKER_ALIEN",	int(GINPUT_BLOODSUCKER_ALIEN)),
				value("GINPUT_SCRIPT_1",			int(GINPUT_SCRIPT_1)),
				value("GINPUT_SCRIPT_2",			int(GINPUT_SCRIPT_2)),
				value("GINPUT_SCRIPT_3",			int(GINPUT_SCRIPT_3)),
				value("GINPUT_SCRIPT_4",			int(GINPUT_SCRIPT_4)),
				value("GINPUT_SCRIPT_5",			int(GINPUT_SCRIPT_5))
			],

			class_<enum_exporter<KeyboardBlocks> >("keyboard_blocks")
			.enum_("keyboard_input_blocks")
			[
				value("KEYB_SCRIPT_1",				int(KEYB_SCRIPT_1)),
				value("KEYB_SCRIPT_2",				int(KEYB_SCRIPT_2)),
				value("KEYB_SCRIPT_3",				int(KEYB_SCRIPT_3)),
				value("KEYB_SCRIPT_4",				int(KEYB_SCRIPT_4)),
				value("KEYB_SCRIPT_5",				int(KEYB_SCRIPT_5))
			],

			class_< SDrawStaticStruct >("SDrawStaticStruct")
			.def_readwrite("m_endTime",		&SDrawStaticStruct::m_endTime)
			.def("wnd",					&SDrawStaticStruct::wnd),

			class_< CUIGameCustom >("CUIGameCustom")
			.def("AddDialogToRender",		&CUIGameCustom::AddDialogToRender)
			.def("RemoveDialogToRender",	&CUIGameCustom::RemoveDialogToRender)
			.def("AddCustomMessage",		(void(CUIGameCustom::*)(LPCSTR, float, float, float, CGameFont*, u16, u32))&CUIGameCustom::AddCustomMessage)
			.def("AddCustomMessage",		(void(CUIGameCustom::*)(LPCSTR, float, float, float, CGameFont*, u16, u32, float))&CUIGameCustom::AddCustomMessage)
			.def("CustomMessageOut",		&CUIGameCustom::CustomMessageOut)
			.def("RemoveCustomMessage",		&CUIGameCustom::RemoveCustomMessage)
			.def("CommonMessageOut",		&CUIGameCustom::CommonMessageOut)
			.def("AddCustomStatic",			&CUIGameCustom::AddCustomStatic)
			.def("RemoveCustomStatic",		&CUIGameCustom::RemoveCustomStatic)

			.def("ShowGameIndicators",		&CUIGameCustom::ShowGameIndicators)
			.def("GameIndicatorsShown",		&CUIGameCustom::GameIndicatorsShown)
			.def("ShowCrosshair",			&CUIGameCustom::ShowCrosshair)
			.def("CrosshairShown",			&CUIGameCustom::CrosshairShown)
			.def("AllowInput",				&CUIGameCustom::AllowInput)
			.def("InputAllowed",			&CUIGameCustom::InputAllowed)
			.def("AllowKeyboard",			&CUIGameCustom::AllowKeyboard)
			.def("KeyboardAllowed",			&CUIGameCustom::KeyboardAllowed)
			.def("ShowHUDEffects",			&CUIGameCustom::ShowHUDEffects)
			.def("HUDEffectsShown",			&CUIGameCustom::HUDEffectsShown)

			.def("HideActorMenu",			&CUIGameCustom::HideActorMenu)
			.def("HidePdaMenu",				&CUIGameCustom::HidePdaMenu)
			.def("show_messages",			&CUIGameCustom::ShowMessagesWindow)
			.def("hide_messages",			&CUIGameCustom::HideMessagesWindow)
			.def("GetCustomStatic",			&CUIGameCustom::GetCustomStatic)
			.def("OnKeyboardAction",		&CUIGameCustom::OnKeyboardAction)
			.def("OnMouseAction",			&CUIGameCustom::OnMouseAction),
#pragma todo("Cop merge: Fix it")
			//.def("update_fake_indicators",	&CUIGameCustom::update_fake_indicators)
			//.def("enable_fake_indicators",	&CUIGameCustom::enable_fake_indicators),
			def("get_hud",					&get_hud)
		];
}
