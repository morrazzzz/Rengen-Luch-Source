////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_update_manager.h
//	Created 	: 25.12.2002
//  Modified 	: 12.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife Simulator update manager
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "alife_switch_manager.h"
#include "alife_surge_manager.h"
#include "alife_storage_manager.h"
#include "xrServer_Objects_ALife_Monsters.h"

namespace RestrictionSpace {
	enum ERestrictorTypes;
}

class CALifeUpdateManager :
	public CALifeSwitchManager,
	public CALifeSurgeManager,
	public CALifeStorageManager,
	public ISheduled
{
private:
	bool				m_first_time;

protected:
	// Limit for itterating time per call for Safe map itterator of Graph (in seconds)
	float				maxTimeALifeGraphRegUpdate_;
	// Limit for itterating time per call for Safe map itteratoration process in ALife Schedule Registry 'update' (in seconds)
	float				maxTimeALifeScRegUpdate_;

	u32					maxObjectsALifeScRegUpdate_;

	bool				m_changing_level;

public:
			void __stdcall	update				();

protected:
			void		new_game				(LPCSTR	save_name);
			void		init_ef_storage			() const;
	virtual	void		reload					(LPCSTR section);

public:
						CALifeUpdateManager		(xrServer *server, LPCSTR section);
	virtual 			~CALifeUpdateManager	();
	virtual	shared_str	SchedulerName			() const		{ return shared_str("alife_simulator"); };
	virtual float		shedule_Scale			();
	virtual void		ScheduledUpdate			(u32 dt);	
	virtual bool		shedule_Needed			()				{return true;};
			void		update_switch			();
			void		update_scheduled		(bool init_ef = true);
			void		load					(LPCSTR game_name = 0, bool no_assert = false, bool new_only = false);
			bool		load_game				(LPCSTR game_name, bool no_assert = false);
	IC		float		update_monster_factor	() const;
			bool		change_level			(NET_Packet	&net_packet);

			void		SetGraphProcessTimeLimit(float seconds);
			void		SetRegidtryUpdateLimits (const u32 objects_per_update, const float time_limit);

			float		GetRegUpdTimeLimit()	{ return maxTimeALifeScRegUpdate_; };
			u32			GetRegUpdObjLimit()		{ return maxObjectsALifeScRegUpdate_; };
			float		GetGraphUpdTimeLimit()	{ return maxTimeALifeGraphRegUpdate_; };

			void		set_switch_online		(ALife::_OBJECT_ID id, bool value);
			void		set_switch_offline		(ALife::_OBJECT_ID id, bool value);
			void		set_interactive			(ALife::_OBJECT_ID id, bool value);
			void		jump_to_level			(LPCSTR level_name) const;
			void		teleport_object			(ALife::_OBJECT_ID id, GameGraph::_GRAPH_ID game_vertex_id, u32 level_vertex_id, const Fvector &position);
			void		switch_to_offline		(ALife::_OBJECT_ID id);
			void		switch_to_online		(ALife::_OBJECT_ID id);
			void		add_restriction			(ALife::_OBJECT_ID id, ALife::_OBJECT_ID restriction_id, const RestrictionSpace::ERestrictorTypes &restriction_type);
			void		remove_restriction		(ALife::_OBJECT_ID id, ALife::_OBJECT_ID restriction_id, const RestrictionSpace::ERestrictorTypes &restriction_type);
			void		remove_all_restrictions	(ALife::_OBJECT_ID id, const RestrictionSpace::ERestrictorTypes &restriction_type);

			bool		anomalyPoolCreated_;
#ifdef XRGAME_EXPORTS
			// All anomalies on server
			xr_vector<CSE_ALifeAnomalousZone*>	anomaliesOnServer_;
#endif
			bool		IsAnomalyPoolCreated() { return anomalyPoolCreated_; };
			void		ReUpdateAnomalyPool() { anomalyPoolCreated_ = false; };

			void		UpdateServerAnomalies();
			void		CreateAnomaliesPool();

			u32			anomalyPoolUpdIndex_;

			// total arts owned by anomalies in game
			u32			totalSpawnedAnomalyArts_;
			// Get Total Anomaly Owned Arts on server
			u32			GetTotalSpawnedAnomArts();
			void		CountAnomalyArtsOnServer();

			void		DoForceRespawnArts(); // takes all anomalies in game and does respawn ignoring timers
};

#include "alife_update_manager_inline.h"