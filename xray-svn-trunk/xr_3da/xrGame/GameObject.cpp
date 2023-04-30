#include "pch_script.h"
#include "GameObject.h"
#include "../Include/xrRender/RenderVisual.h"
#include "../../xrphysics/PhysicsShell.h"
#include "ai_space.h"
#include "CustomMonster.h" 
#include "physicobject.h"
#include "level_graph.h"
#include "ph_shell_interface.h"
#include "script_game_object.h"
#include "xrserver_objects_alife.h"
#include "xrServer_Objects_ALife_Items.h"
#include "object_factory.h"
#include "../Include/xrRender/Kinematics.h"
#include "ai_object_location_impl.h"
#include "game_graph.h"
#include "ai_debug.h"
#include "../xr_3da/igame_level.h"
#include "level.h"
#include "script_callback_ex.h"
#include "game_level_cross_table.h"
#include "ai_obstacle.h"
#include "magic_box3.h"
#include "animation_movement_controller.h"
#include "../xr_3da/xr_collide_form.h"
extern MagicBox3 MagicMinBox (int iQuantity, const Fvector* akPoint);

#ifdef DEBUG
#	include "debug_renderer.h"
#	include "PHDebug.h"
#	include "../../xrPhysics/MathUtils.h"
#endif

CGameObject::CGameObject()
{
	m_ai_obstacle				= 0;

	init						();
	m_spawn_time				= 0;
	m_ai_location				= xr_new <CAI_ObjectLocation>();
	m_server_flags.one();

	m_callbacks					= xr_new <CALLBACK_MAP>();
	m_anim_mov_ctrl				= 0;
}

CGameObject::~CGameObject()
{
	VERIFY						(!animation_movement());
	VERIFY						(!m_ini_file);
	VERIFY						(!m_lua_game_object);
	VERIFY						(!m_spawned);

	xr_delete					(m_ai_location);
	xr_delete					(m_callbacks);
	xr_delete					(m_ai_obstacle);
}

void CGameObject::init()
{
	m_lua_game_object			= 0;
	m_script_clsid				= -1;
	m_ini_file					= 0;

	m_spawned					= false;
}

void CGameObject::LoadCfg(LPCSTR section)
{
	inherited::LoadCfg(section);
	ISpatial* self = smart_cast<ISpatial*> (this);

	if (self)
	{
		//self->spatial.s_type	|=	STYPE_VISIBLEFORAI;	
		self->spatial.s_type &= ~STYPE_REACTTOSOUND;
	}
}

void CGameObject::reinit()
{
	m_visual_callback.clear();

	ai_location().reinit();

	// clear callbacks	
	for (CALLBACK_MAP_IT it = m_callbacks->begin(); it != m_callbacks->end(); ++it) it->second.clear();
	m_callbacks->erase(m_callbacks->begin(), m_callbacks->end());
}

void CGameObject::reload(LPCSTR section)
{
	m_script_clsid = object_factory().script_clsid(CLS_ID);
}

void CGameObject::DestroyClientObj()
{
#ifdef DEBUG
	if (psAI_Flags.test(aiDestroy))
		Msg					("Destroying client object [%d][%s][%x]",ID(),*ObjectName(),this);
#endif

	VERIFY(m_spawned);
	if( m_anim_mov_ctrl )
					destroy_anim_mov_ctrl	();

	if (animation_movement_controlled())
		destroy_anim_mov_ctrl();

	if (m_ini_file)
		xr_delete(m_ini_file);

	m_script_clsid = -1;

	if (Visual() && smart_cast<IKinematics*>(Visual()))
		smart_cast<IKinematics*>(Visual())->Callback(0, 0);

	inherited::DestroyClientObj();

	setReady(FALSE);

	g_pGameLevel->Objects.net_Unregister(this);

	if (this == Level().CurrentEntity())
	{
		Level().SetEntity(0);
		Level().SetControlEntity(0);
	}

	CScriptBinder::DestroyClientObj();

	xr_delete(m_lua_game_object);

	m_spawned = false;
}

