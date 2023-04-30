#include "stdafx.h"
#include "uicursor.h"

#include "ui/UIXmlInit.h"
#include "ui/UIStatic.h"
#include "ui/UIBtnHint.h"
#include "hudmanager.h"


#define C_DEFAULT	D3DCOLOR_XRGB(0xff,0xff,0xff)

CUICursor::CUICursor()
:m_static(NULL),m_b_use_win_cursor(false)
{    
	bVisible				= false;
	needDraw_				= false;
	vPrevPos.set			(0.0f, 0.0f);
	vPos.set				(0.f,0.f);
	InitInternal			();
	Device.seqRender.Add	(this,-3/*2*/);
	Device.seqResolutionChanged.Add(this);
}
//--------------------------------------------------------------------
CUICursor::~CUICursor	()
{
	xr_delete				(m_static);
	Device.seqRender.Remove	(this);
	Device.seqResolutionChanged.Remove(this);
}

void CUICursor::OnScreenResolutionChanged()
{
	xr_delete					(m_static);
	InitInternal				();
}

void CUICursor::InitInternal()
{
	string128 xml_name;
	xr_sprintf(xml_name, "ui_cursor_%d.xml", ui_hud_type);

	CUIXml xml_doc;
	xml_doc.Load(CONFIG_PATH, UI_PATH, xml_name);

	m_static = xr_new <CUIStatic>();

	LPCSTR nodevalue = xml_doc.Read("texture_name", 0, "ui\\ui_ani_cursor");

	m_static->InitTextureEx(nodevalue, "hud\\cursor");

	float r_x = xml_doc.ReadFlt("texture_rect_x", 0, 40.f);
	float r_y = xml_doc.ReadFlt("texture_rect_y", 0, 40.f);

	Frect rect;
	rect.set(0.0f, 0.0f, r_x, r_y);
	m_static->SetTextureRect(rect);

	float width = xml_doc.ReadFlt("width", 0, 40.0f);
	float height = xml_doc.ReadFlt("height", 0, 40.0f);

	BOOL stretch = xml_doc.ReadInt("stretch", 0, TRUE);
	
	m_static->SetWndSize(Fvector2().set(width, height));	
	m_static->SetStretchTexture(!!stretch);

	u32 screen_size_x	= GetSystemMetrics( SM_CXSCREEN );
	u32 screen_size_y	= GetSystemMetrics( SM_CYSCREEN );
	m_b_use_win_cursor	= (screen_size_y >=Device.dwHeight && screen_size_x>=Device.dwWidth);
}

//--------------------------------------------------------------------
u32 last_render_frame = 0;
void CUICursor::OnRender	()
{
	if (Render->currentViewPort != MAIN_VIEWPORT)
		return;

	g_btnHint->OnRender();
	g_statHint->OnRender();

	if(!IsVisible() || !NeedDraw())
		return;
#ifdef DEBUG
	VERIFY(last_render_frame != CurrentFrame());
	last_render_frame = CurrentFrame();

	if(bDebug)
	{
	CGameFont* F		= UI().Font().pFontDI;
	F->SetAligment		(CGameFont::alCenter);
	F->SetHeightI		(0.02f);
	F->OutSetI			(0.f,-0.9f);
	F->SetColor			(0xffffffff);
	Fvector2			pt = GetCursorPosition();
	F->OutNext			("%f-%f",pt.x, pt.y);
	}
#endif

	m_static->SetWndPos	(vPos);
	m_static->Update	();
	m_static->Draw		();
}

Fvector2 CUICursor::GetCursorPosition()
{
	return  vPos;
}

Fvector2 CUICursor::GetCursorPositionDelta()
{
	Fvector2 res_delta;

	res_delta.x = vPos.x - vPrevPos.x;
	res_delta.y = vPos.y - vPrevPos.y;
	return res_delta;
}

void CUICursor::UpdateCursorPosition(int _dx, int _dy)
{
	Fvector2	p;
	vPrevPos	= vPos;
	if(m_b_use_win_cursor)
	{
		POINT		pti;
		BOOL r		= GetCursorPos(&pti);
		if(!r)		return;
		p.x			= (float)pti.x;
		p.y			= (float)pti.y;
		vPos.x		= p.x * (UI_BASE_WIDTH/(float)Device.dwWidth);
		vPos.y		= p.y * (UI_BASE_HEIGHT/(float)Device.dwHeight);
	}else
	{
		float sens = 1.0f;
		vPos.x		+= _dx*sens;
		vPos.y		+= _dy*sens;
	}
	clamp		(vPos.x, 0.f, UI_BASE_WIDTH);
	clamp		(vPos.y, 0.f, UI_BASE_HEIGHT);
}

void CUICursor::SetUICursorPosition(Fvector2 pos)
{
	vPos		= pos;
	POINT		p;
	p.x			= iFloor(vPos.x / (UI_BASE_WIDTH/(float)Device.dwWidth));
	p.y			= iFloor(vPos.y / (UI_BASE_HEIGHT/(float)Device.dwHeight));

	SetCursorPos(p.x, p.y);
}
