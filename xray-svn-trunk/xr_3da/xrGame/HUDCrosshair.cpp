// HUDCrosshair.cpp:  крестик прицела, отображающий текущую дисперсию
// 
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "HUDCrosshair.h"
#include "ui_base.h"

CHUDCrosshair::CHUDCrosshair	()
{
	hShader->create				("hud\\crosshair");

	radius = 0;
}


CHUDCrosshair::~CHUDCrosshair	()
{
}

void CHUDCrosshair::Load		()
{
	//все размеры в процентах от длины экрана
	//длина крестика 
	cross_length_perc = pSettings->r_float (HUD_CURSOR_SECTION, "cross_length");
	min_radius_perc = pSettings->r_float (HUD_CURSOR_SECTION, "min_radius");
	max_radius_perc = pSettings->r_float (HUD_CURSOR_SECTION, "max_radius");
	cross_color = pSettings->r_fcolor (HUD_CURSOR_SECTION, "cross_color").get();

	radius_speed_perc = pSettings->r_float (HUD_CURSOR_SECTION, "radius_lerp_speed");
}

//выставляет radius от min_radius до max_radius
void CHUDCrosshair::SetDispersion	(float disp)
{ 
	Fvector4 r;
	Fvector R			= { VIEWPORT_NEAR*_sin(disp), 0.f, VIEWPORT_NEAR };
	Device.mProject.transform	(r,R);

	Fvector2		scr_size;
	scr_size.set	(float(::Render->getTarget()->get_width()), float(::Render->getTarget()->get_height()));
	float radius_pixels		= _abs(r.x)*scr_size.x/2.0f;
	target_radius		= radius_pixels; 
}

extern u32 crosshairAnimationType;
void CHUDCrosshair::OnRender ()
{
	VERIFY(engineState.test(FRAME_RENDERING));

	Fvector2		center;
	Fvector2		scr_size;
	scr_size.set	(float(::Render->getTarget()->get_width()), float(::Render->getTarget()->get_height()));
	center.set		(scr_size.x/2.0f, scr_size.y/2.0f);

	UIRender->StartPrimitive		(10, IUIRender::ptLineList, UI().m_currentPointType);

	float cross_length					= cross_length_perc*scr_size.x;
	float min_radius					= min_radius_perc*scr_size.x;
	float max_radius					= max_radius_perc*scr_size.x;

	clamp								(target_radius , min_radius, max_radius);

	float x_min							= min_radius + radius;
	float x_max							= x_min + cross_length;

	float y_min							= x_min;
	float y_max							= x_max;

	float smeshenie = 20.5f * (scr_size.x / 1280.f);

	if (crosshairAnimationType == 2)
	{
		// Правая
		UIRender->PushPoint(center.x + 1.f + x_min,			center.y - y_min, 0, cross_color, 0, 0);
		UIRender->PushPoint(center.x + smeshenie + x_min,	center.y - y_max * 0.75f, 0, cross_color, 0, 0);
		// Левая
		UIRender->PushPoint(center.x + 1.f - x_min,			center.y - y_min, 0, cross_color, 0, 0);
		UIRender->PushPoint(center.x - smeshenie - x_min,	center.y - y_max * 0.75f, 0, cross_color, 0, 0);
		// Нижняя
		UIRender->PushPoint(center.x + 1.f,					center.y + y_min, 0, cross_color, 0, 0);
		UIRender->PushPoint(center.x + 1.f,					center.y + y_max, 0, cross_color, 0, 0);
	}
	else if (crosshairAnimationType == 3)
	{
		// Правая
		UIRender->PushPoint(center.x + 1.f + x_min,			center.y + y_min,	0, cross_color, 0,0);
		UIRender->PushPoint(center.x + smeshenie + x_min,	center.y + y_max * 0.75f,	0, cross_color, 0,0);
		// Левая
		UIRender->PushPoint(center.x + 1.f - x_min,			center.y + y_min,	0, cross_color, 0,0);
		UIRender->PushPoint(center.x - smeshenie - x_min,	center.y + y_max * 0.75f,	0, cross_color, 0,0);
		// Верхняя
		UIRender->PushPoint(center.x+1,						center.y - y_min,	0, cross_color, 0,0);
		UIRender->PushPoint(center.x+1,						center.y - y_max,	0, cross_color, 0,0);
	}
	else if (crosshairAnimationType == 4)
	{
		// 0 Нижняя
		UIRender->PushPoint(center.x + 1.f, center.y + y_min, 0, cross_color, 0, 0);
		UIRender->PushPoint(center.x + 1.f, center.y + y_max, 0, cross_color, 0, 0);
		// 1 Верхняя
		UIRender->PushPoint(center.x + 1.f, center.y - y_min, 0, cross_color, 0, 0);
		UIRender->PushPoint(center.x + 1.f, center.y - y_max, 0, cross_color, 0, 0);
		// 2 Правая
		UIRender->PushPoint(center.x + x_min + 1.f, center.y, 0, cross_color, 0, 0);
		UIRender->PushPoint(center.x + x_max + 1.f, center.y, 0, cross_color, 0, 0);
		// 3 Левая
		UIRender->PushPoint(center.x - x_min, center.y, 0, cross_color, 0, 0);
		UIRender->PushPoint(center.x - x_max, center.y, 0, cross_color, 0, 0);
	}
	
	// point
	UIRender->PushPoint(center.x,		center.y,			0, cross_color, 0,0);
	UIRender->PushPoint(center.x + 1.f,		center.y,			0, cross_color, 0,0);


	// render	
	UIRender->SetShader(*hShader);
	UIRender->FlushPrimitive		();


	if(!fsimilar(target_radius,radius))
	{

		//cop merge
		//radius = target_radius;
		//return;

		float sp				= radius_speed_perc * scr_size.x ;
		float radius_change		= sp*TimeDelta();
		clamp					(radius_change, 0.0f, sp*0.033f); // clamp to 30 fps
		clamp					(radius_change, 0.0f, _abs(target_radius-radius));
//
		if(target_radius < radius)
			radius -= radius_change;
		else
			radius += radius_change;
	};
}