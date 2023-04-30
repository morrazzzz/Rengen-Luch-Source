////////////////////////////////////////////////////////////////////////////
//	Module 		: script_zone.cpp
//	Created 	: 10.10.2003
//  Modified 	: 10.10.2003
//	Author		: Dmitriy Iassenev
//	Description : Script zone object
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_zone.h"
#include "script_game_object.h"
#include "xrserver_objects_alife_monsters.h"
#include "../xr_collide_form.h"
#include "script_callback_ex.h"
#include "game_object_space.h"

#ifdef DEBUG
#	include "level.h"
#	include "debug_renderer.h"
#endif

CScriptZone::CScriptZone		()
{
}

CScriptZone::~CScriptZone		()
{
}

void CScriptZone::reinit		()
{
	inherited::reinit		();
}

BOOL CScriptZone::SpawnAndImportSOData(CSE_Abstract* data_containing_so) 
{
	feel_touch.clear			();

	if (!inherited::SpawnAndImportSOData(data_containing_so))
		return					(FALSE);

	return						(TRUE);
}

void CScriptZone::DestroyClientObj()
{
	inherited::DestroyClientObj();
}

void CScriptZone::ScheduledUpdate(u32 dt)
{
#ifdef MEASURE_UPDATES
	CTimer measure_sc_update; measure_sc_update.Start();
#endif


	inherited::ScheduledUpdate	(dt);

	const Fsphere& s = CFORM()->getSphere();

	Fvector P;

	XFORM().transform_tiny(P, s.P);

	feel_touch_update(P, s.R);


#ifdef MEASURE_UPDATES
	Device.Statistic->scheduler_ScriptZone_ += measure_sc_update.GetElapsed_sec();
#endif
}

void CScriptZone::feel_touch_new	(CObject *tpObject)
{
	CGameObject					*l_tpGameObject = smart_cast<CGameObject*>(tpObject);
	if (!l_tpGameObject)
		return;
	
	callback(GameObject::eZoneEnter)(lua_game_object(),l_tpGameObject->lua_game_object());
}

void CScriptZone::feel_touch_delete	(CObject *tpObject)
{
	CGameObject					*l_tpGameObject = smart_cast<CGameObject*>(tpObject);
	
	if (!l_tpGameObject || l_tpGameObject->getDestroy())
		return;

	callback(GameObject::eZoneExit)(lua_game_object(),l_tpGameObject->lua_game_object());
}

void CScriptZone::RemoveLinksToCLObj(CObject *O)
{
	CGameObject					*l_tpGameObject = smart_cast<CGameObject*>(O);
	if (!l_tpGameObject)
		return;

	xr_vector<CObject*>::iterator	I = std::find(feel_touch.begin(),feel_touch.end(),O);
	if (I != feel_touch.end()) {
		callback(GameObject::eZoneExit)(lua_game_object(),l_tpGameObject->lua_game_object());
	}
}

BOOL CScriptZone::feel_touch_contact	(CObject* O)
{
	return						(((CCF_Shape*)CFORM())->Contact(O));
}

#ifdef DEBUG
void CScriptZone::OnRender() 
{
	if (Render->currentViewPort != MAIN_VIEWPORT)
		return;
	
	if(!bDebug) return;
	DRender->OnFrameEnd();
	Fvector l_half; l_half.set(.5f, .5f, .5f);
	Fmatrix l_ball, l_box;
	xr_vector<CCF_Shape::shape_def> &l_shapes = ((CCF_Shape*)CFORM())->Shapes();
	xr_vector<CCF_Shape::shape_def>::iterator l_pShape;
	
	for(l_pShape = l_shapes.begin(); l_shapes.end() != l_pShape; ++l_pShape) 
	{
		switch(l_pShape->type)
		{
		case 0:
			{
                Fsphere &l_sphere = l_pShape->data.sphere;
				l_ball.scale(l_sphere.R, l_sphere.R, l_sphere.R);
				Fvector l_p; XFORM().transform(l_p, l_sphere.P);
				l_ball.translate_add(l_p);
				Level().debug_renderer().draw_ellipse(l_ball, D3DCOLOR_XRGB(0,255,255));
			}
			break;
		case 1:
			{
				l_box.mul(XFORM(), l_pShape->data.box);
				Level().debug_renderer().draw_obb(l_box, l_half, D3DCOLOR_XRGB(0,255,255));
			}
			break;
		}
	}
}
#endif

bool CScriptZone::active_contact(u16 id) const
{
	xr_vector<CObject*>::const_iterator	I = feel_touch.begin();
	xr_vector<CObject*>::const_iterator	E = feel_touch.end();
	for ( ; I != E; ++I)
		if ((*I)->ID() == id)
			return						(true);
	return								(false);
}
