////////////////////////////////////////////////////////////////////////////
//	Module 		: enemy_manager.cpp
//	Created 	: 30.12.2003
//  Modified 	: 30.12.2003
//	Author		: Dmitriy Iassenev
//	Description : Enemy manager
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "enemy_manager.h"
#include "memory_manager.h"
#include "visual_memory_manager.h"
#include "sound_memory_manager.h"
#include "hit_memory_manager.h"
#include "clsid_game.h"
#include "ef_storage.h"
#include "ef_pattern.h"
#include "autosave_manager.h"
#include "ai_object_location.h"
#include "level_graph.h"
#include "level.h"
#include "script_game_object.h"
#include "ai_space.h"
#include "profiler.h"
#include "ai/stalker/ai_stalker.h"
#include "movement_manager.h"
#include "agent_manager.h"
#include "agent_enemy_manager.h"
#include "danger_manager.h"
#include "../GameMtlLib.h"
#include "mt_config.h"
#include "memory_space_impl.h"
#include "ai\stalker\ai_stalker_impl.h"
static const u32 ENEMY_INERTIA_TIME_TO_SOMEBODY	= 3000;
static const u32 ENEMY_INERTIA_TIME_TO_ACTOR	= 0;
static const u32 ENEMY_INERTIA_TIME_FROM_ACTOR	= 3000;

#define FIRE_IF_WAS_VISIBLE_TIME	10000
#define FIRE_IF_HITED_TIME			5000

#ifdef _DEBUG
bool g_enemy_manager_second_update	 = false;
#endif

#define USE_EVALUATOR
#define FIRE_AT_ENEMY_SOUND

CEnemyManager::CEnemyManager(CCustomMonster *object)
{
	VERIFY						(object);
	m_object					= object;
	m_ignore_monster_threshold	= 1.f;
	m_max_ignore_distance		= 0.f;
	m_ready_to_save				= true;
	m_last_enemy_time			= 0;
	m_last_enemy_change			= 0;
	m_stalker					= smart_cast<CAI_Stalker*>(object);
	m_enable_enemy_change		= true;

	lastTimeEnemyFound_			= 0;

	canFireAtEnemyBehindCover_	= false;

	canFireAtEnemySound_		= false;
	canFireAtEnemyLastVis_		= false;
	canFireAtEnemyHit_			= false;

	enemiesPowerValue_			= 300.f;
	alliesPowerValue_			= 300.f;

	alliesCount_				= 0;
	enemiesCount_				= 0;

	enemyPosInMemory_			= Fvector().set(0.f, 500.f, 0.f);
	possibleEnemyLookOutPos_	= Fvector().set(0.f, 500.f, 0.f);
	m_smart_cover_enemy			= 0;
}

CEnemyManager::~CEnemyManager()
{
	Device.RemoveFromAuxthread2Pool(fastdelegate::FastDelegate0<>(this, &CEnemyManager::UpdateMT));
	Device.RemoveFromAuxthread3Pool(fastdelegate::FastDelegate0<>(this, &CEnemyManager::UpdateMT));
}

bool CEnemyManager::is_useful(const CEntityAlive *entity_alive) const
{
	bool res = m_object->useful(this, entity_alive);
	return res;
}

bool CEnemyManager::useful(const CEntityAlive *entity_alive) const
{
	if (!entity_alive)
		return(false);

	if (!entity_alive->g_Alive())
		return				(false);

	if ((entity_alive->spatial.s_type & STYPE_VISIBLEFORAI) != STYPE_VISIBLEFORAI)
		return				(false);

	if ((m_object->ID() == entity_alive->ID()) || !m_object->is_relation_enemy(entity_alive))
		return				(false);

	if (!ai().get_level_graph() || !ai().level_graph().valid_vertex_id(entity_alive->ai_location().level_vertex_id()))
		return				(false);

	if (
		m_object->human_being() &&
		!entity_alive->human_being() &&
		!expedient(entity_alive) &&
		(evaluate(entity_alive) >= m_ignore_monster_threshold) &&
		(m_object->Position().distance_to(entity_alive->Position()) >= m_max_ignore_distance)
	) return (false);

	return(m_useful_callback ? m_useful_callback(m_object->lua_game_object(),entity_alive->lua_game_object()) : true);
}

