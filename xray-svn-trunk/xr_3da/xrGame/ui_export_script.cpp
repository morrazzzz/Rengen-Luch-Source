#include "pch_script.h"

#include "script_ui_registrator.h"
#include "UI\UIMultiTextStatic.h"
#include "MainMenu.h"

#include "UIGameCustom.h"
#include "UI/UIScriptWnd.h"
#include "UI/UIButton.h"
#include "UI/UIProgressBar.h"
#include "UI/UIEditBox.h"
#include "UI/UIListWnd.h"
#include "UI/UIMessageBox.h"
#include "UI/UIPropertiesBox.h"
#include "UI/UITabControl.h"
#include "UI/UIComboBox.h"
#include "ui/UIOptionsManagerScript.h"
#include "ScriptXmlInit.h"

using namespace luabind;

CMainMenu*	MainMenu();

#pragma optimize("s",on)
void UIRegistrator::script_register(lua_State *L)
{
	CUIWindow::script_register(L);
	CUIStatic::script_register(L);
	CUIButton::script_register(L);
	CUIProgressBar::script_register(L);
	CUIComboBox::script_register(L);
	CUIEditBox::script_register(L);
	CUITabControl::script_register(L);
	CUIMessageBox::script_register(L);
	CUIListBox::script_register(L);
	CUIListWnd::script_register(L);
	CUIDialogWndEx::script_register(L);
	CUIPropertiesBox::script_register(L);
	CUIOptionsManagerScript::script_register(L);
	CScriptXmlInit::script_register(L);
	CUIGameCustom::script_register(L);

	module(L)
	[

		class_<CGameFont>("CGameFont")
			.enum_("EAligment")
			[
				value("alLeft",						int(CGameFont::alLeft)),
				value("alRight",					int(CGameFont::alRight)),
				value("alCenter",					int(CGameFont::alCenter))
			],

		class_<CUICaption>("CUICaption")
			.def("addCustomMessage",	&CUICaption::addCustomMessage)
			.def("setCaption",			&CUICaption::setCaption),

		class_<CMainMenu>("CMainMenu")
			.def("GetGSVer",				&CMainMenu::GetEngineVer)
			.def("IsActive",				&CMainMenu::IsActive)
	];
	module(L,"main_menu")
	[
		def("get_main_menu",				&MainMenu)
	];

}
