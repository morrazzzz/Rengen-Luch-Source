#include "pch_script.h"

//UI-controls
#include "UIScriptWnd.h"
#include "UIStatic.h"
#include "UIEditBox.h"
#include "UIListBox.h"
#include "UIListWnd.h"
#include "UIFrameWindow.h"
#include "UIFrameLineWnd.h"
#include "UIProgressBar.h"
#include "UITabControl.h"

#include "uiscriptwnd_script.h"

using namespace luabind;

#pragma optimize("s",on)

export_class &script_register_ui_window2(export_class &instance)
{
	instance
		.def("GetStatic",		(CUIStatic* (BaseType::*)(LPCSTR)) &BaseType::GetControl<CUIStatic>)
		.def("GetEditBox",		(CUIEditBox* (BaseType::*)(LPCSTR)) &BaseType::GetControl<CUIEditBox>)
		.def("GetDialogWnd",	(CUIDialogWnd* (BaseType::*)(LPCSTR)) &BaseType::GetControl<CUIDialogWnd>)
		.def("GetFrameWindow",	(CUIFrameWindow* (BaseType::*)(LPCSTR)) &BaseType::GetControl<CUIFrameWindow>)
		.def("GetFrameLineWnd",	(CUIFrameLineWnd* (BaseType::*)(LPCSTR)) &BaseType::GetControl<CUIFrameLineWnd>)
		.def("GetProgressBar",	(CUIProgressBar* (BaseType::*)(LPCSTR)) &BaseType::GetControl<CUIProgressBar>)
		.def("GetTabControl",	(CUITabControl* (BaseType::*)(LPCSTR)) &BaseType::GetControl<CUITabControl>)
		.def("GetListBox",		(CUIListBox* (BaseType::*)(LPCSTR)) &BaseType::GetControl<CUIListBox>)
		.def("GetListWnd",		(CUIListWnd* (BaseType::*)(LPCSTR)) &BaseType::GetControl<CUIListWnd>)

		.def("OnKeyboard",		&BaseType::OnKeyboardAction, &WrapType::OnKeyboard_static)
		.def("Update",			&BaseType::Update, &WrapType::Update_static)
		.def("Dispatch",		&BaseType::Dispatch, &WrapType::Dispatch_static)

	;return	(instance);
}