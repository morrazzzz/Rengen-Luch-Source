#include "stdafx.h"
#include "x_ray.h"

#include "igame_level.h"
#include "igame_persistent.h"

#include "GameFont.h"


LPCSTR _GetFontTexName(LPCSTR section)
{
	static char* tex_names[] = { "texture800", "texture", "texture1600" };

	int def_idx = 1;//default 1024x768
	int idx = def_idx;

#if 0
	u32 w = Device.dwWidth;

	if (w <= 800)
		idx = 0;
	else if (w <= 1280)
		idx = 1;
	else
		idx = 2;
#else
	u32 h = Device.dwHeight;

	if (h <= 600)
		idx = 0;
	else if (h < 1024)
		idx = 1;
	else
		idx = 2;
#endif

	while (idx >= 0)
	{
		if (pSettings->line_exist(section, tex_names[idx]))
			return pSettings->r_string(section, tex_names[idx]);

		--idx;
	}

	return pSettings->r_string(section, tex_names[def_idx]);
}

void _InitializeFont(CGameFont*& F, LPCSTR section, u32 flags)
{
	LPCSTR font_tex_name = _GetFontTexName(section);

	R_ASSERT(font_tex_name);

	LPCSTR sh_name = pSettings->r_string(section, "shader");

	if (!F)
	{
		F = xr_new <CGameFont>(sh_name, font_tex_name, flags);
	}
	else
		F->Initialize(sh_name, font_tex_name);

	if (pSettings->line_exist(section, "size"))
	{
		float sz = pSettings->r_float(section, "size");

		if (flags&CGameFont::fsDeviceIndependent)
			F->SetHeightI(sz);
		else
			F->SetHeight(sz);
	}

	if (pSettings->line_exist(section, "interval"))
		F->SetInterval(pSettings->r_fvector2(section, "interval"));

}


static CTimer phase_timer;
extern ENGINE_API BOOL g_appLoaded = FALSE;

void CApplication::LoadPhaseBegin(u8 load_phases)
{
	ll_dwReference++;

	if (1 == ll_dwReference)
	{
		g_appLoaded = FALSE;

		_InitializeFont(pFontSystem, "ui_font_graffiti19_russian", 0);

		m_pRender->LoadBegin();

		phase_timer.Start();

		load_stage = 0;
	}

	max_load_stage = load_phases;
}

void CApplication::LoadPhaseEnd()
{
	ll_dwReference--;

	if (0 == ll_dwReference)
	{
		g_appLoaded = TRUE;
	}
}

void CApplication::destroy_loading_shaders()
{
	m_pRender->destroy_loading_shaders();
}

#include "render.h"

void CApplication::LoadDraw()
{
	if (g_appLoaded)
		return;

	u32 frame = CurrentFrame();
	SetCurrentFrame(frame + 1);

	engineState.set(FRAME_RENDERING, true);

	Render->firstViewPort = MAIN_VIEWPORT;
	Render->lastViewPort = MAIN_VIEWPORT;
	Render->currentViewPort = MAIN_VIEWPORT;
	Render->viewPortsThisFrame.push_back(MAIN_VIEWPORT);

	Device.m_pRender->SwitchViewPortRTZB(MAIN_VIEWPORT);

	if (!Device.BeginRendering(MAIN_VIEWPORT))
		return;

	load_draw_internal();

	Device.FinishRendering(MAIN_VIEWPORT);
	Render->viewPortsThisFrame.clear();
	engineState.set(FRAME_RENDERING, false);
}

void CApplication::LoadTitleInt(LPCSTR str1, LPCSTR str2, LPCSTR str3, LPCSTR log_msg)
{
	xr_strcpy(ls_header, str1);

	if (xr_strcmp(str2, "keep"))
	{
		xr_strcpy(ls_tip_number, str2);
		xr_strcpy(ls_tip, str3);
	}

	LoadStage(log_msg);
}

void CApplication::ClearTitle()
{
	ls_header[0] = '\0';
}

void CApplication::LoadStage(LPCSTR log_msg)
{
	load_stage++;

	VERIFY(ll_dwReference);

	Msg("\n* Phase end: Time: [%d ms], Mem usage [%u KB]", phase_timer.GetElapsed_ms(), Device.Statistic->GetTotalRAMConsumption() / 1024);
	Msg("# New Phase: %s\n", log_msg);

	phase_timer.Start();
	LoadDraw();
}


#pragma optimize("g", off)

void CApplication::load_draw_internal()
{
	m_pRender->load_draw_internal(*this);
}