void CGameObject::OnEvent(NET_Packet& P, u16 type)
{
	switch (type)
	{
	case GE_HIT:
		{
			SHit HDS;
			HDS.PACKET_TYPE = type;
			HDS.Read_Packet_Cont(P);

			CObject* Hitter = Level().Objects.net_Find(HDS.whoID);
			CObject* Weapon = Level().Objects.net_Find(HDS.weaponID);
			HDS.who = Hitter;

			SetHitInfo(Hitter, Weapon, HDS.bone(), HDS.p_in_bone_space, HDS.dir);

			Hit(&HDS);

		}
		break;
	case GE_DESTROY:
		{
			if(H_Parent())
			{
				Msg("GE_DESTROY arrived, but H_Parent() exist. object[%d][%s] parent[%d][%s] [%d]", 
					ID(), ObjectName().c_str(),
					H_Parent()->ID(), H_Parent()->ObjectName().c_str(),
					CurrentFrame());
			}

			setDestroy		(TRUE);
		}

		break;
	}
}

void VisualCallback(IKinematics *tpKinematics);

BOOL CGameObject::SpawnAndImportSOData(CSE_Abstract* data_containing_so)
{
	VERIFY(!m_spawned);

	m_spawned = true;

	m_spawn_time = CurrentFrame();

	CSE_Abstract* E = (CSE_Abstract*)data_containing_so;

	VERIFY(E);

	const CSE_Visual* visual = smart_cast<const CSE_Visual*>(E);
	m_ai_obstacle					= xr_new<ai_obstacle>(this);

	if (visual)
	{
		SetVisualName(visual_name(E));

		if (visual->flags.test(CSE_Visual::flObstacle))
		{
			ISpatial* self = smart_cast<ISpatial*>(this);
			self->spatial.s_type |= STYPE_OBSTACLE;
	m_ai_obstacle					= xr_new<ai_obstacle>(this);
		}
	}

	// Naming
	SetObjectName						(E->s_name);
	SetSectionName					(E->s_name);

	if (E->name_replace()[0])
		SetObjectName(E->name_replace());

	setID(E->ID);
	
	// XForm
	XFORM().setXYZ					(E->o_Angle);
	Position().set					(E->o_Position);
#ifdef DEBUG
	if(ph_dbg_draw_mask1.test(ph_m1_DbgTrackObject)&&stricmp(PH_DBG_ObjectTrackName(), ObjectNameStr())==0)
	{
		Msg("CGameObject::SpawnAndImportSOData obj %s Position set from CSE_Abstract %f,%f,%f",PH_DBG_ObjectTrackName(),Position().x,Position().y,Position().z);
	}
#endif

	VERIFY(_valid(renderable.xform));
	VERIFY(!fis_zero(DET(renderable.xform)));

	CSE_ALifeObject* O = smart_cast<CSE_ALifeObject*>(E);

	if (O && xr_strlen(O->m_ini_string))
	{
#pragma warning(push)
#pragma warning(disable:4238)
		m_ini_file = xr_new <CInifile>
			(&IReader ((void*)(*(O->m_ini_string)), O->m_ini_string.size()), FS.get_path("$game_config$")->m_Path);
#pragma warning(pop)
	}

	m_story_id = ALife::_STORY_ID(-1);

	if (O)
		m_story_id = O->m_story_id;

	setReady(TRUE);

	g_pGameLevel->Objects.net_Register(this);

	m_server_flags.one();

	if (O)
	{
		m_server_flags = O->m_flags;

		if (O->m_flags.is(CSE_ALifeObject::flVisibleForAI))
			spatial.s_type |= STYPE_VISIBLEFORAI;
		else
			spatial.s_type = (spatial.s_type | STYPE_VISIBLEFORAI) ^ STYPE_VISIBLEFORAI;
	}

	reload(*SectionName());

	CScriptBinder::reload(*SectionName());
	
	reinit();

	CScriptBinder::reinit();

#ifdef DEBUG
	if(ph_dbg_draw_mask1.test(ph_m1_DbgTrackObject)&&stricmp(PH_DBG_ObjectTrackName(), ObjectNameStr())==0)
	{
		Msg("CGameObject::SpawnAndImportSOData obj %s After Script Binder reinit %f,%f,%f",PH_DBG_ObjectTrackName(),Position().x,Position().y,Position().z);
	}
#endif

	//load custom user data from server
	if(!E->client_data.empty())
	{	
		IReader	ireader = IReader(&*E->client_data.begin(), E->client_data.size());
		net_Load(ireader);
	}

	// if we have a parent
	if (ai().get_level_graph())
	{
		if (E->ID_Parent == 0xffff)
		{
			CSE_ALifeObject* l_tpALifeObject = smart_cast<CSE_ALifeObject*>(E);

			if (l_tpALifeObject && ai().level_graph().valid_vertex_id(l_tpALifeObject->m_tNodeID))
				ai_location().level_vertex(l_tpALifeObject->m_tNodeID);
			else
			{
				CSE_Temporary* l_tpTemporary = smart_cast<CSE_Temporary*>(E);

				if (l_tpTemporary && ai().level_graph().valid_vertex_id(l_tpTemporary->m_tNodeID))
					ai_location().level_vertex	(l_tpTemporary->m_tNodeID);
			}

			if (l_tpALifeObject && ai().game_graph().valid_vertex_id(l_tpALifeObject->m_tGraphID))
				ai_location().game_vertex(l_tpALifeObject->m_tGraphID);

			validate_ai_locations(false);

			// validating position
			if	(UsedAI_Locations() && ai().level_graph().inside(ai_location().level_vertex_id(), Position()) && can_validate_position_on_spawn())
				Position().y = EPS_L + ai().level_graph().vertex_plane_y(*ai_location().level_vertex(),Position().x,Position().z);
		}
		else
		{
			CSE_ALifeObject* const alife_object	= smart_cast<CSE_ALifeObject*>(E);

			if (alife_object && ai().level_graph().valid_vertex_id(alife_object->m_tNodeID))
			{
				ai_location().level_vertex(alife_object->m_tNodeID);
				ai_location().game_vertex(alife_object->m_tGraphID);
			}
		}
	}

	inherited::SpawnAndImportSOData(data_containing_so);

	m_bObjectRemoved = false;

	spawn_supplies();
#ifdef DEBUG
	if(ph_dbg_draw_mask1.test(ph_m1_DbgTrackObject)&&stricmp(PH_DBG_ObjectTrackName(), ObjectNameStr())==0)
	{
		Msg("CGameObject::SpawnAndImportSOData obj %s Before CScriptBinder::SpawnAndImportSOData %f,%f,%f",PH_DBG_ObjectTrackName(),Position().x,Position().y,Position().z);
	}

	BOOL ret = CScriptBinder::SpawnAndImportSOData(data_containing_so);

	return ret;
#else
	return (CScriptBinder::SpawnAndImportSOData(data_containing_so));
#endif
}

