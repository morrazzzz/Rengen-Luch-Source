#include "stdafx.h"
#include "player_hud.h"
#include "level.h"
#include "debug_renderer.h"
#include "../xr_input.h"
#include "HudItem.h"
#include "ui_base.h"


int hud_adj_mode			= 0;
int hud_adj_item_idx		= 0;
int hud_adj_addon_idx		= 0;
shared_str hud_section_name = NULL;
// press SHIFT+NUM
// hud_adj_mode = 0-return 1-hud_pos 2-hud_rot 3-itm_pos 4-itm_rot 5-fire_point 6-fire_2_point 7-shell_point";

float hud_adj_delta_pos			= 0.0005f;
float hud_adj_delta_rot			= 0.05f;

bool is_attachable_item_tuning_mode()
{
	return	pInput->iGetAsyncKeyState(DIK_LSHIFT)	||
			pInput->iGetAsyncKeyState(DIK_Z)		||
			pInput->iGetAsyncKeyState(DIK_X)		||
			pInput->iGetAsyncKeyState(DIK_C);
}

void tune_remap(const Ivector& in_values, Ivector& out_values)
{
	if( pInput->iGetAsyncKeyState(DIK_LSHIFT) )
	{
		out_values = in_values;
	}else
	if( pInput->iGetAsyncKeyState(DIK_Z) )
	{ //strict by X
		out_values.x = in_values.y;
		out_values.y = 0;
		out_values.z = 0;
	}else
	if( pInput->iGetAsyncKeyState(DIK_X) )
	{ //strict by Y
		out_values.x = 0;
		out_values.y = in_values.y;
		out_values.z = 0;
	}else
	if( pInput->iGetAsyncKeyState(DIK_C) )
	{ //strict by Z
		out_values.x = 0;
		out_values.y = 0;
		out_values.z = in_values.y;
	}else
	{
		out_values.set(0,0,0);
	}
}

void calc_cam_diff_pos(Fmatrix item_transform, Fvector diff, Fvector& res)
{
	Fmatrix							cam_m;
	cam_m.i.set						(Device.vCameraRight);
	cam_m.j.set						(Device.vCameraTop);
	cam_m.k.set						(Device.vCameraDirection);
	cam_m.c.set						(Device.vCameraPosition);


	Fvector							res1;
	cam_m.transform_dir				(res1, diff);

	Fmatrix							item_transform_i;
	item_transform_i.invert			(item_transform);
	item_transform_i.transform_dir	(res, res1);
}

void calc_cam_diff_rot(Fmatrix item_transform, Fvector diff, Fvector& res)
{
	Fmatrix							cam_m;
	cam_m.i.set						(Device.vCameraRight);
	cam_m.j.set						(Device.vCameraTop);
	cam_m.k.set						(Device.vCameraDirection);
	cam_m.c.set						(Device.vCameraPosition);

	Fmatrix							R;
	R.identity						();
	if(!fis_zero(diff.x))
	{
		R.rotation(cam_m.i,diff.x);
	}else
	if(!fis_zero(diff.y))
	{
		R.rotation(cam_m.j,diff.y);
	}else
	if(!fis_zero(diff.z))
	{
		R.rotation(cam_m.k,diff.z);
	};

	Fmatrix					item_transform_i;
	item_transform_i.invert	(item_transform);
	R.mulB_43(item_transform);
	R.mulA_43(item_transform_i);
	
	R.getHPB	(res);

	res.mul					(180.0f/PI);
}