float CEnemyManager::do_evaluate(const CEntityAlive *object) const
{
	return(m_object->evaluate(this,object));
}

float CEnemyManager::evaluate(const CEntityAlive *object) const
{
//	Msg						("[%6d] enemy manager %s evaluates %s",EngineTimeU(),*m_object->ObjectName(),*object->ObjectName());

	bool actor = (object->CLS_ID == CLSID_OBJECT_ACTOR);

	if (actor)
		m_ready_to_save		= false;

	const CAI_Stalker* stalker = smart_cast<const CAI_Stalker*>(object);

	bool wounded = stalker ? stalker->wounded(&m_object->movement().restrictions()) : false;

	if (wounded)
	{
		if (m_stalker && m_stalker->agent_manager().enemy().assigned_wounded(object,m_stalker))
			return			(0.f);

		float				distance = m_object->Position().distance_to_sqr(object->Position());

		return(distance);
	}

	float penalty = 10000.f;

	// if we are hit
	if (object->ID() == m_object->memory().HitMManager().last_hit_object_id())
	{
		penalty -= 1200.f;
	}

	// if we see object
	if (m_object->memory().VisualMManager().visible_now(object))
	{
		penalty -= 1000.f;
	}
	else if (object->Position().y > m_object->Position().y + 2.8f || object->Position().y < m_object->Position().y - 2.8f)// if object is not visible right now and is on different height - add penalty
	{
		penalty += 800.f;
	}

	if (!object->g_Alive())
		penalty += 2000.f;

#ifdef USE_EVALUATOR
	ai().ef_storage().non_alife().member_item()	= 0;
	ai().ef_storage().non_alife().enemy_item()	= 0;
	ai().ef_storage().non_alife().member()		= m_object;
	ai().ef_storage().non_alife().enemy()		= object;

	float distance = m_object->Position().distance_to_sqr(object->Position());

	return(penalty + distance / 100.f + ai().ef_storage().m_pfVictoryProbability->ffGetValue() / 100.f);
#else
	float					distance = m_object->Position().distance_to_sqr(object->Position());
	return					(
		1000.f*(visible ? 0.f : 1.f) +
		distance
	);
#endif
}

bool CEnemyManager::expedient				(const CEntityAlive *object) const
{
	ai().ef_storage().non_alife().member() = m_object;

	VERIFY(ai().ef_storage().non_alife().member());

	ai().ef_storage().non_alife().enemy() = object;

	if (ai().ef_storage().m_pfExpediency->dwfGetDiscreteValue())
		return(true);

	if (m_object->memory().HitMManager().hit(ai().ef_storage().non_alife().enemy()))
		return(true);

	return(false);
}

void CEnemyManager::reload					(LPCSTR section)
{
	m_ignore_monster_threshold	= READ_IF_EXISTS(pSettings, r_float, section, "ignore_monster_threshold", 1.f);
	m_max_ignore_distance		= READ_IF_EXISTS(pSettings, r_float, section, "max_ignore_distance", 0.f);

	clearEnemiesTime_			= READ_IF_EXISTS(pSettings, r_u32, section, "clear_enemy_time", 60);
	clearEnemiesTime_			*= 1000;

	m_last_enemy_time			= 0;
	m_last_enemy				= 0;
	m_last_enemy_change			= 0;

	m_useful_callback.clear();

	VERIFY(m_ready_to_save);

	lastTimeEnemyFound_			= 0;
}