void CGameObject::net_Save(NET_Packet& net_packet)
{
	u32	position;
	net_packet.w_chunk_open16(position);

	save(net_packet);

#ifdef DEBUG	
	if (psAI_Flags.test(aiSerialize))
	{
		Msg(">> **** Save script object [%s] *****", *ObjectName());
		Msg(">> Before save :: packet position = [%u]", net_packet.w_tell());
	}
#endif

	CScriptBinder::save(net_packet);

#ifdef DEBUG	

	if (psAI_Flags.test(aiSerialize))
	{
		Msg(">> After save :: packet position = [%u]", net_packet.w_tell());
	}
#endif

	net_packet.w_chunk_close16	(position);
}

void CGameObject::net_Load(IReader& ireader)
{
	load(ireader);

#ifdef DEBUG	
	if (psAI_Flags.test(aiSerialize))	{
		Msg(">> **** Load script object [%s] *****", *ObjectName());
		Msg(">> Before load :: reader position = [%i]", ireader.tell());
	}

#endif

	CScriptBinder::load		(ireader);

#ifdef DEBUG	

	if (psAI_Flags.test(aiSerialize))	{
		Msg(">> After load :: reader position = [%i]", ireader.tell());
	}
#endif

#ifdef DEBUG
	if(ph_dbg_draw_mask1.test(ph_m1_DbgTrackObject)&&stricmp(PH_DBG_ObjectTrackName(),*ObjectName())==0)
	{
		Msg("CGameObject::net_Load obj %s (loaded) %f,%f,%f", PH_DBG_ObjectTrackName(),Position().x,Position().y,Position().z);
	}
#endif

}

void CGameObject::save(NET_Packet &output_packet) 
{
}

void CGameObject::load(IReader &input_packet)
{
}

