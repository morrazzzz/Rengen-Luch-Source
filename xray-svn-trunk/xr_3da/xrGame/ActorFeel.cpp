#include "stdafx.h"
#include "actor.h"
#include "weapon.h"
#include "inventory.h"
#include "character_info.h"
#include "UsableScriptObject.h"
#include "customzone.h"
#include "../GameMtlLib.h"
#include "ui/UIMainIngameWnd.h"
#include "UIGameCustom.h"
#include "Grenade.h"
#include "game_cl_base.h"
#include "Level.h"
#include "GameConstants.h"
#include "HUDManager.h"


#define PICKUP_INFO_COLOR_FAREST 0xFF686054
#define PICKUP_INFO_COLOR_MIDLE 0xFF776E61
#define PICKUP_INFO_COLOR_NEAREST 0xFF897F70


void CActor::feel_touch_new				(CObject* O)
{
}

void CActor::feel_touch_delete	(CObject* O)
{
	CPhysicsShellHolder* sh=smart_cast<CPhysicsShellHolder*>(O);
	if(sh&&sh->character_physics_support()) m_feel_touch_characters--;
}

BOOL CActor::feel_touch_contact		(CObject *O)
{
	CInventoryItem	*item = smart_cast<CInventoryItem*>(O);
	CInventoryOwner	*inventory_owner = smart_cast<CInventoryOwner*>(O);

	if (item && item->Useful() && !item->object().H_Parent()) 
		return TRUE;

	if(inventory_owner && inventory_owner != smart_cast<CInventoryOwner*>(this))
	{
		CPhysicsShellHolder* sh=smart_cast<CPhysicsShellHolder*>(O);
		if(sh&&sh->character_physics_support()) m_feel_touch_characters++;
		return TRUE;
	}

	return		(FALSE);
}

BOOL CActor::feel_touch_on_contact	(CObject *O)
{
	CCustomZone	*custom_zone = smart_cast<CCustomZone*>(O);
	if (!custom_zone)
		return	(TRUE);

	Fsphere		sphere;
	Center		(sphere.P);
	sphere.R	= 0.1f;
	if (custom_zone->inside(sphere))
		return	(TRUE);

	return		(FALSE);
}

ICF static BOOL info_trace_callback(collide::rq_result& result, LPVOID params)
{
	BOOL& bOverlaped	= *(BOOL*)params;
	if(result.O)
	{
		if (Level().CurrentEntity() == result.O)
		{ //ignore self-actor
			return			TRUE;
		}
		else
		{ //check obstacle flag
			if (result.O->spatial.s_type&STYPE_OBSTACLE)
				bOverlaped = TRUE;

			return			TRUE;
		}
	}
	else
	{
		//получить треугольник и узнать его материал
		CDB::TRI* T = Level().ObjectSpace.GetStaticTris() + result.element;
		if (GMLib.GetMaterialByIdx(T->material)->Flags.is(SGameMtl::flPassable) && !GMLib.GetMaterialByIdx(T->material)->Flags.is(SGameMtl::flDynamic)){
			return TRUE;
		}
	}	
	bOverlaped			= TRUE;
	return				FALSE;
}

BOOL CActor::CanPickItem(const CFrustum& frustum, const Fvector& from, CObject* item)
{
	if (!item->getVisible())
		return FALSE;

	BOOL	bOverlaped		= FALSE;
	Fvector dir,to; 
	item->Center			(to);
	float range				= dir.sub(to,from).magnitude();

	if (range>0.25f){
		if (frustum.testSphere_dirty(to,item->Radius())){
			dir.div						(range);
			collide::ray_defs			RD(from, dir, range, CDB::OPT_CULL, collide::rqtBoth);
			VERIFY						(!fis_zero(RD.dir.square_magnitude()));
			RQR.r_clear					();
			Level().ObjectSpace.RayQuery(RQR,RD, info_trace_callback, &bOverlaped, NULL, item);
		}
	}
	return !bOverlaped;
}

#include "ai\monsters\ai_monster_utils.h"

void CActor::NearItemsUpdate()
{
	CFrustum frustum;
	frustum.CreateFromMatrix(Device.mFullTransform,FRUSTUM_P_LRTB|FRUSTUM_P_FAR);

	//. slow (ray-query test)
	for(xr_vector<CObject*>::iterator it = feel_touch.begin(); it != feel_touch.end(); it++)
		if (CanPickItem(frustum, Device.vCameraPosition, *it))
			PickupInfoDraw(*it);
}

