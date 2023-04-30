#include "pch_script.h"
#include "xrServer_Objects_ALife_All.h"
#include "level.h"
#include "ai_space.h"
#include "game_level_cross_table.h"
#include "level_graph.h"
#include "client_spawn_manager.h"
#include "../xr_object.h"
#include "GameObject.h"

void CLevel::cl_Process_Spawn(NET_Packet& P)
{
	// Begin analysis
	shared_str			s_name;
	P.r_stringZ			(s_name);

	// Create DC (xrSE)
	CSE_Abstract*	E	= F_entity_Create	(*s_name);
	R_ASSERT2(E, *s_name);

	E->Spawn_Read		(P);
	if (E->s_flags.is(M_SPAWN_UPDATE))
		E->UPDATE_Read	(P);
//-------------------------------------------------
//	Msg ("M_SPAWN - %s[%d][%x] - %d", *s_name,  E->ID, E,E->ID_Parent);
//-------------------------------------------------
	//force object to be local for server client

	E->s_flags.set(M_SPAWN_OBJECT_LOCAL, TRUE);

	g_sv_Spawn					(E);

	F_entity_Destroy			(E);
};

void CLevel::g_cl_Spawn		(LPCSTR name, u8 rp, u16 flags, Fvector pos)
{
	// Create
	CSE_Abstract*		E	= F_entity_Create(name);
	R_ASSERT			(E);

	// Fill
	E->s_name			= name;
	E->set_name_replace	("");
	E->s_RP				=	rp;
	E->ID				=	0xffff;
	E->ID_Parent		=	0xffff;
	E->ID_Phantom		=	0xffff;
	E->s_flags.assign	(flags);
	E->RespawnTime		=	0;
	E->o_Position		= pos;

	// Send
	NET_Packet			P;
	E->Spawn_Write		(P,TRUE);
	Send				(P,net_flags(TRUE));

	// Destroy
	F_entity_Destroy	(E);
}

void CLevel::g_sv_Spawn		(CSE_Abstract* E)
{
#ifdef DEBUG_MEMORY_MANAGER
	u32							E_mem = 0;
	if (g_bMEMO)	{
		lua_gc					(ai().script_engine().lua(),LUA_GCCOLLECT,0);
		lua_gc					(ai().script_engine().lua(),LUA_GCCOLLECT,0);
		E_mem					= Memory.mem_usage();	
		Memory.stat_calls		= 0;
	}
#endif 

	// Optimization for single-player only	- minimize traffic between client and server
	psNET_Flags.set	(NETFLAG_MINIMIZEUPDATES,TRUE);


	// Client spawn
	CObject*	O		= Objects.Create	(*E->s_name);

#ifdef DEBUG_MEMORY_MANAGER
	mem_alloc_gather_stats		(false);
#endif

	if (0==O || (!O->SpawnAndImportSOData(E))) 
	{
		O->DestroyClientObj( );

		client_spawn_manager().clear(O->ID());

		Objects.Destroy			(O, true);
		Msg						("! Failed to spawn entity '%s'",*E->s_name);

#ifdef DEBUG_MEMORY_MANAGER
		mem_alloc_gather_stats	(!!psAI_Flags.test(aiDebugOnFrameAllocs));
#endif

	} else {

#ifdef DEBUG_MEMORY_MANAGER
		mem_alloc_gather_stats	(!!psAI_Flags.test(aiDebugOnFrameAllocs));
#endif

		client_spawn_manager().callback(O);
		//Msg			("--spawn--SPAWN: %f ms",1000.f*T.GetAsync());
		if ((E->s_flags.is(M_SPAWN_OBJECT_LOCAL)) && (E->s_flags.is(M_SPAWN_OBJECT_ASPLAYER)))	{
			if (CurrentEntity() != NULL) 
			{
				CGameObject* pGO = smart_cast<CGameObject*>(CurrentEntity());
				if (pGO) pGO->On_B_NotCurrentEntity();
			}
			SetEntity			(O);
			SetControlEntity	(O);
		}

		if (0xffff != E->ID_Parent)	
		{
			NET_Packet	GEN;
			GEN.write_start();
			GEN.read_start();
			GEN.w_u16			(u16(O->ID()));
			GEN.w_u8			(1);
			cl_Process_Event(E->ID_Parent, GE_OWNERSHIP_TAKE, GEN);
		}
	}
#ifdef DEBUG_MEMORY_MANAGER
	if (g_bMEMO) {
		lua_gc					(ai().script_engine().lua(),LUA_GCCOLLECT,0);
		lua_gc					(ai().script_engine().lua(),LUA_GCCOLLECT,0);
		Msg						("* %20s : %d bytes, %d ops", *E->s_name,Memory.mem_usage()-E_mem, Memory.stat_calls );
	}
#endif
}

CSE_Abstract *CLevel::spawn_item		(LPCSTR section, const Fvector &position, u32 level_vertex_id, u16 parent_id, bool return_item)
{
	CSE_Abstract			*abstract = F_entity_Create(section);
	R_ASSERT3				(abstract,"Cannot find item with section",section);
	CSE_ALifeDynamicObject	*dynamic_object = smart_cast<CSE_ALifeDynamicObject*>(abstract);
	if (dynamic_object && ai().get_level_graph()) {
		dynamic_object->m_tNodeID	= level_vertex_id;
		if (ai().level_graph().valid_vertex_id(level_vertex_id) && ai().get_game_graph() && ai().get_cross_table())
			dynamic_object->m_tGraphID	= ai().cross_table().vertex(level_vertex_id).game_vertex_id();
	}

	//������ ������� � ������ ���������
	CSE_ALifeItemWeapon* weapon = smart_cast<CSE_ALifeItemWeapon*>(abstract);
	if(weapon)
		weapon->a_elapsed	= weapon->get_ammo_magsize();
	
	// Fill
	abstract->s_name		= section;
	abstract->set_name_replace	(section);
	abstract->o_Position	= position;
	abstract->s_RP			= 0xff;
	abstract->ID			= 0xffff;
	abstract->ID_Parent		= parent_id;
	abstract->ID_Phantom	= 0xffff;
	abstract->s_flags.assign(M_SPAWN_OBJECT_LOCAL);
	abstract->RespawnTime	= 0;

	if (!return_item) {
		NET_Packet				P;
		abstract->Spawn_Write	(P,TRUE);
		Send					(P,net_flags(TRUE));
		F_entity_Destroy		(abstract);
		return					(0);
	}
	else
		return				(abstract);
}

void	CLevel::ProcessGameSpawns	()
{
	while (!game_spawn_queue.empty())
	{
		CSE_Abstract*	E			= game_spawn_queue.front();

		g_sv_Spawn					(E);

		F_entity_Destroy			(E);

		game_spawn_queue.pop_front	();
	}
}
