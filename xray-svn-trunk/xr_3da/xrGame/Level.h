// Level.h: implementation for the IGame_Level interface.

#pragma once

#include "../IGame_Level.h"
#include "../../xrNetServer/net_client.h"
#include "script_export_space.h"
#include "../StatGraph.h"
#include "alife_space.h"
#include "xrDebug.h"
#include "xrServer.h"
#include "../feel_touch.h"

class	CHUDManager;
class	CParticlesObject;
class	xrServer;
class	game_cl_GameState;
class	NET_Queue_Event;
class	CSE_Abstract;
class	CSpaceRestrictionManager;
class	CSeniorityHierarchyHolder;
class	CClientSpawnManager;
class	CGameObject;
class	CAutosaveManager;
class	CPHCommander;
class	CLevelDebug;
class	CLevelSoundManager;
class	CZoneList;
class	CGameTaskManager;

class CBulletManager;
class CMapManager;

#ifdef DRENDER
class	CDebugRenderer;
#endif

extern ENGINE_API float camFov;

class GlobalFeelTouch : public Feel::Touch
{
public:
					GlobalFeelTouch();
	virtual			~GlobalFeelTouch();

	void			update();
	bool			is_object_denied			(CObject const * O);
};

class CLevel					: public IGame_Level, public IPureClient
{
	void						ClearAllObjects			();
protected:
	typedef IGame_Level			inherited;
	
	CLevelSoundManager			*m_level_sound_manager;

	// movement restriction manager
	CSpaceRestrictionManager	*m_space_restriction_manager;
	// seniority hierarchy holder
	CSeniorityHierarchyHolder	*m_seniority_hierarchy_holder;
	// client spawn_manager
	CClientSpawnManager			*m_client_spawn_manager;
	// autosave manager
	CAutosaveManager			*m_autosave_manager;

#ifdef DRENDER
	// debug renderer
	CDebugRenderer				*m_debug_renderer;
#endif

	CPHCommander				*m_ph_commander;
	CPHCommander				*m_ph_commander_scripts;
	// level name
	shared_str					m_name;
	CPHCommander				*m_ph_commander_physics_worldstep;
	// Local events
	EVENT						eChangeTrack;
	EVENT						eEnvironment;
	EVENT						eEntitySpawn;

	u32							m_dwRPC;	//ReceivedPacketsCount
	u32							m_dwRPS;	//ReceivedPacketsSize
	//---------------------------------------------
	
	// ambient particles
	CParticlesObject*			ambient_particles;
	u32							ambient_sound_next_time;
	u32							ambient_effect_next_time;
	u32							ambient_effect_stop_time;

public:
#ifdef DEBUG
	// level debugger
	CLevelDebug					*m_level_debug;
#endif

public:
	virtual void				OnMessage				(void* data, u32 size);
private:

	DEF_VECTOR					(OBJECTS_LIST, CGameObject*);

	CObject*					pCurrentControlEntity;
public:
	CObject*					CurrentControlEntity	( void ) const		{ return pCurrentControlEntity; }
	void						SetControlEntity		( CObject* O  )		{ pCurrentControlEntity=O; }
private:
	BOOL						Connect2Server();

public:

	xrServer*					Server;
	game_cl_GameState			*level_game_cl_base;

	BOOL						m_bGameConfigStarted;
	BOOL						game_configured;
	NET_Queue_Event				*game_events;
	xr_deque<CSE_Abstract*>		game_spawn_queue;

	GlobalFeelTouch				m_feel_deny;
	//CZoneList*					hud_zones_list;
	//CZoneList*					create_hud_zones_list();

private:
	// preload sounds registry
	DEFINE_MAP					(shared_str,ref_sound,SoundRegistryMap,SoundRegistryMapIt);
	SoundRegistryMap			sound_registry;

public:
	void						PrefetchSound			(LPCSTR name);
	void						SetGameTime				(u32 new_hours, u32 new_mins);

protected:
	bool	__stdcall			Gameloading_Stage_1();
	bool	__stdcall			Gameloading_Stage_3();
	bool	__stdcall			Gameloading_Stage_4();
	bool	__stdcall			Gameloading_Stage_5();
	bool	__stdcall			Gameloading_Stage_6();
	bool	__stdcall			Gameloading_Stage_7();

public:
	// sounds
	xr_vector<ref_sound*>		static_Sounds;

	// startup options
	shared_str					m_GameCreationOptions;

	// Starting/Loading
	virtual BOOL				Gameloading				(LPCSTR op_server);
	virtual void				net_Load				(LPCSTR name){};
	virtual void				net_Save				(LPCSTR name );
	virtual void				net_Stop				();
	virtual void				net_Update				();


	virtual BOOL				Load_GameSpecific_Before();
	virtual BOOL				Load_GameSpecific_After ();
	virtual void				Load_GameSpecific_CFORM	(CDB::TRI* T, u32 count);

	// Events
	virtual void				OnEvent					(EVENT E, u64 P1, u64 P2);
	virtual void				OnFrame					();
	virtual void				OnFrameBegin			();
	virtual void				OnFrameEnd				();
	virtual void				OnRender				();

	virtual void				ApplyCamera				();

	virtual void				RenderTracers			(); //tracers

	void						cl_Process_Event		(u16 dest, u16 type, NET_Packet& P);
	void						cl_Process_Spawn		(NET_Packet& P);
	void						ProcessGameEvents		();
	void						ProcessGameSpawns		();