void CEnemyManager::set_ready_to_save()
{
	if (m_ready_to_save)
		return;

//	Msg							("%6d %s DEcreased enemy counter for player (%d -> %d)",EngineTimeU(),*m_object->ObjectName(),Level().autosave_manager().not_ready_count(),Level().autosave_manager().not_ready_count()-1);
	Level().autosave_manager().dec_not_ready();

	m_ready_to_save	= true;
}


void CEnemyManager::remove_links			(CObject *object)
{
	// since we use no members in CEntityAlive during search,
	// we just use the pinter itself, we can just statically cast object

	OBJECTS::iterator			I = std::find(objectsArray_.begin(), objectsArray_.end(), (CEntityAlive*)object);
	if (I != objectsArray_.end())
		objectsArray_.erase(I);

	if (m_last_enemy == object)
		m_last_enemy			= 0;

	if (m_selected == object)
		m_selected				= 0;
}


void CEnemyManager::ignore_monster_threshold			(const float &ignore_monster_threshold)
{
	m_ignore_monster_threshold	= ignore_monster_threshold;
}


void CEnemyManager::restore_ignore_monster_threshold	()
{
	m_ignore_monster_threshold	= READ_IF_EXISTS(pSettings,r_float,*m_object->SectionName(),"ignore_monster_threshold",1.f);
}


float CEnemyManager::ignore_monster_threshold			() const
{
	return(m_ignore_monster_threshold);
}


void CEnemyManager::max_ignore_monster_distance(const float &max_ignore_monster_distance)
{
	m_max_ignore_distance = max_ignore_monster_distance;
}



void CEnemyManager::restore_max_ignore_monster_distance()
{
	m_max_ignore_distance = READ_IF_EXISTS(pSettings,r_float,*m_object->SectionName(),"max_ignore_distance",0.f);
}


float CEnemyManager::max_ignore_monster_distance() const
{
	return(m_max_ignore_distance);
}


bool CEnemyManager::change_from_wounded					(const CEntityAlive *current, const CEntityAlive *previous) const
{
	const CAI_Stalker			*current_stalker = smart_cast<const CAI_Stalker*>(current);
	if (!current_stalker)
		return					(false);

	if (current_stalker->wounded())
		return					(false);

	const CAI_Stalker			*previous_stalker = smart_cast<const CAI_Stalker*>(previous);
	if (!previous_stalker)
		return					(false);

	if (!previous_stalker->wounded())
		return					(false);

	return						(true);
}

IC	bool CEnemyManager::enemy_inertia(const CEntityAlive *previous_enemy) const
{
	if (m_selected->CLS_ID == CLSID_OBJECT_ACTOR)
		return (EngineTimeU() <= (m_last_enemy_change + ENEMY_INERTIA_TIME_TO_ACTOR));

	if (previous_enemy && previous_enemy->CLS_ID == CLSID_OBJECT_ACTOR)
		return (EngineTimeU() <= (m_last_enemy_change + ENEMY_INERTIA_TIME_FROM_ACTOR));

	return (EngineTimeU() <= (m_last_enemy_change + ENEMY_INERTIA_TIME_TO_SOMEBODY));
}

void CEnemyManager::on_enemy_change(const CEntityAlive *previous_enemy)
{
	VERIFY(previous_enemy);
	VERIFY(selected());

	if (!previous_enemy->g_Alive())
	{
		m_last_enemy_change		= EngineTimeU();

		return;
	}

	if (change_from_wounded(selected(),previous_enemy))
	{
		m_last_enemy_change		= EngineTimeU();

		return;
	}

	if (enemy_inertia(previous_enemy))
	{
		m_selected				= previous_enemy;

		return;
	}

	if (!m_object->memory().VisualMManager().visible_now(previous_enemy) && m_object->memory().VisualMManager().visible_now(selected()))
	{
		m_last_enemy_change		= EngineTimeU();

		return;
	}

	m_last_enemy_change			= EngineTimeU();
}

