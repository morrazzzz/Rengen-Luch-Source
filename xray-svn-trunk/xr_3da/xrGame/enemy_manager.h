////////////////////////////////////////////////////////////////////////////
//	Module 		: enemy_manager.h
//	Created 	: 30.12.2003
//  Modified 	: 30.12.2003
//	Author		: Dmitriy Iassenev
//	Description : Enemy manager
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "object_manager.h"
#include "entity_alive.h"
#include "custommonster.h"
#include "script_callback_ex.h"
#include "memory_space.h"

class CAI_Stalker;

class CEnemyManager : public CObjectManager<const CEntityAlive> {
public:
	typedef CObjectManager<const CEntityAlive>	inherited;
	typedef OBJECTS								ENEMIES;
	typedef CScriptCallbackEx<bool>				USEFULE_CALLBACK;

	typedef xr_vector<CMemoryInfo>				MEMORY_POOL;

private:
	CAI_Stalker					*m_stalker;
	float						m_ignore_monster_threshold;
	float						m_max_ignore_distance;
	mutable bool				m_ready_to_save;
	u32							m_last_enemy_time;
	const CEntityAlive			*m_last_enemy;
	USEFULE_CALLBACK			m_useful_callback;
	bool						m_enable_enemy_change;
	CEntityAlive const			*m_smart_cover_enemy;

	Fvector						enemyPosInMemory_;

	Fvector						possibleEnemyLookOutPos_;

	bool						canFireAtEnemySound_;
	bool						canFireAtEnemyLastVis_;
	bool						canFireAtEnemyHit_;

	bool						canFireAtEnemyBehindCover_;

	float						enemiesPowerValue_;
	float						alliesPowerValue_;

	u32							alliesCount_;
	u32							enemiesCount_;

	collide::rq_result			RQ;
	collide::rq_results			RQR;

	MEMORY_POOL					prestoredAllies_;
	MEMORY_POOL					prestoredEnemies_;
private:
	u32							m_last_enemy_change;
	u32							lastTimeEnemyFound_;

private:
	IC		bool				enemy_inertia		(const CEntityAlive *previous_enemy) const;
			bool				need_update			(const bool &only_wounded) const;
			void				process_wounded		(bool &only_wounded);
			bool				change_from_wounded	(const CEntityAlive *current, const CEntityAlive *previous) const;
			void				remove_wounded		();
			void				try_change_enemy	();

			void				UpdateEnemyPos		();
			void 				UpdateCanFireIfNotSeen();
			void 				UpdateCanFireAtEnemyBehindCover();
			void				UpdateEnemiesAndAlliesPower();

			void _stdcall		UpdateMT			();

			void				UpdateStoredMemory	();

	AccessLock					protectMemUpdate_;

protected:
			void				on_enemy_change		(const CEntityAlive *previous_enemy);
			bool				expedient			(const CEntityAlive *object) const;

public:
								CEnemyManager		(CCustomMonster *object);
	virtual						~CEnemyManager		();
	virtual void				reload				(LPCSTR section);

	IC		bool				AddEnemy			(const CEntityAlive *object);

	virtual bool				useful				(const CEntityAlive *object) const;
	virtual bool				is_useful			(const CEntityAlive *object) const;
	virtual	float				evaluate			(const CEntityAlive *object) const;
	virtual	float				do_evaluate			(const CEntityAlive *object) const;
	virtual void				update				();
	virtual void				set_ready_to_save	();
	IC		u32					last_enemy_time		() const;
	IC		const CEntityAlive	*last_enemy			() const;
	IC		USEFULE_CALLBACK	&useful_callback	();
			void				remove_links		(CObject *object);

	IC		u32					LastTimeEnemyFound() { return lastTimeEnemyFound_; };

	CCustomMonster				*m_object;

	// Get the enemy position from raw data memories.
	IC const Fvector&			EnemyPosInMemory() const		{ R_ASSERT(_valid(enemyPosInMemory_)); return enemyPosInMemory_; };

	IC const Fvector&			PossibleEnemyLookOutPos() const	{ R_ASSERT(_valid(possibleEnemyLookOutPos_)); return possibleEnemyLookOutPos_; };

	
	IC		bool				CanFireAtEnemySound	()			{ return canFireAtEnemySound_; };
	IC		bool				CanFireAtEnemyHit	()			{ return canFireAtEnemyHit_; };
	IC		bool				CanFireAtEnemyLastSeenPos()		{ return canFireAtEnemyLastVis_; };

	// Is it making sence for NPC to fire at last memorysed enemy position. Ignores enemy cover by decreasing ray query lenghs by couple of meters
	IC		bool				CanFireAtEnemyBehindCover()		{ return canFireAtEnemyBehindCover_; };

	
			float				GetEnemiesPower		()			{ return enemiesPowerValue_; };
			float				GetAlliesPower		()			{ return alliesPowerValue_; };

			u32					AlliesCount			()			{ return alliesCount_; };
			u32					EnemiesCount		()			{ return enemiesCount_; };

				// prestored ALLY list that are known for us. Keep an eye that it updated in "Delayed Multithreaded stage"
	const MEMORY_POOL&			GetPrestoredAllies	()			{ return prestoredAllies_; }
	// prestored ENEMY list that are known for us. Keep an eye that it updated in "Delayed Multithreaded stage"
	const MEMORY_POOL&			GetPrestoredEnemies	()			{ return prestoredEnemies_; }
public:
			void				ignore_monster_threshold			(const float &ignore_monster_threshold);
			void				restore_ignore_monster_threshold	();
			float				ignore_monster_threshold			() const;
			void				max_ignore_monster_distance			(const float &max_ignore_monster_distance);
			void				restore_max_ignore_monster_distance	();
			float				max_ignore_monster_distance			() const;

			u32					clearEnemiesTime_;

public:
			void				wounded				(const CEntityAlive *wounded_enemy);
	IC		const CEntityAlive	*wounded			() const;
	IC		CEntityAlive const	*selected			() const;
	IC		void				set_enemy			(CEntityAlive const	*enemy);
	IC		void				invalidate_enemy	();

public:
	IC		void				enable_enemy_change	(const bool &value);
	IC		bool				enable_enemy_change	() const;
	IC		void				reset() override { objectsArray_.clear(); if (selected()) objectsArray_.push_back(selected()); };
};

#include "enemy_manager_inline.h"