void CGameObject::spawn_supplies()
{
	if (!spawn_ini() || ai().get_alife())
		return;

	if (!spawn_ini()->section_exist("spawn"))
		return;

	LPCSTR					N, V;

	float					p;

	bool bScope				=	false;
	bool bSilencer			=	false;
	bool bLauncher			=	false;

	for (u32 k = 0, j; spawn_ini()->r_line("spawn",k,&N,&V); k++)
	{
		VERIFY(xr_strlen(N));
		j = 1;
		p = 1.f;
		
		float f_cond = 1.0f;

		if (V && xr_strlen(V))
		{
			int n = _GetItemCount(V);
			string16 temp;

			if (n > 0)
				j = atoi(_GetItem(V,0,temp)); //count
			
			if(NULL != strstr(V, "prob="))
				p =(float)atof(strstr(V, "prob=") + 5);

			if (fis_zero(p))
				p = 1.f;

			if (!j)
				j = 1;

			if(NULL != strstr(V, "cond="))
				f_cond = (float)atof(strstr(V, "cond=") + 5);

			bScope = (NULL!=strstr(V,"scope"));
			bSilencer = (NULL!=strstr(V,"silencer"));
			bLauncher = (NULL!=strstr(V,"launcher"));

		}

		for (u32 i = 0; i < j; ++i)

			if (::Random.randF(1.f) < p)
			{
				CSE_Abstract* A = Level().spawn_item(N, Position(), ai_location().level_vertex_id(), ID(), true);

				CSE_ALifeInventoryItem*	pSE_InventoryItem = smart_cast<CSE_ALifeInventoryItem*>(A);

				if(pSE_InventoryItem)
						pSE_InventoryItem->m_fCondition = f_cond;

				CSE_ALifeItemWeapon* W =  smart_cast<CSE_ALifeItemWeapon*>(A);
				if (W) {
					if (W->m_scope_status			== ALife::eAddonAttachable)
						W->m_addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonScope, bScope);
					if (W->m_silencer_status		== ALife::eAddonAttachable)
						W->m_addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonSilencer, bSilencer);
					if (W->m_grenade_launcher_status == ALife::eAddonAttachable)
						W->m_addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher, bLauncher);
				}

				NET_Packet					P;
				A->Spawn_Write				(P,TRUE);
				Level().Send				(P,net_flags(TRUE));
				F_entity_Destroy			(A);
		}
	}
}

void CGameObject::setup_parent_ai_locations(bool assign_position)
{
	VERIFY(H_Parent());

	CGameObject* l_tpGameObject = static_cast<CGameObject*>(H_Parent());

	VERIFY(l_tpGameObject);

	// get parent's position
	if (assign_position && use_parent_ai_locations())
		Position().set(l_tpGameObject->Position());

	// setup its ai locations
	if (!UsedAI_Locations())
		return;

	if (!ai().get_level_graph())
		return;

	if (l_tpGameObject->UsedAI_Locations() && ai().level_graph().valid_vertex_id(l_tpGameObject->ai_location().level_vertex_id()))
		ai_location().level_vertex(l_tpGameObject->ai_location().level_vertex_id());
	else
		validate_ai_locations(false);

	if (ai().game_graph().valid_vertex_id(l_tpGameObject->ai_location().game_vertex_id()))
		ai_location().game_vertex(l_tpGameObject->ai_location().game_vertex_id());
	else
		ai_location().game_vertex(ai().cross_table().vertex(ai_location().level_vertex_id()).game_vertex_id());
}

u32 CGameObject::new_level_vertex_id			() const
{
	Fvector							center;
	Center							(center);
	center.x						= Position().x;
	center.z						= Position().z;
	return							(ai().level_graph().vertex(ai_location().level_vertex_id(),center));
}

void CGameObject::update_ai_locations			(bool decrement_reference)
{
	u32								l_dwNewLevelVertexID = new_level_vertex_id();
	VERIFY							(ai().level_graph().valid_vertex_id(l_dwNewLevelVertexID));
	if (decrement_reference && (ai_location().level_vertex_id() == l_dwNewLevelVertexID))
		return;

	ai_location().level_vertex		(l_dwNewLevelVertexID);

	if (!ai().get_game_graph() && ai().get_cross_table())
		return;

	ai_location().game_vertex		(ai().cross_table().vertex(ai_location().level_vertex_id()).game_vertex_id());
	VERIFY							(ai().game_graph().valid_vertex_id(ai_location().game_vertex_id()));
}