void CEnemyManager::remove_wounded()
{
	struct no_wounded
	{
		IC	static bool	predicate	(const CEntityAlive *enemy)
		{
			if (!enemy->g_Alive())
				return				(true);

			const CAI_Stalker		*stalker = smart_cast<const CAI_Stalker*>(enemy);
			if (!stalker)
				return				(false);

			if (!stalker->wounded())
				return				(false);

			return					(true);
		}
	};

	objectsArray_.erase(std::remove_if(objectsArray_.begin(), objectsArray_.end(), &no_wounded::predicate), objectsArray_.end());
}

void CEnemyManager::process_wounded(bool &only_wounded)
{
	only_wounded = true;

	ENEMIES::const_iterator		I = objectsArray_.begin();
	ENEMIES::const_iterator		E = objectsArray_.end();

	for ( ; I != E; ++I)
	{
		const CAI_Stalker		*stalker = smart_cast<const CAI_Stalker*>(*I);

		if (stalker && stalker->wounded())
			continue;

		only_wounded = false;

		break;
	}

	if (only_wounded)
	{
#if 0//def _DEBUG
		if (g_enemy_manager_second_update)
			Msg					("%6d ONLY WOUNDED LEFT %s",EngineTimeU(),*m_object->ObjectName());
#endif

		return;
	}

	remove_wounded				();
}

bool CEnemyManager::need_update(const bool &only_wounded) const
{
	if (!selected())
		return					(true);

	if (!selected()->g_Alive())
		return					(true);

	if (enable_enemy_change() && !m_object->memory().VisualMManager().visible_now(selected()))
		return					(true); 

	if ( !m_object->is_relation_enemy(selected()) )
		return					(true);

	if (only_wounded)
		return					(false);

	const CAI_Stalker* stalker = smart_cast<const CAI_Stalker*>(selected());

	if (stalker && stalker->wounded())
		return					(true);

	u32	last_hit_time			= m_object->memory().HitMManager().last_hit_time();
	if (last_hit_time && (last_hit_time > m_last_enemy_change) ) {
		ALife::_OBJECT_ID		enemy_id	= m_object->memory().HitMManager().last_hit_object_id();
		VERIFY					( enemy_id != ALife::_OBJECT_ID(-1) );
		CObject const* enemy	= Level().Objects.net_Find(enemy_id);
		VERIFY					(enemy);
		CEntityAlive const*		alive_enemy = smart_cast<CEntityAlive const*>(enemy);
		if (alive_enemy && m_object->is_relation_enemy(alive_enemy))
			return				(true);
	}
	return						(false);
}


void CEnemyManager::try_change_enemy()
{
	const CEntityAlive* previous_selected = selected();

	bool only_wounded;
	process_wounded(only_wounded);

	if (!need_update(only_wounded))
		return;

	inherited::update();

	if (selected() != previous_selected)
	{
		if (selected() && previous_selected)
			on_enemy_change		(previous_selected);
		else
			m_last_enemy_change	= EngineTimeU();
	}

	if (selected() != previous_selected)
		m_object->on_enemy_change	(previous_selected);
}

static bool LevelTimePred(const CMemoryObject<CGameObject>* x, const CMemoryObject<CGameObject>* y)
{
	return x->m_level_time > y->m_level_time;
}

void CEnemyManager::UpdateEnemyPos()
{
	const CVisibleObject* vis = m_object->memory().VisualMManager().GetIfIsInPool(selected());
	const CHitObject* hit = m_object->memory().HitMManager().GetIfIsInPool(selected());
	const CSoundObject* snd = m_object->memory().SoundMManager().GetIfIsInPool(selected());

	xr_vector<const CMemoryObject<CGameObject>*> compareChart_;

	if (vis && vis->m_object)
		compareChart_.push_back(vis);

	if (hit && hit->m_object)
		compareChart_.push_back((CMemoryObject<CGameObject>*)hit);

	if (snd && snd->m_object)
		compareChart_.push_back(snd);

	if (!compareChart_.size())
		return;

	std::sort(compareChart_.begin(), compareChart_.end(), LevelTimePred);

	VERIFY(compareChart_.front()->m_object);
	VERIFY(compareChart_.front()->m_object->SectionNameStr());

	enemyPosInMemory_ = compareChart_.front()->m_object_params.GetMemorizedPos();

	VERIFY(_valid(enemyPosInMemory_));
}

