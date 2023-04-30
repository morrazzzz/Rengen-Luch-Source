#include "stdafx.h"
#include "UIOutfitSlot.h"
#include "UIStatic.h"
#include "UICellItem.h"
#include "../CustomOutfit.h"
#include "../actor.h"
#include "UIInventoryUtilities.h"

CUIOutfitDragDropList::CUIOutfitDragDropList()
{
	m_background				= xr_new <CUIStatic>();
	m_background->SetAutoDelete	(true);
	AttachChild					(m_background);
	m_default_outfit			= "npc_icon_without_outfit";
}

CUIOutfitDragDropList::~CUIOutfitDragDropList()
{
}

#include "../level.h"

void CUIOutfitDragDropList::SetOutfit(CUICellItem* itm)
{
	m_background->SetWndPos				(Fvector2().set(0,0));
	m_background->SetWndSize			(Fvector2().set(GetWidth(), GetHeight()));
	m_background->SetStretchTexture		(true);

	if(itm)
	{
		PIItem _iitem	= (PIItem)itm->m_pData;
		CCustomOutfit* pOutfit = smart_cast<CCustomOutfit*>(_iitem); VERIFY(pOutfit);

		m_background->InitTexture			(pOutfit->GetFullIconName().c_str());
	} else
		m_background->InitTexture		("npc_icon_without_outfit");


	m_background->TextureOn				();
}

void CUIOutfitDragDropList::SetDefaultOutfit(LPCSTR default_outfit){
	m_default_outfit = default_outfit;
}

void CUIOutfitDragDropList::SetItem(CUICellItem* itm)
{
	if(itm)	inherited::SetItem			(itm);
	SetOutfit							(itm);
}

void CUIOutfitDragDropList::SetItem(CUICellItem* itm, Fvector2 abs_pos)
{
	if(itm)	inherited::SetItem			(itm, abs_pos);
	SetOutfit							(itm);
}

void CUIOutfitDragDropList::SetItem(CUICellItem* itm, Ivector2 cell_pos)
{
	if(itm)	inherited::SetItem			(itm, cell_pos);
	SetOutfit							(itm);
}

CUICellItem* CUIOutfitDragDropList::RemoveItem(CUICellItem* itm, bool force_root)
{
	VERIFY								(!force_root);
	CUICellItem* ci						= inherited::RemoveItem(itm, force_root);
	SetOutfit							(nullptr);
	return								ci;
}

void CUIOutfitDragDropList::ClearAll(bool bDestroy)
{
	inherited::ClearAll(bDestroy);
	SetOutfit(nullptr);
}


void CUIOutfitDragDropList::Draw()
{
	m_background->Draw					();
//.	inherited::Draw						();
}