void CGameObject::validate_ai_locations			(bool decrement_reference)
{
	if (!ai().get_level_graph())
		return;

	if (!UsedAI_Locations())
		return;

	update_ai_locations				(decrement_reference);
}

void CGameObject::spatial_move_intern()
{
	if (H_Parent())
		setup_parent_ai_locations();
	else
		if (Visual())
			validate_ai_locations();

	inherited::spatial_move_intern();
}

#ifdef DEBUG
void CGameObject::dbg_DrawSkeleton()
{
	CCF_Skeleton* Skeleton = smart_cast<CCF_Skeleton*>(collidable.model);

	if (!Skeleton)
		return;

	Skeleton->_dbg_refresh();

	const CCF_Skeleton::ElementVec& Elements = Skeleton->_GetElements();

	for (CCF_Skeleton::ElementVec::const_iterator I=Elements.begin(); I!=Elements.end(); I++)
	{
		if (!I->valid())
			continue;

		switch (I->type){
			case SBoneShape::stBox:
			{
				Fmatrix M;
				M.invert(I->b_IM);
				Fvector h_size = I->b_hsize;

				Level().debug_renderer().draw_obb(M, h_size, color_rgba(0, 255, 0, 255));
			}break;
			case SBoneShape::stCylinder:
			{
				Fmatrix M;
				M.c.set				(I->c_cylinder.m_center);
				M.k.set				(I->c_cylinder.m_direction);
				Fvector				h_size;
				h_size.set			(I->c_cylinder.m_radius, I->c_cylinder.m_radius, I->c_cylinder.m_height * 0.5f);

				Fvector::generate_orthonormal_basis(M.k, M.j, M.i);

				Level().debug_renderer().draw_obb	(M, h_size, color_rgba(0, 127, 255, 255));
			}break;
			case SBoneShape::stSphere:
			{
				Fmatrix				l_ball;
				l_ball.scale		(I->s_sphere.R, I->s_sphere.R, I->s_sphere.R);
				l_ball.translate_add(I->s_sphere.P);

				Level().debug_renderer().draw_ellipse(l_ball, color_rgba(0, 255, 0, 255));
			}break;
		};
	};	
}
#endif

void CGameObject::renderable_Render(IRenderBuffer& render_buffer)
{
	inherited::renderable_Render(render_buffer);

	render_buffer.renderingMatrix_ = XFORM();
	::Render->add_Visual(Visual(), render_buffer);
}

CObject::SavedPosition CGameObject::ps_Element(u32 ID) const
{
	VERIFY(ID < ps_Size());

	inherited::SavedPosition SP = PositionStack[ID];
	SP.dwTime += Level().timeServer_Delta();

	return SP;
}

void CGameObject::u_EventGen(NET_Packet& P, u32 type, u32 dest)
{
	P.w_begin	(M_EVENT);
	P.w_u32		(Level().timeServer());
	P.w_u16		(u16(type&0xffff));
	P.w_u16		(u16(dest&0xffff));
}

void CGameObject::u_EventSend(NET_Packet& P, u32 dwFlags)
{
	Level().Send(P, dwFlags);
}

#include "bolt.h"
void CGameObject::BeforeAttachToParent()
{
	inherited::BeforeAttachToParent();
}

void CGameObject::BeforeDetachFromParent(bool just_before_destroy)
{
	inherited::BeforeDetachFromParent(just_before_destroy);

	setup_parent_ai_locations(false);

	CGameObject* parent = smart_cast<CGameObject*>(H_Parent());

	VERIFY(parent);

	if (ai().get_level_graph() && ai().level_graph().valid_vertex_id(parent->ai_location().level_vertex_id()))
		validate_ai_locations(false);
}


#ifdef DEBUG
void CGameObject::OnRender()
{
	if (Render->currentViewPort != MAIN_VIEWPORT)
		return;
	
	if (bDebug && Visual())
	{
		Fvector bc, bd; 
		Visual()->getVisData().box.get_CD(bc, bd);

		Fmatrix	M = XFORM();
		M.c.add (bc);

		Level().debug_renderer().draw_obb(M, bd, color_rgba(0, 0, 255, 255));
	}	
}
#endif

BOOL CGameObject::UsedAI_Locations()
{
	return(m_server_flags.test(CSE_ALifeObject::flUsedAI_Locations));
}

