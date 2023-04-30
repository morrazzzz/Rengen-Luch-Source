#include "stdafx.h"
#include "ConstantDebug.h"
#include "GameFont.h"
#pragma hdrstop

CConstantsDebug *g_pConstantsDebug = NULL;

CConstantsDebug::CConstantsDebug()
{
	Clear();
}

CConstantsDebug::~CConstantsDebug()
{
	Clear();
}

#define VPUSH3(v) v.x,v.y,v.z
#define VPUSH4(v) v.x,v.y,v.z,v.w

#include "Render.h"

void CConstantsDebug::OnRender(CGameFont* font)
{
	if (Render->currentViewPort != MAIN_VIEWPORT)
		return;
	
	for (VEC3_STORAGE_IT it = m_vecs3.begin(), last = m_vecs3.end(); it != last; ++it)
	{
		font->OutNext("[%s]=[%.5f,%.5f,%.5f]", *(it->first), VPUSH3(it->second));
	}
	for (VEC4_STORAGE_IT it = m_vecs4.begin(), last = m_vecs4.end(); it != last; ++it)
	{
		font->OutNext("[%s]=[%.5f,%.5f,%.5f,%.5f]", *(it->first), VPUSH4(it->second));
	}
}