ICF static BOOL ray_callback(collide::rq_result& result, LPVOID params)
{
	collide::rq_result* RQ = (collide::rq_result*)params;

	if (result.O)
	{
		*RQ = result;

		return FALSE;
	}
	else
	{
		//получить треугольник и узнать его материал
		CDB::TRI* T = Level().ObjectSpace.GetStaticTris() + result.element;

		if (GMLib.GetMaterialByIdx(T->material)->Flags.is(SGameMtl::flPassable) || GMLib.GetMaterialByIdx(T->material)->Flags.is(SGameMtl::flBreakable))
			return TRUE;
	}

	*RQ = result;

	return FALSE;
}

void CEnemyManager::UpdateCanFireIfNotSeen()
{
	canFireAtEnemySound_ = false;
	canFireAtEnemyLastVis_ = false;
	canFireAtEnemyHit_ = false;

	if (!CanFireAtEnemyBehindCover())
		return;

	const CVisibleObject* vis = m_object->memory().VisualMManager().GetIfIsInPool(selected());

	if (vis && vis->m_object && vis->m_level_time != u32(-1) && vis->m_level_time + FIRE_IF_WAS_VISIBLE_TIME > EngineTimeU())
		canFireAtEnemyLastVis_ = true;

	const CHitObject* hit = m_object->memory().HitMManager().GetIfIsInPool(selected());

	if (hit && hit->m_object && hit->m_level_time + FIRE_IF_HITED_TIME > EngineTimeU())
		canFireAtEnemyHit_ = true;


#ifdef FIRE_AT_ENEMY_SOUND
	const CSoundObject* snd = m_object->memory().SoundMManager().GetIfIsInPool(selected());

	if (snd && snd->m_object && snd->m_level_time + 3000 > EngineTimeU())
	{
		Fvector sound_pos = snd->m_object->Position();
		Fvector our_pos = m_object->eye_matrix.c;

		float distance = our_pos.distance_to(sound_pos);

		if (distance > 5.f)
			canFireAtEnemySound_ = true;
	}
#endif


}

void CEnemyManager::UpdateCanFireAtEnemyBehindCover()
{
	Fvector enemy_pos = EnemyPosInMemory();
	Fvector our_pos = m_object->eye_matrix.c;

	float distance = our_pos.distance_to(enemy_pos);

	Fvector dir;

	enemy_pos.y += 0.5f;
	dir.sub(enemy_pos, our_pos).normalize_safe();
	float ray_length = distance - 3.f; // To make it ok to fire at enemy hiding behind the trees or other covers

	RQ.O = nullptr;
	RQ.range = ray_length;
	RQ.element = -1;

	collide::ray_defs RD(our_pos, dir, RQ.range, CDB::OPT_CULL, collide::rqtBoth);

	RQR.r_clear();

	BOOL res1 = Level().ObjectSpace.RayQuery(RQR, RD, ray_callback, &RQ, NULL, m_object);

	// Now look for "through" point or a hole near the cover. A point, at which it is making sence to fire
	if (res1 == 0 || RQ.O == selected())
	{
		//Msg("2 %f %f %f", VPUSH(enemy_pos));

		for (u8 i = 0; i < 3; i++)
		{
			Fvector enemy_pos2 = enemy_pos;
			Fvector shift;

			shift.mul(m_object->eye_matrix.j, i == 0 ? 0.f : i == 1 ? 0.7f : -0.7f);

			enemy_pos2.add(shift);

			for (u8 y = 0; y < 3; y++)
			{
				Fvector enemy_pos3 = enemy_pos2;

				shift.mul(m_object->eye_matrix.i, y == 0 ? 0.f : y == 1 ? 0.7f : -0.7f);

				enemy_pos3.add(shift);

				//Msg("%u %u %f %f %f", i, y, VPUSH(enemy_pos3));

				// do raytest towards shifted position

				distance = our_pos.distance_to(enemy_pos3);
				dir.sub(enemy_pos3, our_pos).normalize_safe();
				
				ray_length = distance + 3.f;

				RQ.O = nullptr;
				RQ.range = ray_length;
				RQ.element = -1;

				collide::ray_defs RD(our_pos, dir, RQ.range, CDB::OPT_CULL, collide::rqtBoth);

				RQR.r_clear();

				res1 = Level().ObjectSpace.RayQuery(RQR, RD, ray_callback, &RQ, NULL, m_object);

				//Msg("%s %d", res1 ? "-" : "*", res1);

				if (!res1)
				{
					possibleEnemyLookOutPos_ = enemy_pos3;

					canFireAtEnemyBehindCover_ = true;

					return;
				}
			}
		}
	}

	canFireAtEnemyBehindCover_ = false;
}

