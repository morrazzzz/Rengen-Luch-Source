#pragma once

#include "UIStatic.h"
#include "UI3tButton.h"

class CUIInvCategoryTabs : public CUIWindow
{
public:
	CUIInvCategoryTabs();
	virtual					~CUIInvCategoryTabs();

	void 					InitFromXml				(CUIXml& xml_doc, LPCSTR node);

	void					SetConstButtonState		(CUI3tButton* highlited_one, IBState state, bool clear_other = true);
	void					SetHighlitedTab			(u32 mask);

	// "All" tab
	CUI3tButton*				UITabButton1;
	// Weapon tab
	CUI3tButton*				UITabButton2;
	// Outfit tab
	CUI3tButton*				UITabButton3;
	// Ammo tab
	CUI3tButton*				UITabButton4;
	// Medicine tab
	CUI3tButton*				UITabButton5;
	// Food tab
	CUI3tButton*				UITabButton5_2;
	// Devices tab
	CUI3tButton*				UITabButton6;
	// Artefact tab
	CUI3tButton*				UITabButton7;
	// Monster parts tab
	CUI3tButton*				UITabButton8;
	// Misc tab
	CUI3tButton*				UITabButton9;
};