BOOL CGameObject::TestServerFlag(u32 Flag) const
{
	return(m_server_flags.test(Flag));
}

void CGameObject::add_visual_callback(visual_callback* callback)
{
	VERIFY(smart_cast<IKinematics*>(Visual()));

	CALLBACK_VECTOR_IT I = std::find(visual_callbacks().begin(), visual_callbacks().end(), callback);

	VERIFY(I == visual_callbacks().end());

	if (m_visual_callback.empty())
		SetKinematicsCallback(true);

	m_visual_callback.push_back	(callback);
}

void CGameObject::remove_visual_callback(visual_callback* callback)
{
	CALLBACK_VECTOR_IT I = std::find(m_visual_callback.begin(), m_visual_callback.end(), callback);

	VERIFY(I != m_visual_callback.end());
	m_visual_callback.erase(I);

	if (m_visual_callback.empty())
		SetKinematicsCallback(false);
}

void CGameObject::SetKinematicsCallback(bool set)
{
	if(!Visual())
		return;

	if (set)
		smart_cast<IKinematics*>(Visual())->Callback(VisualCallback, this);
	else
		smart_cast<IKinematics*>(Visual())->Callback(0, 0);
};

void VisualCallback	(IKinematics* tpKinematics)
{
	CGameObject* game_object = static_cast<CGameObject*>(static_cast<CObject*>(tpKinematics->GetUpdateCallbackParam()));

	VERIFY(game_object);
	
	CGameObject::CALLBACK_VECTOR_IT	I = game_object->visual_callbacks().begin();
	CGameObject::CALLBACK_VECTOR_IT	E = game_object->visual_callbacks().end();

	for ( ; I != E; ++I)
		(*I)(tpKinematics);
}

CScriptGameObject *CGameObject::lua_game_object() const
{
	if (!m_spawned)
		Msg	("! You are trying to use a destroyed object (or binder object) [%d][%s][%x]",ID(),*ObjectName(),this);

	THROW(m_spawned);

	if (!m_lua_game_object)
		m_lua_game_object = xr_new <CScriptGameObject>(const_cast<CGameObject*>(this));

	return(m_lua_game_object);
}

void CGameObject::DestroyObject()			
{
	if(m_bObjectRemoved)
		return;

	m_bObjectRemoved = true;

	if (getDestroy())
		return;

	NET_Packet		P;
	u_EventGen(P, GE_DESTROY, ID());
	u_EventSend(P);
}

void CGameObject::ScheduledUpdate(u32 dt)
{
#ifdef MEASURE_UPDATES
	CTimer measure_sc_update; measure_sc_update.Start();
#endif

	inherited::ScheduledUpdate(dt);
	
	CScriptBinder::ScheduledUpdate(dt);


#ifdef MEASURE_UPDATES
	Device.Statistic->scheduler_GameObject_ += measure_sc_update.GetElapsed_sec();
#endif
}

BOOL CGameObject::net_SaveRelevant	()
{
	return	(CScriptBinder::net_SaveRelevant());
}

//игровое имя объекта
LPCSTR CGameObject::Name()const
{
	return(*ObjectName());
}

u32	CGameObject::ef_creature_type() const
{
	string16	temp; CLSID2TEXT(CLS_ID, temp);
	R_ASSERT3(false, "Invalid creature type request, virtual function is not properly overridden!", temp);
	return		(u32(-1));
}

u32	CGameObject::ef_equipment_type() const
{
	string16	temp; CLSID2TEXT(CLS_ID, temp);
	R_ASSERT3(false, "Invalid equipment type request, virtual function is not properly overridden!", temp);
	return		(u32(-1));
	//	return		(6);
}

u32	CGameObject::ef_main_weapon_type() const
{
	string16	temp; CLSID2TEXT(CLS_ID, temp);
	R_ASSERT3(false, "Invalid main weapon type request, virtual function is not properly overridden!", temp);
	return		(u32(-1));
	//	return		(5);
}

u32	CGameObject::ef_anomaly_type() const
{
	string16	temp; CLSID2TEXT(CLS_ID, temp);
	R_ASSERT3(false, "Invalid anomaly type request, virtual function is not properly overridden!", temp);
	return		(u32(-1));
}