	// Input
	virtual	void				IR_OnKeyboardPress		(int btn);
	virtual void				IR_OnKeyboardRelease	(int btn);
	virtual void				IR_OnKeyboardHold		(int btn);
	virtual void				IR_OnMousePress			(int btn);
	virtual void				IR_OnMouseRelease		(int btn);
	virtual void				IR_OnMouseHold			(int btn);
	virtual void				IR_OnMouseMove			(int, int);
	virtual void				IR_OnMouseStop			(int, int);
	virtual void				IR_OnMouseWheel			(int direction);
	virtual void				IR_OnActivate			();

	// Game
	void						InitializeClientGame	(NET_Packet& P);
	void						ClientReceive();
	void						ClientSend();
	void						ClientSave();
			u32					Objects_net_Save		(NET_Packet* _Packet, u32 start, u32 count);
	virtual	void				Send					(NET_Packet& P, u32 dwFlags=DPNSEND_GUARANTEED, u32 dwTimeout=0);
	
	void						g_cl_Spawn				(LPCSTR name, u8 rp, u16 flags, Fvector pos);
	void						g_sv_Spawn				(CSE_Abstract* E);
	
	IC CSpaceRestrictionManager		&space_restriction_manager();
	IC CSeniorityHierarchyHolder	&seniority_holder();
	IC CClientSpawnManager			&client_spawn_manager();
	IC CAutosaveManager				&autosave_manager();

#ifdef DRENDER
	IC CDebugRenderer				&debug_renderer	();
#endif
	// Lua garbage collector func
	void	__stdcall				script_gc();
	IC CPHCommander					&ph_commander_physics_worldstep();

	IC CPHCommander					&ph_commander();
	IC CPHCommander					&ph_commander_scripts();
	IC CLevelSoundManager			&level_sound_manager();
	// C/D
	CLevel();
	virtual ~CLevel();

	//названияе текущего уровня
	virtual shared_str	name					() const;

	//Функция перезагрузки кофигоф и погоды
	void						ReloadEnvironment	();
	virtual u64					GetTimeForEnv		();
	virtual float				GetTimeFactorForEnv	();
	virtual void				PlayEnvironmentEffects();

protected:	
	CMapManager*			m_map_manager;
	CGameTaskManager*		m_game_task_manager;
public:
	CMapManager&			MapManager()				{return *m_map_manager;}
	
	IC CGameTaskManager&	GameTaskManager		() const { return *m_game_task_manager; }
	IC CGameTaskManager*	GameTaskManagerP	() const { return m_game_task_manager; }

	void					OnAlifeSimulatorLoaded		();
	void					OnAlifeSimulatorUnLoaded	();

	//работа с пулями
protected:	
	CBulletManager*		m_pBulletManager;
public:
	IC CBulletManager&	BulletManager()			{return	*m_pBulletManager;}

	CSE_Abstract	*spawn_item					(LPCSTR section, const Fvector &position, u32 level_vertex_id, u16 parent_id, bool return_item = false);

	void			 DirectMoveItem				(u16 from_id, u16 to_id, u16 what_id);
public:
	void			remove_objects();

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CLevel)
#undef script_type_list
#define script_type_list save_type_list(CLevel)

IC CLevel&				Level()					{ return *((CLevel*)g_pGameLevel); }
IC CLevel*				PLevel()				{ return ((CLevel*)g_pGameLevel); }

IC game_cl_GameState&	Level_Game_Cl_Base()	{return *Level().level_game_cl_base;}

#ifdef DEBUG
IC CLevelDebug&			DBG()		{return *((CLevelDebug*)Level().m_level_debug);}
#endif	
	
IC CSpaceRestrictionManager	&CLevel::space_restriction_manager()
{
	R_ASSERT			(m_space_restriction_manager);
	return				(*m_space_restriction_manager);
}

IC CSeniorityHierarchyHolder &CLevel::seniority_holder()
{
	R_ASSERT			(m_seniority_hierarchy_holder);
	return				(*m_seniority_hierarchy_holder);
}

IC CClientSpawnManager &CLevel::client_spawn_manager()
{
	R_ASSERT			(m_client_spawn_manager);
	return				(*m_client_spawn_manager);
}

IC CAutosaveManager &CLevel::autosave_manager()
{
	R_ASSERT			(m_autosave_manager);
	return				(*m_autosave_manager);
}
#ifdef DRENDER
IC CDebugRenderer &CLevel::debug_renderer()
{
	R_ASSERT			(m_debug_renderer);
	return				(*m_debug_renderer);
}
#endif //DRENDER
IC CPHCommander	& CLevel::ph_commander()
{
	R_ASSERT			(m_ph_commander);
	return *m_ph_commander;
}
IC CPHCommander & CLevel::ph_commander_scripts()
{
	R_ASSERT			(m_ph_commander_scripts);
	return *m_ph_commander_scripts;
}

IC CPHCommander & CLevel::ph_commander_physics_worldstep()
{
	VERIFY(m_ph_commander_physics_worldstep);
	return *m_ph_commander_physics_worldstep;
}

IC CLevelSoundManager &CLevel::level_sound_manager()
{
	R_ASSERT			(m_level_sound_manager);
	return *m_level_sound_manager;
}