void attachable_hud_item::tune(Ivector values)
{
	if(!is_attachable_item_tuning_mode() )
		return;

	Fvector					diff;
	diff.set				(0,0,0);

	if(hud_adj_mode==3 || hud_adj_mode==4 || hud_adj_mode==10 || hud_adj_mode == 11)
	{
		u8 idx = hud_adj_addon_idx;

		if(hud_adj_mode==3 || hud_adj_mode == 10)
		{
			if(values.x)	diff.x = (values.x>0)?hud_adj_delta_pos:-hud_adj_delta_pos;
			if(values.y)	diff.y = (values.y>0)?hud_adj_delta_pos:-hud_adj_delta_pos;
			if(values.z)	diff.z = (values.z>0)?hud_adj_delta_pos:-hud_adj_delta_pos;
			
			Fvector							d;
			Fmatrix							ancor_m;

			if (hud_adj_mode == 10)
			{
				m_parent->calc_transform(0, Fidentity, ancor_m);
				calc_cam_diff_pos(ancor_m, diff, d);
				m_measures.m_custom_addon_offset[0][idx].add(d);
			}
			else
			{
				m_parent->calc_transform(m_attach_place_idx, Fidentity, ancor_m);
				calc_cam_diff_pos(ancor_m, diff, d);
				m_measures.m_item_attach[0].add(d);
			}

		}else
		if(hud_adj_mode==4 || hud_adj_mode == 11)
		{
			if(values.x)	diff.x = (values.x>0)?hud_adj_delta_rot:-hud_adj_delta_rot;
			if(values.y)	diff.y = (values.y>0)?hud_adj_delta_rot:-hud_adj_delta_rot;
			if(values.z)	diff.z = (values.z>0)?hud_adj_delta_rot:-hud_adj_delta_rot;

			Fvector							d;
			Fmatrix							ancor_m;

			if (hud_adj_mode == 11)
			{
				m_parent->calc_transform(0, Fidentity, ancor_m);
				calc_cam_diff_pos(m_item_transform, diff, d);
				m_measures.m_custom_addon_offset[1][idx].add(d);
			}
			else
			{
				m_parent->calc_transform(m_attach_place_idx, Fidentity, ancor_m);
				calc_cam_diff_pos(m_item_transform, diff, d);
				m_measures.m_item_attach[1].add(d);
			}


		}

		if((values.x)||(values.y)||(values.z))
		{

			if (hud_adj_mode == 10 || hud_adj_mode == 11)
			{
				if (hud_section_name == NULL)
					return;

				string64	_prefix;

				switch (idx)
				{
				case 0:
					xr_sprintf(_prefix, "%s", "");
					break;
				case 1:
					xr_sprintf(_prefix, "%s", "sil_");
					break;
				case 2:
					xr_sprintf(_prefix, "%s", "gl_");
					break;
				default:
					break;
				}

				string128	val_name;

				Msg("[%s]", hud_section_name.c_str());
				strconcat(sizeof(val_name), val_name, _prefix, "hud_attach_pos");
				Msg("%s			= %f,%f,%f", val_name,m_measures.m_custom_addon_offset[0][idx].x, m_measures.m_custom_addon_offset[0][idx].y, m_measures.m_custom_addon_offset[0][idx].z);
				strconcat(sizeof(val_name), val_name, _prefix, "hud_attach_rot");
				Msg("%s			= %f,%f,%f", val_name, m_measures.m_custom_addon_offset[1][idx].x, m_measures.m_custom_addon_offset[1][idx].y, m_measures.m_custom_addon_offset[1][idx].z);
				Log("-----------");
			}
			else
			{
				Msg("[%s]", m_sect_name.c_str());
				Msg("item_position				= %f,%f,%f", m_measures.m_item_attach[0].x, m_measures.m_item_attach[0].y, m_measures.m_item_attach[0].z);
				Msg("item_orientation			= %f,%f,%f", m_measures.m_item_attach[1].x, m_measures.m_item_attach[1].y, m_measures.m_item_attach[1].z);
				Log("-----------");
			}


		}
	}



	if(hud_adj_mode==5||hud_adj_mode==6||hud_adj_mode==7)
	{
		if(values.x)	diff.x = (values.x>0)?hud_adj_delta_pos:-hud_adj_delta_pos;
		if(values.y)	diff.y = (values.y>0)?hud_adj_delta_pos:-hud_adj_delta_pos;
		if(values.z)	diff.z = (values.z>0)?hud_adj_delta_pos:-hud_adj_delta_pos;

		if(hud_adj_mode==5)
		{
			m_measures.m_fire_point_offset.add(diff);
		}
		if(hud_adj_mode==6)
		{
			m_measures.m_fire_point2_offset.add(diff);
		}
		if(hud_adj_mode==7)
		{
			m_measures.m_shell_point_offset.add(diff);
		}
		if((values.x)||(values.y)||(values.z))
		{
			Msg("[%s]",					m_sect_name.c_str());
			Msg("fire_point				= %f,%f,%f",m_measures.m_fire_point_offset.x,	m_measures.m_fire_point_offset.y,	m_measures.m_fire_point_offset.z);
			Msg("fire_point2			= %f,%f,%f",m_measures.m_fire_point2_offset.x,	m_measures.m_fire_point2_offset.y,	m_measures.m_fire_point2_offset.z);
			Msg("shell_point			= %f,%f,%f",m_measures.m_shell_point_offset.x,	m_measures.m_shell_point_offset.y,	m_measures.m_shell_point_offset.z);
			Log("-----------");
		}
	}
}