u32	CGameObject::ef_weapon_type() const
{
	string16	temp; CLSID2TEXT(CLS_ID, temp);
	R_ASSERT3(false, "Invalid weapon type request, virtual function is not properly overridden!", temp);
	return		(u32(-1));
	//	return		(u32(0));
}

u32	CGameObject::ef_detector_type() const
{
	string16	temp; CLSID2TEXT(CLS_ID, temp);
	R_ASSERT3(false, "Invalid detector type request, virtual function is not properly overridden!", temp);
	return		(u32(-1));
}

void CGameObject::RemoveLinksToCLObj(CObject* O)
{
	inherited::RemoveLinksToCLObj(O);

	CScriptBinder::RemoveLinksToCLObj(O);
}

CGameObject::CScriptCallbackExVoid &CGameObject::callback(GameObject::ECallbackType type) const
{
	return ((*m_callbacks)[type]);
}

LPCSTR CGameObject::visual_name(CSE_Abstract* server_entity)
{
	const CSE_Visual* visual = smart_cast<const CSE_Visual*>(server_entity);

	VERIFY(visual);

	return(visual->get_visual());
}

void CGameObject::UpdateCL()
{
#ifdef MEASURE_UPDATES
	CTimer measure_updatecl; measure_updatecl.Start();
#endif


	inherited::UpdateCL();

	
#ifdef MEASURE_UPDATES
	Device.Statistic->updateCL_GameObject_ += measure_updatecl.GetElapsed_sec();
#endif
}

void CGameObject::OnChangeVisual()
{
	inherited::OnChangeVisual();
	if( animation_movement_controlled())
	{
		destroy_anim_mov_ctrl();
	}
}

bool CGameObject::shedule_Needed()
{
	return (!getDestroy());
}
bool CGameObject::animation_movement_controlled() const
{
	return	!!animation_movement() && animation_movement()->IsActive();
}

void CGameObject::update_animation_movement_controller()
{
	if (!m_anim_mov_ctrl)
		return;

	if (m_anim_mov_ctrl->IsActive())
	{
		m_anim_mov_ctrl->OnFrame();
		return;
	}

	destroy_anim_mov_ctrl();
}

bool CGameObject::is_ai_obstacle() const
{
	return (false);
}

void CGameObject::create_anim_mov_ctrl(CBlend *b, Fmatrix *start_pose, bool local_animation)
{
	if (animation_movement_controlled())
	{
		m_anim_mov_ctrl->NewBlend(
			b,
			start_pose ? *start_pose : XFORM(),
			local_animation
		);
	}
	else
	{
		//		start_pose		= &renderable.xform;
		if (m_anim_mov_ctrl)
			destroy_anim_mov_ctrl();

		VERIFY2(
			start_pose,
			make_string(
				"start pose hasn't been specified for animation [%s][%s]",
				smart_cast<IKinematicsAnimated&>(*Visual()).LL_MotionDefName_dbg(b->motionID).first,
				smart_cast<IKinematicsAnimated&>(*Visual()).LL_MotionDefName_dbg(b->motionID).second
			)
		);

		VERIFY2(
			!animation_movement(),
			make_string(
				"start pose hasn't been specified for animation [%s][%s]",
				smart_cast<IKinematicsAnimated&>(*Visual()).LL_MotionDefName_dbg(b->motionID).first,
				smart_cast<IKinematicsAnimated&>(*Visual()).LL_MotionDefName_dbg(b->motionID).second
			)
		);

		VERIFY(Visual());
		IKinematics		*K = Visual()->dcast_PKinematics();
		VERIFY(K);

		m_anim_mov_ctrl = xr_new<animation_movement_controller>(&XFORM(), *start_pose, K, b);
	}
}

void CGameObject::destroy_anim_mov_ctrl()
{
	xr_delete(m_anim_mov_ctrl);
}

IC	bool similar(const Fmatrix &_0, const Fmatrix &_1, const float &epsilon = EPS)
{
	if (!_0.i.similar(_1.i, epsilon))
		return						(false);

	if (!_0.j.similar(_1.j, epsilon))
		return						(false);

	if (!_0.k.similar(_1.k, epsilon))
		return						(false);

	if (!_0.c.similar(_1.c, epsilon))
		return						(false);

	// note: we do not compare projection here
	return							(true);
}

void CGameObject::on_matrix_change(const Fmatrix &previous)
{
	obstacle().on_move();
}
