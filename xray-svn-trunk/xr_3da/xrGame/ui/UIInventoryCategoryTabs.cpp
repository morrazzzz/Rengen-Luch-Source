#include "stdafx.h"
#include "UIInventoryCategoryTabs.h"
#include "UIXmlInit.h"
#include "UIHelper.h"
#include "../inventory_item.h"

CUIInvCategoryTabs::CUIInvCategoryTabs()
{

}

CUIInvCategoryTabs::~CUIInvCategoryTabs()
{

}

void CUIInvCategoryTabs::InitFromXml(CUIXml& xml_doc, LPCSTR node)
{
	CUIXmlInit::InitWindow(xml_doc, node, 0, this);
	{
		R_ASSERT(GetParent());

		UITabButton1 = UIHelper::Create3tButton(xml_doc, "tab_1", this);
		UITabButton1->SetMessageTarget(smart_cast<CUIWindow*>(GetParent()));

		UITabButton2 = UIHelper::Create3tButton(xml_doc, "tab_2", this);
		UITabButton2->SetMessageTarget(smart_cast<CUIWindow*>(GetParent()));

		UITabButton3 = UIHelper::Create3tButton(xml_doc, "tab_3", this);
		UITabButton3->SetMessageTarget(smart_cast<CUIWindow*>(GetParent()));

		UITabButton4 = UIHelper::Create3tButton(xml_doc, "tab_4", this);
		UITabButton4->SetMessageTarget(smart_cast<CUIWindow*>(GetParent()));

		UITabButton5 = UIHelper::Create3tButton(xml_doc, "tab_5", this);
		UITabButton5->SetMessageTarget(smart_cast<CUIWindow*>(GetParent()));

		UITabButton5_2 = UIHelper::Create3tButton(xml_doc, "tab_5_2", this);
		UITabButton5_2->SetMessageTarget(smart_cast<CUIWindow*>(GetParent()));

		UITabButton6 = UIHelper::Create3tButton(xml_doc, "tab_6", this);
		UITabButton6->SetMessageTarget(smart_cast<CUIWindow*>(GetParent()));

		UITabButton7 = UIHelper::Create3tButton(xml_doc, "tab_7", this);
		UITabButton7->SetMessageTarget(smart_cast<CUIWindow*>(GetParent()));

		UITabButton8 = UIHelper::Create3tButton(xml_doc, "tab_8", this);
		UITabButton8->SetMessageTarget(smart_cast<CUIWindow*>(GetParent()));

		UITabButton9 = UIHelper::Create3tButton(xml_doc, "tab_9", this);
		UITabButton9->SetMessageTarget(smart_cast<CUIWindow*>(GetParent()));
	}
}

void CUIInvCategoryTabs::SetConstButtonState(CUI3tButton* highlited_one, IBState state, bool clear_other)
{
	if (clear_other)
	{
		UITabButton1->RemoveConstTextureState();
		UITabButton2->RemoveConstTextureState();
		UITabButton3->RemoveConstTextureState();
		UITabButton4->RemoveConstTextureState();
		UITabButton5->RemoveConstTextureState();
		UITabButton5_2->RemoveConstTextureState();
		UITabButton6->RemoveConstTextureState();
		UITabButton7->RemoveConstTextureState();
		UITabButton8->RemoveConstTextureState();
		UITabButton9->RemoveConstTextureState();
	}

	highlited_one->SetConstTextureState(state);
}


void CUIInvCategoryTabs::SetHighlitedTab(u32 mask)
{
	CUI3tButton* target_button = UITabButton1;

	switch (mask)
	{
	case icAll:
		target_button = UITabButton1;
		break;
	case icWeapons:
		target_button = UITabButton2;
		break;
	case icOutfits:
		target_button = UITabButton3;
		break;
	case icAmmo:
		target_button = UITabButton4;
		break;
	case icMedicine:
		target_button = UITabButton5;
		break;
	case icFood:
		target_button = UITabButton5_2;
		break;
	case icDevices:
		target_button = UITabButton6;
		break;
	case icArtefacts:
		target_button = UITabButton7;
		break;
	case icMutantParts:
		target_button = UITabButton8;
		break;
	case icMisc:
		target_button = UITabButton9;
		break;
	}

	SetConstButtonState(target_button, S_Highlighted);
}