void CEnemyManager::UpdateEnemiesAndAlliesPower()
{
	enemiesPowerValue_ = 500.f;
	alliesPowerValue_ = 500.f;

	alliesCount_ = 0;
	enemiesCount_ = 0;

	{
		for (u32 i = 0; i < m_object->memory().VisualMManager().GetMemorizedObjects().size(); i++)
		{
			const CVisibleObject* vis = &m_object->memory().VisualMManager().GetMemorizedObjects()[i];

			if (!vis->m_enabled)
				continue;

			const CEntityAlive* entity_alive = smart_cast<const CEntityAlive*>(vis->m_object);

			if (entity_alive && entity_alive->g_Alive())
			{
				if (vis->m_level_time + m_object->memory().VisualMManager().forgetVisTime_ > EngineTimeU())
				{
					const CAI_Stalker* stalker = smart_cast<const CAI_Stalker*>(vis->m_object);

					float value = 300.f;

					if (stalker && !stalker->wounded())
						value += (float)stalker->GetIORank();

					value *= entity_alive->GetfHealth();

					if (m_object->is_relation_enemy(entity_alive))
					{
						enemiesCount_++;

						enemiesPowerValue_ += value;
					}
					else
					{
						alliesCount_++;

						alliesPowerValue_ += value;
					}
				}
			}
		}
	}
}

struct SRemoveOfflinePredicate
{
	bool operator() (const CEntityAlive* object) const
	{
		if (!object)
			return true;

		if (!object->g_Alive())
			return true;

		return(!!object->getDestroy() || object->H_Parent());
	}
};

void CEnemyManager::update()
{
	START_PROFILE("Memory Manager/enemies::update")

		if (!m_ready_to_save)
		{
			//		Msg						("%6d %s DEcreased enemy counter for player (%d -> %d)",EngineTimeU(),*m_object->ObjectName(),Level().autosave_manager().not_ready_count(),Level().autosave_manager().not_ready_count()-1);
			Level().autosave_manager().dec_not_ready();
		}

	m_ready_to_save = true;

	objectsArray_.erase(std::remove_if(objectsArray_.begin(), objectsArray_.end(), SRemoveOfflinePredicate()), objectsArray_.end());

	// Clears enemies memory, which should lead to exiting danger state

	if (selected() && lastTimeEnemyFound_ + clearEnemiesTime_ < EngineTimeU() || lastTimeEnemyFound_ > u32(-1) - 500)
	{
		reset();
		m_selected = nullptr;
		m_object->memory().DangerManager().reset();

		if (m_stalker)
		{
			if (m_stalker->ActiveSoundsNum() <= 0 && ::Random.randI(1, 4))
				m_stalker->PlayFinishFightSound();
		}
	}
	else if (objectsArray_.size())
	{
		try_change_enemy();
	}

	if (selected())
	{
		m_last_enemy_time		= EngineTimeU();
		m_last_enemy			= selected();
	}

	if (g_mt_config.test(mtAIMisc))
	{
		if (m_object->mtUpdateThread_ == 1)
			Device.AddToAuxThread_Pool(2, fastdelegate::FastDelegate0<>(this, &CEnemyManager::UpdateMT));
		else // if cpu has more than 4 physical cores - send half of the workload to another aux thread
			Device.AddToAuxThread_Pool(CPU::GetPhysicalCoresNum() > 4 ? 3 : 2, fastdelegate::FastDelegate0<>(this, &CEnemyManager::UpdateMT));
	}
	else
		UpdateMT();

	if (!m_ready_to_save)
	{
//		Msg						("%6d %s INcreased enemy counter for player (%d -> %d)",EngineTimeU(),*m_object->ObjectName(),Level().autosave_manager().not_ready_count(),Level().autosave_manager().not_ready_count()+1);
		Level().autosave_manager().inc_not_ready();
	}

#if 0//def _DEBUG
	if (g_enemy_manager_second_update && selected() && smart_cast<const CAI_Stalker*>(selected()) && smart_cast<const CAI_Stalker*>(selected())->wounded())
		Msg						("%6d WOUNDED CHOOSED %s",EngineTimeU(),*m_object->ObjectName());
#endif // _DEBUG


	STOP_PROFILE
}

