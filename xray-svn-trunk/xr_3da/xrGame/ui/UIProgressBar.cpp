#include "stdafx.h"
#include "uiprogressbar.h"
#include "UIBtnHint.h"

CUIProgressBar::CUIProgressBar(void)
{
	m_MinPos				= 1.0f;
	m_MaxPos				= 1.0f+EPS;

	Enable					(false);

	m_bBackgroundPresent	= false;
	m_bUseColor				= false;

	AttachChild				(&m_UIBackgroundItem);
	AttachChild				(&m_UIProgressItem);
	m_ProgressPos.x			= 0.0f;
	m_ProgressPos.y			= 0.0f;
	m_inertion				= 0.0f;
	m_last_render_frame		= u32(-1);
	m_orient_mode			= om_horz;
}

CUIProgressBar::~CUIProgressBar(void)
{
}

void CUIProgressBar::InitProgressBar(Fvector2 pos, Fvector2 size, EOrientMode mode)
{
	m_orient_mode			= mode;
	SetWndPos				(pos);
	SetWndSize				(size);
	UpdateProgressBar		();
}

void CUIProgressBar::UpdateProgressBar()
{
	if( fsimilar(m_MaxPos,m_MinPos) ) m_MaxPos	+= EPS;

	float progressbar_unit = 1/(m_MaxPos-m_MinPos);

	float fCurrentLength = m_ProgressPos.x*progressbar_unit;

	if ( m_orient_mode == om_horz || m_orient_mode == om_back )
	{
		m_CurrentLength = GetWidth()*fCurrentLength;
	}
	else if ( m_orient_mode == om_vert || m_orient_mode == om_down )
	{
		m_CurrentLength = GetHeight()*fCurrentLength;
	}
	else
	{
		m_CurrentLength = 0.0f;
	}

	if(m_bUseColor)
	{
		Fcolor curr;
		curr.lerp							(m_minColor,m_middleColor,m_maxColor,fCurrentLength);
		m_UIProgressItem.SetTextureColor	(curr.get());
	}
}

void CUIProgressBar::SetProgressPos(float _Pos)				
{ 
	m_ProgressPos.y		= _Pos; 
	clamp(m_ProgressPos.y,m_MinPos,m_MaxPos);

	if (m_last_render_frame + 1 != CurrentFrame())
		m_ProgressPos.x = m_ProgressPos.y;

	UpdateProgressBar	();
}

float _sign(const float& v)
{
	return (v>0.0f)?+1.0f:-1.0f;
}

void CUIProgressBar::Update()
{
	inherited::Update();
	if(!fsimilar(m_ProgressPos.x, m_ProgressPos.y))
	{
		if( fsimilar(m_MaxPos,m_MinPos) ) m_MaxPos	+= EPS;	//hack ^(
		float _diff				= m_ProgressPos.y - m_ProgressPos.x;
		
		float _length			= (m_MaxPos-m_MinPos);
		float _val				= _length*(1.0f-m_inertion)*TimeDelta();

		_val					= _min(_abs(_val), _abs(_diff) );
		_val					*= _sign(_diff);
		m_ProgressPos.x			+= _val;
		UpdateProgressBar		();
	}

	UpdateHintShow();
}

extern bool is_in2(const Frect& b1, const Frect& b2);

void CUIProgressBar::UpdateHintShow()
{
	if (CursorOverWindow() && m_hint_text.size() && !g_statHint->Owner() && EngineTimeU()>m_dwFocusReceiveTime + 700)
	{
		g_statHint->SetHintText(this, m_hint_text.c_str());

		Fvector2 c_pos = GetUICursor().GetCursorPosition();
		Frect vis_rect;
		vis_rect.set(0, 0, UI_BASE_WIDTH, UI_BASE_HEIGHT);

		//select appropriate position
		Frect r;
		r.set(0.0f, 0.0f, g_statHint->GetWidth(), g_statHint->GetHeight());
		r.add(c_pos.x, c_pos.y);

		r.sub(0.0f, r.height());

		if (false == is_in2(vis_rect, r))
			r.sub(r.width(), 0.0f);
		if (false == is_in2(vis_rect, r))
			r.add(0.0f, r.height());

		if (false == is_in2(vis_rect, r))
			r.add(r.width(), 45.0f);

		g_statHint->SetWndPos(r.lt);
	}

	if (!CursorOverWindow() && g_statHint->Owner() == this)
		g_statHint->Discard();
}

void CUIProgressBar::Draw()
{
	Frect					rect;
	GetAbsoluteRect			(rect);

	if(m_bBackgroundPresent){
		UI().PushScissor	(rect);		
		m_UIBackgroundItem.Draw();
		UI().PopScissor	();
	}

	Frect progress_rect;

	switch ( m_orient_mode )
	{
	case om_horz:
		progress_rect.set	( 0, 0, m_CurrentLength, GetHeight() );
		break;
	case om_vert:
		progress_rect.set	( 0, GetHeight() - m_CurrentLength, GetWidth(), GetHeight() );
		break;
	case om_back:
		progress_rect.set	( GetWidth() - m_CurrentLength * 1.01f, 0, GetWidth(), GetHeight() );
	    break;
	case om_down:
		progress_rect.set	( 0, 0, GetWidth(), m_CurrentLength );
	    break;
	default:
		NODEFAULT;
		break;
	}
	
	if(m_CurrentLength>0){
		Fvector2 pos		= m_UIProgressItem.GetWndPos();	
		progress_rect.add	(rect.left + pos.x,rect.top + pos.y);

		UI().PushScissor	(progress_rect);
		m_UIProgressItem.Draw();
		UI().PopScissor	();
	}

	if (g_statHint->Owner() == this)
		g_statHint->Draw_();

	m_last_render_frame = CurrentFrame();
}