void attachable_hud_item::debug_draw_firedeps()
{
#ifdef DRENDER
	bool bForce = (hud_adj_mode==3||hud_adj_mode==4);

	if(hud_adj_mode==5||hud_adj_mode==6||hud_adj_mode==7 ||bForce)
	{
		CDebugRenderer			&render = Level().debug_renderer();

		firedeps			fd;
		setup_firedeps		(fd);
		
		if(hud_adj_mode==5||bForce)
			render.draw_aabb(fd.vLastFP,0.005f,0.005f,0.005f,D3DCOLOR_XRGB(255,0,0));

		if(hud_adj_mode==6)
			render.draw_aabb(fd.vLastFP2,0.005f,0.005f,0.005f,D3DCOLOR_XRGB(0,0,255));

		if(hud_adj_mode==7)
			render.draw_aabb(fd.vLastSP,0.005f,0.005f,0.005f,D3DCOLOR_XRGB(0,255,0));
	}
#endif // DRENDER
}


void player_hud::tune(Ivector _values)
{
	Ivector				values;
	tune_remap			(_values,values);

	bool is_16x9		= UI().is_widescreen();


	if (hud_adj_mode == 1 || hud_adj_mode == 2)
	{
		Fvector			diff;
		diff.set(0, 0, 0);

		float _curr_dr = hud_adj_delta_rot;

		if ((hud_adj_item_idx > 0) && (m_attached_items[hud_adj_item_idx] == NULL))
			return;

			u8 idx = m_attached_items[hud_adj_item_idx]->m_parent_hud_item->GetCurrentHudOffsetIdx();

			if (idx) {
				_curr_dr /= 20.0f;
			}

			Fvector& pos_ = (idx != 0)?m_attached_items[hud_adj_item_idx]->m_parent_hud_item->hands_offset_pos():m_attached_items[hud_adj_item_idx]->hands_attach_pos();
			Fvector& rot_ = (idx != 0)?m_attached_items[hud_adj_item_idx]->m_parent_hud_item->hands_offset_rot():m_attached_items[hud_adj_item_idx]->hands_attach_rot();


			if (hud_adj_mode == 1)
			{
				if (values.x)	diff.x = (values.x < 0)?hud_adj_delta_pos:-hud_adj_delta_pos;
				if (values.y)	diff.y = (values.y > 0)?hud_adj_delta_pos:-hud_adj_delta_pos;
				if (values.z)	diff.z = (values.z > 0)?hud_adj_delta_pos:-hud_adj_delta_pos;

				pos_.add(diff);
			}

			if (hud_adj_mode == 2)
			{
				if (values.x)	diff.x = (values.x > 0)?_curr_dr:-_curr_dr;
				if (values.y)	diff.y = (values.y > 0)?_curr_dr:-_curr_dr;
				if (values.z)	diff.z = (values.z > 0)?_curr_dr:-_curr_dr;

				rot_.add(diff);
			}
			if ((values.x) || (values.y) || (values.z))
			{
				if (idx == 0)
				{
					Msg("[%s]", m_attached_items[hud_adj_item_idx]->m_sect_name.c_str());
					Msg("hands_position%s				= %f,%f,%f", (is_16x9)?"_16x9":"", pos_.x, pos_.y, pos_.z);
					Msg("hands_orientation%s			= %f,%f,%f", (is_16x9)?"_16x9":"", rot_.x, rot_.y, rot_.z);
					Log("-----------");
				}
				else
					if (idx == 1)
					{
						Msg("[%s]", m_attached_items[hud_adj_item_idx]->m_sect_name.c_str());
						if (m_attached_items[hud_adj_item_idx]->m_parent_hud_item->UseScopeParams())
						{
							Msg("aim_scope_hud_offset_pos%s				= %f,%f,%f", (is_16x9) ? "_16x9" : "", pos_.x, pos_.y, pos_.z);
							Msg("aim_scope_hud_offset_rot%s				= %f,%f,%f", (is_16x9) ? "_16x9" : "", rot_.x, rot_.y, rot_.z);
						}
						else
						{
							Msg("aim_hud_offset_pos%s				= %f,%f,%f", (is_16x9) ? "_16x9" : "", pos_.x, pos_.y, pos_.z);
							Msg("aim_hud_offset_rot%s				= %f,%f,%f", (is_16x9) ? "_16x9" : "", rot_.x, rot_.y, rot_.z);
						}
						Log("-----------");
					}
					else
						if (idx == 2)
						{
							Msg("[%s]", m_attached_items[hud_adj_item_idx]->m_sect_name.c_str());
							if (m_attached_items[hud_adj_item_idx]->m_parent_hud_item->UseScopeParams())
							{
								Msg("gl_scope_hud_offset_pos%s				= %f,%f,%f", (is_16x9) ? "_16x9" : "", pos_.x, pos_.y, pos_.z);
								Msg("gl_scope_hud_offset_rot%s				= %f,%f,%f", (is_16x9) ? "_16x9" : "", rot_.x, rot_.y, rot_.z);
							}
							else
							{
								Msg("gl_hud_offset_pos%s				= %f,%f,%f", (is_16x9) ? "_16x9" : "", pos_.x, pos_.y, pos_.z);
								Msg("gl_hud_offset_rot%s				= %f,%f,%f", (is_16x9) ? "_16x9" : "", rot_.x, rot_.y, rot_.z);
							}
							Log("-----------");
						}
			}
		}
		else
			if (hud_adj_mode == 8 || hud_adj_mode == 9)
			{
				if (hud_adj_mode == 8 && (values.z))
					hud_adj_delta_pos += (values.z > 0)?0.001f:-0.001f;

				if (hud_adj_mode == 9 && (values.z))
					hud_adj_delta_rot += (values.z > 0)?0.1f:-0.1f;
			}
			else
			{
				attachable_hud_item* hi = m_attached_items[hud_adj_item_idx];
				if (!hi)	return;
				hi->tune(values);
			}

	// Load current params from file for addons
	if (pInput->iGetAsyncKeyState(DIK_LSHIFT) && pInput->iGetAsyncKeyState(DIK_DELETE))
	{

	}
}