void CEnemyManager::UpdateMT()
{
#ifdef MEASURE_MT
	CTimer measure_mt; measure_mt.Start();
#endif


	// for prestored data for perfomance important places
	UpdateStoredMemory();

	if (selected())
	{
		UpdateEnemyPos();

		UpdateCanFireAtEnemyBehindCover();

		UpdateCanFireIfNotSeen();

		UpdateEnemiesAndAlliesPower();
	}


#ifdef MEASURE_MT
	Device.Statistic->mtAIMiscTime_ += measure_mt.GetElapsed_sec();
#endif
}

void CEnemyManager::UpdateStoredMemory()
{
	// Update raw data holder
	protectMemUpdate_.Enter();

	CVisualMemoryManager& vm = m_object->memory().VisualMManager();
	CSoundMemoryManager& sm = m_object->memory().SoundMManager();

	if (vm.m_object)
		vm.UpdateStoredVisibles();

	sm.UpdateStoredHeard();

	// Update mem pool of entity_alives
	prestoredAllies_.clear();
	prestoredEnemies_.clear();

	CMemoryInfo	result;

	for (u32 i = 0; i < 2; i++)
	{
		const CVisualMemoryManager::VISIBLES& pool = (i == 0) ? vm.GetVisibleAllies() : vm.GetVisibleEnemies();
		MEMORY_POOL& storage = (i == 0) ? prestoredAllies_ : prestoredEnemies_;

		auto I = pool.begin();
		auto E = pool.end();

		for (; I != E; ++I)
		{
			auto mem = (*I);

			(CVisibleObject&)result = mem;

			result.m_visual_info = true;

			VERIFY(result.m_object);

			storage.push_back(result);
		}
	}

	for (u32 i = 0; i < 2; i++)
	{
		const CSoundMemoryManager::SOUNDS& pool = (i == 0) ? sm.GetHeardAllies() : sm.GetHeardEnemies();
		MEMORY_POOL& storage = (i == 0) ? prestoredAllies_ : prestoredEnemies_;

		auto I = pool.begin();
		auto E = pool.end();

		for (; I != E; ++I)
		{
			auto mem = (*I);

			if (!mem.m_object) // not all sounds have object ?
				continue;

			auto it = std::find(storage.begin(), storage.end(), object_id(mem.m_object));

			if (storage.end() != it) // skip it, we have it from vis memory already
				continue;

			(CMemoryObject<CGameObject>&)result = (CMemoryObject<CGameObject>&)mem;
			result.m_sound_info = true;

			VERIFY(result.m_object);

			storage.push_back(result);
		}
	}

	protectMemUpdate_.Leave();
}