void CActor::PickupItemsUpdate()
{
	if (eacFirstEye != cam_active)
		feel_touch_update(Position(), m_fPickupInfoRadius);
	else
	{
		Fvector pos = get_bone_position(this, "bip01_spine");
		feel_touch_update(pos, m_fPickupInfoRadius);
	}

	// Use ray queury res, if possible. If not, check if we have item in "ok" frustum

	auto& RQ = HUD().GetCurrentRayQuery();

	float dist_to_obj = RQ.range;

	if (RQ.O && eacFirstEye != cam_active)
		dist_to_obj = get_bone_position(this, "bip01_spine").distance_to((smart_cast<CGameObject*>(RQ.O))->Position());

	if (RQ.O && dist_to_obj < inventory().GetTakeDist())
		inventory().m_pTarget = smart_cast<PIItem>(RQ.O);
	else
		inventory().m_pTarget = nullptr;

	// Find one focused pickup and group pickup
	pickUpItems.clear();

	CFrustum group_frustum, single_frustum;
	Fmatrix m_projection, m_full;

	m_projection.build_projection(deg2rad(GameConstants::GetSinglePickUpFov()), 1.f, 0.2f, inventory().GetTakeDist());
	m_full.mul(m_projection, Device.mView);
	single_frustum.CreateFromMatrix(m_full, FRUSTUM_P_LRTB | FRUSTUM_P_FAR);

	m_projection.build_projection(deg2rad(GameConstants::GetGroupPickUpFov()), 1.f, 0.2f, inventory().GetTakeDist());
	m_full.mul(m_projection, Device.mView);
	group_frustum.CreateFromMatrix(m_full, FRUSTUM_P_LRTB | FRUSTUM_P_FAR);

	float last_dist = inventory().GetTakeDist();

	u32 group_max = GameConstants::GetGroupPickUpLimit();

	Fvector bone_pos;

	if (eacFirstEye != cam_active)
		bone_pos = get_bone_position(this, "bip01_spine");
	else
		bone_pos = get_bone_position(this, "bip01_head");

	for (xr_vector<CObject*>::iterator it = feel_touch.begin(); it != feel_touch.end(); it++)
	{
		CObject* obj = *it;

		CInventoryItem* item = smart_cast<CInventoryItem*>(obj);

		if (item)
		{
			Fvector pos;
			obj->Center(pos);

			// Find nearest for group pickup
			if (pickUpItems.size() < group_max)
			{
				u32	mask = group_frustum.getMask();

				if (group_frustum.testSphere(pos, obj->Radius(), mask))
				{
					pickUpItems.push_back(item);
				}
			}

			// if ray queury did not find item - find it in "ok to pick up" frustum
			if (!inventory().m_pTarget)
			{
				// Find one focused for single pickup
				u32	mask = single_frustum.getMask();

				if (single_frustum.testSphere(pos, obj->Radius(), mask))
				{
					float d1 = bone_pos.distance_to(obj->Position());

					if (d1 < last_dist)
					{
						inventory().m_pTarget = item;
						last_dist = d1;
					}
				}
			}
		}
	}

	if (inventory().m_pTarget && !pickUpLongInProgress) // dont show info, if actor ties to pick up group
		CurrentGameUI()->UIMainIngameWnd->SetPickUpItem(inventory().m_pTarget);
	else
		CurrentGameUI()->UIMainIngameWnd->SetPickUpItem(nullptr);
}

void CActor::PickupInfoDraw(CObject* object)
{
	CInventoryItem* item = smart_cast<CInventoryItem*>(object);
	if(!item || !item->IsPickUpVisible())		return;

	LPCSTR draw_str 	= item->NameShort();
	Fmatrix			res;
	res.mul			(Device.mFullTransform,object->XFORM());
	Fvector4		v_res;
	Fvector			shift;

	Fvector dir, to;
	object->Center(to);
	float range = dir.sub(to, Device.vCameraPosition).magnitude();

	shift.set(0,0,0);

	res.transform(v_res,shift);

	if (v_res.z < 0 || v_res.w < 0)	return;
	if (v_res.x < -1.f || v_res.x > 1.f || v_res.y<-1.f || v_res.y>1.f) return;

	float x = (1.f + v_res.x)/2.f * (Device.dwWidth);
	float y = (1.f - v_res.y)/2.f * (Device.dwHeight);

	float convertedX = (x  / Device.dwWidth);
	float convertedY = (y  / Device.dwHeight);
	convertedX = 1280 * convertedX;
	convertedY = 720 * convertedY;

	if (draw_str && convertedX > 380 && convertedX < 900 && convertedY > 200 && convertedY <660)
	{
		if (range < 2.f){
			UI().Font().pFontGraffiti22Russian->SetAligment(CGameFont::alCenter);
			UI().Font().pFontGraffiti22Russian->SetColor(PICKUP_INFO_COLOR_NEAREST);
			UI().Font().pFontGraffiti22Russian->Out(x, y, draw_str);
		}
		else if(range < 4.f){
			UI().Font().pFontLetterica18Russian->SetAligment(CGameFont::alCenter);
			UI().Font().pFontLetterica18Russian->SetColor(PICKUP_INFO_COLOR_MIDLE);
			UI().Font().pFontLetterica18Russian->Out(x, y, draw_str);
		}else
		{
			UI().Font().pFontLetterica16Russian->SetAligment(CGameFont::alCenter);
			UI().Font().pFontLetterica16Russian->SetColor(PICKUP_INFO_COLOR_FAREST);
			UI().Font().pFontLetterica16Russian->Out(x, y, draw_str);
		}


	}
}

void CActor::feel_sound_new(CObject* who, int type, CSound_UserDataPtr user_data, const Fvector& Position, float power)
{
	if(who == this)
		m_snd_noise = _max(m_snd_noise,power);
}

void CActor::Feel_Grenade_Update(float rad)
{
	// Find all nearest objects
	Fvector pos_actor;
	Center(pos_actor);

	q_nearest.clear_not_free();
	g_pGameLevel->ObjectSpace.GetNearest(q_nearest, pos_actor, rad, NULL);

	xr_vector<CObject*>::iterator	it_b = q_nearest.begin();
	xr_vector<CObject*>::iterator	it_e = q_nearest.end();

	// select only grenade
	for (; it_b != it_e; ++it_b)
	{
		if ((*it_b)->getDestroy()) continue;					// Don't touch candidates for destroy

		CGrenade* grn = smart_cast<CGrenade*>(*it_b);
		if (!grn || grn->Initiator() == ID() || grn->Useful())
		{
			continue;
		}
		if (grn->time_from_begin_throw() < m_fFeelGrenadeTime)
		{
			continue;
		}
		if (HUD().AddGrenade_ForMark(grn))
		{
			//.	Msg("__ __ Add new grenade! id = %d ", grn->ID() );
		}
	}// for it

	HUD().Update_GrenadeView(pos_actor);
}