void hud_draw_adjust_mode()
{
	if (!hud_adj_mode) {
		if (hud_adj_item_idx) {
			hud_adj_item_idx = 0;
		}
		return;
	}

	LPCSTR _text = NULL;
	if(pInput->iGetAsyncKeyState(DIK_LSHIFT) && hud_adj_mode)
		_text = "press SHIFT+NUM 0-return 1-hud_pos 2-hud_rot 3-itm_pos 4-itm_rot 5-fire_point 6-fire_2_point 7-shell_point 8-pos_step 9-rot_step";

	switch (hud_adj_mode)
		{
		case 1:
			_text = "adjusting HUD POSITION";
			break;
		case 2:
			_text = "adjusting HUD ROTATION";
			break;
		case 3:
			_text = "adjusting ITEM POSITION";
			break;
		case 4:
			_text = "adjusting ITEM ROTATION";
			break;
		case 5:
			_text = "adjusting FIRE POINT";
			break;
		case 6:
			_text = "adjusting FIRE 2 POINT";
			break;
		case 7:
			_text = "adjusting SHELL POINT";
			break;
		case 8:
			_text = "adjusting pos STEP";
			break;
		case 9:
			_text = "adjusting rot STEP";
			break;
		case 10:
			_text = "adjusting ADDON POSITION";
			break;
		case 11:
			_text = "adjusting ADDON ROTATION";
			break;

		};
		if(_text)
		{
			CGameFont* F		= UI().Font().pFontDI;
			F->SetAligment		(CGameFont::alCenter);
			F->OutSetI			(0.f,-0.8f);
			F->SetColor			(0xffffffff);
			F->OutNext			(_text);
			if (hud_adj_mode == 10 || hud_adj_mode == 11)
			{
				F->OutNext("for addon [%d]", hud_adj_addon_idx);
				F->OutNext("0 - scope, 1 - silencer, 2 - launcher");
			}
			else 
				F->OutNext("for item [%d]", hud_adj_item_idx);

			F->OutNext			("delta values dP=%f dR=%f", hud_adj_delta_pos, hud_adj_delta_rot);
			F->OutNext			("[Z]-x axis [X]-y axis [C]-z axis");
		}
}

void hud_adjust_mode_keyb(int dik)
{
	if(pInput->iGetAsyncKeyState(DIK_LSHIFT))
	{
		if(dik==DIK_NUMPAD0)
			hud_adj_mode = 0;
		if(dik==DIK_NUMPAD1)
			hud_adj_mode = 1;
		if(dik==DIK_NUMPAD2)
			hud_adj_mode = 2;
		if(dik==DIK_NUMPAD3)
			hud_adj_mode = 3;
		if(dik==DIK_NUMPAD4)
			hud_adj_mode = 4;
		if(dik==DIK_NUMPAD5)
			hud_adj_mode = 5;
		if(dik==DIK_NUMPAD6)
			hud_adj_mode = 6;
		if(dik==DIK_NUMPAD7)
			hud_adj_mode = 7;
		if(dik==DIK_NUMPAD8)
			hud_adj_mode = 8;
		if(dik==DIK_NUMPAD9)
			hud_adj_mode = 9;
	}
	if(pInput->iGetAsyncKeyState(DIK_LCONTROL))
	{
		if (hud_adj_mode == 10 || hud_adj_addon_idx == 11)
		{
			if (dik == DIK_NUMPAD0)
				hud_adj_addon_idx = 0;
			if (dik == DIK_NUMPAD1)
				hud_adj_addon_idx = 1;
			if (dik == DIK_NUMPAD2)
				hud_adj_addon_idx = 2;
		}
		else
		{
			if (dik == DIK_NUMPAD0)
				hud_adj_item_idx = 0;
			if (dik == DIK_NUMPAD1)
				hud_adj_item_idx = 1;
		}



	}
}