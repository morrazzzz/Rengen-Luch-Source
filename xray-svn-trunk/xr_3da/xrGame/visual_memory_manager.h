
//	Created 	: 02.10.2001
//	Author		: Dmitriy Iassenev

#pragma once

#include "visual_memory_params.h"
#include "memory_space.h"

class CCustomMonster;
class CAI_Stalker;
class vision_client;

class CVisualMemoryManager
{
#ifdef DEBUG
	friend class CAI_Stalker;
#endif
public:
	typedef MemorySpace::CVisibleObject			CVisibleObject;
	typedef MemorySpace::CNotYetVisibleObject	CNotYetVisibleObject;
	typedef xr_vector<CVisibleObject>			VISIBLES;
	typedef xr_vector<CObject*>					RAW_VISIBLES;
	typedef xr_vector<CNotYetVisibleObject>		NOT_YET_VISIBLES;

private:
	struct CDelayedVisibleObject
	{
		ALife::_OBJECT_ID	m_object_id;
		CVisibleObject		m_visible_object;
	};

private:
	typedef xr_vector<CDelayedVisibleObject>	DELAYED_VISIBLE_OBJECTS;

private:
	vision_client		*m_client;

private:
	RAW_VISIBLES		m_visible_objects;
	VISIBLES			visibleObjects_;
	NOT_YET_VISIBLES	m_not_yet_visible_objects;

	VISIBLES			vObjectsForSaving_;

	VISIBLES			storedVisibleNowAllies_;
	VISIBLES			storedVisibleNowEnemies_;
private:
	DELAYED_VISIBLE_OBJECTS	m_delayed_objects;

private:
	CVisionParameters	m_free;
	CVisionParameters	m_danger;

	// Домножители параметров видимости спокойствия для ранга
	SVisionParametersKoef freeRankCoef_;
	// Домножители параметров видимости тревоги для ранга
	SVisionParametersKoef dangerRankCoef_;

private:
	u32					m_max_object_count;
	bool				m_enabled;
	u32					m_last_update_time;

public:
			void	add_visible_object		(const CObject *object, float time_delta, bool fictitious = false);

	CCustomMonster		*m_object;
	CAI_Stalker			*m_stalker;

protected:
	IC		void	fill_object				(CVisibleObject &visible_object, const CGameObject *game_object);
			bool	should_ignore_object	(CObject const* object) const;
			float	object_visible_distance	(const CGameObject *game_object, float &object_distance) const;
			float	object_luminocity		(const CGameObject *game_object) const;
			float	get_visible_value		(float distance, float object_distance, float time_delta, float object_velocity, float luminocity) const;
			float	get_object_velocity		(const CGameObject *game_object, const CNotYetVisibleObject &not_yet_visible_object) const;
			u32		get_prev_time			(const CGameObject *game_object) const;

public:
			u32		visible_object_time_last_seen			(const CObject *object) const;

protected:
			void	add_not_yet_visible_object				(const CNotYetVisibleObject &not_yet_visible_object);
			CNotYetVisibleObject *not_yet_visible_object	(const CGameObject *game_object);

private:
			void	initialize				();
public:
					CVisualMemoryManager	(CCustomMonster *object);
					CVisualMemoryManager	(CAI_Stalker *stalker);
					CVisualMemoryManager	(vision_client *client);
	virtual			~CVisualMemoryManager	();
	virtual	void	reinit					();
	virtual	void	reload					(LPCSTR section);
	virtual	void	update					(float time_delta);
	virtual	float	feel_vision_mtl_transp	(CObject* O, u32 element);	
			void	remove_links			(CObject *object);

			bool	IsOKToCheckInvalid		() { return m_delayed_objects.empty(); };

			//update the section for rank dependance. Usualy comes from rank change call back
			void	UpdateVisability_RankDependance();

			// update prestored list of visible now alives for getting it in perfomance inportant places
			void	UpdateStoredVisibles	();

			// returns visible_now allies
	const VISIBLES&		GetVisibleAllies	() { return storedVisibleNowAllies_; };
			// returns visible_now enemies
	const VISIBLES&		GetVisibleEnemies	() { return storedVisibleNowEnemies_; };
public:
			bool	visible					(const CGameObject *game_object, float time_delta);
			bool	visible					(u32 level_vertex_id, float yaw, float eye_fov) const;

public:
	IC		void	set_squad_objects		(xr_vector<CVisibleObject> *squad_objects);
			CVisibleObject *visible_object	(const CGameObject *game_object);
			
public:
			// this function returns true if and only if 
			// specified object is visible now
			bool	visible_right_now		(const CGameObject *game_object) const;
			// if current_params.m_still_visible_time == 0
			// this function returns true if and only if 
			// specified object is visible now
			// if current_params.m_still_visible_time > 0
			// this function returns true if and only if 
			// specified object is visible now or 
			// some time ago <= current_params.m_still_visible_time
			bool	visible_now				(const CGameObject *game_object) const;
			const CVisibleObject* GetIfIsInPool(const CGameObject *game_object);

			void	add_visible_object		(CVisibleObject visible_object, bool from_somebody_else);
public:
			void	enable					(const CObject *object, bool enable);

public:
	IC		bool	enabled					() const;
	IC		void	enable					(bool value);

public:
	IC		const VISIBLES&			GetMemorizedObjects() const;
	IC		VISIBLES*				GetMemorizedObjectsP();

	u32								forgetVisTime_;
	u8								countOfSavedVisibleAllies_;
	bool							ignoreVisMemorySharing_;

	IC		const RAW_VISIBLES		&raw_objects				() const;
	IC		const NOT_YET_VISIBLES	&not_yet_visible_objects	() const;
	IC		const CVisionParameters &current_state				() const;
	IC		const SVisionParametersKoef &RankDependancy			() const;
	IC		squad_mask_type			mask						() const;

	IC		float					GetMinViewDistance() const;
	IC		float					GetMaxViewDistance() const;
	IC		float					GetVisibilityThreshold() const;
	IC		float					GetAlwaysVisibleDistance() const;
	IC		float					GetTimeQuant() const;
	IC		float					GetDecreaseValue() const;
	IC		float					GetVelocityFactor() const;
	IC		float					GetTransparencyThreshold() const;
	IC		float					GetLuminocityFactor() const;
	IC		u32						GetStillVisibleTime() const;

	IC		float					GetRankEyeFovK() const;
	IC		float					GetRankEyeRangeK() const;

public:
#ifdef DEBUG
			void					check_visibles				() const;
#endif

public:
			void					save						(NET_Packet &packet);
			void					load						(IReader &packet);
			void					on_requested_spawn			(CObject *object);

private:
			void					clear_delayed_objects		();
};

#include "visual_memory_manager_inline.h"