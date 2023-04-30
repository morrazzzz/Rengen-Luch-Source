////////////////////////////////////////////////////////////////////////////
//	Module 		: stalker_property_evaluators.cpp
//	Created 	: 25.03.2004
//  Modified 	: 26.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Stalker property evaluators classes
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "stalker_property_evaluators.h"
#include "ai/stalker/ai_stalker.h"
#include "stalker_decision_space.h"
#include "script_game_object.h"
#include "ai/ai_monsters_misc.h"
#include "inventory.h"
#include "alife_simulator.h"
#include "alife_object_registry.h"
#include "memory_manager.h"
#include "visual_memory_manager.h"
#include "sound_memory_manager.h"
#include "hit_memory_manager.h"
#include "item_manager.h"
#include "enemy_manager.h"
#include "danger_manager.h"
#include "ai_space.h"
#include "ai/stalker/ai_stalker.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "alife_human_brain.h"
#include "actor.h"
#include "actor_memory.h"
#include "stalker_movement_manager_smart_cover.h"
#include "agent_manager.h"
#include "agent_enemy_manager.h"
#include "agent_member_manager.h"
#include "cover_point.h"
#include "level_graph.h"
#include "cover_point.h"
#include "level_graph.h"
#include "stalker_animation_manager.h"
#include "weapon.h"
#include "ai\stalker\ai_stalker_impl.h"
#include "ai_object_location_impl.h"
#include "script_game_object_impl.h"
#include "smart_cover.h"

using namespace StalkerDecisionSpace;

typedef CStalkerPropertyEvaluator::_value_type _value_type;

const float wounded_enemy_reached_distance = 3.f;

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorALife
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorALife::CStalkerPropertyEvaluatorALife(CAI_Stalker* object, LPCSTR evaluator_name) :
	inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorALife::evaluate()
{
	return (!!ai().get_alife());
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorAlive
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorAlive::CStalkerPropertyEvaluatorAlive(CAI_Stalker* object, LPCSTR evaluator_name) :
	inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorAlive::evaluate()
{
	return(!!object().g_Alive());
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorItems
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorItems::CStalkerPropertyEvaluatorItems(CAI_Stalker* object, LPCSTR evaluator_name) :
	inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorItems::evaluate()
{
	return(!!m_object->memory().ItemManager().selected());
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorEnemies
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorEnemies::CStalkerPropertyEvaluatorEnemies(CAI_Stalker* object, LPCSTR evaluator_name, bool pure_check, u32 time_to_wait, const bool* dont_wait) : 
	inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
	m_time_to_wait	= time_to_wait;
	pdontWaitBool_ = dont_wait;
	pureCheck_ = pure_check;
}

_value_type CStalkerPropertyEvaluatorEnemies::evaluate()
{
	const CEntityAlive* enemy = m_object->memory().EnemyManager().selected();

	if (pureCheck_)
	{
		if (enemy && enemy->g_Alive())
			return true;
		else
			return false;
	}

	if (enemy && enemy->g_Alive())
		return true;

	if (pdontWaitBool_ && *pdontWaitBool_ == true)
		return false;

	if (EngineTimeU() < m_object->memory().EnemyManager().last_enemy_time() + m_time_to_wait)
		return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorSeeEnemy
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorSeeEnemy::CStalkerPropertyEvaluatorSeeEnemy(CAI_Stalker* object, LPCSTR evaluator_name) :
	inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorSeeEnemy::evaluate()
{
	return (m_object->memory().EnemyManager().selected() ? m_object->memory().VisualMManager().visible_now(m_object->memory().EnemyManager().selected()) : false);
}


//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorEnemyLocalized
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorEnemyLocalized::CStalkerPropertyEvaluatorEnemyLocalized(CAI_Stalker* object, LPCSTR evaluator_name, u32 ok_time) :
	inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
	okTime_ = ok_time;
}

_value_type CStalkerPropertyEvaluatorEnemyLocalized::evaluate()
{
	CMemoryManager& mem = m_object->memory();

	if (mem.EnemyManager().selected())
	{
		const CVisibleObject* vis = mem.VisualMManager().GetIfIsInPool(mem.EnemyManager().selected());

		if (vis && vis->m_level_time + okTime_ >= EngineTimeU())
			return true;

		const CSoundObject* snd = mem.SoundMManager().GetIfIsInPool(mem.EnemyManager().selected());

		if (snd && snd->m_level_time + okTime_ >= EngineTimeU())
			return true;

		const CHitObject* hit = mem.HitMManager().GetIfIsInPool(mem.EnemyManager().selected());

		if (hit && hit->m_level_time + okTime_ >= EngineTimeU())
			return true;
	}
	
	return false;
}


//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorKnowWhereEnemy
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorKnowWhereEnemy::CStalkerPropertyEvaluatorKnowWhereEnemy(CAI_Stalker* object, LPCSTR evaluator_name, u32 not_vis_ok_time, u32 not_heard_ok_time) :
inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
	notVisOkTime_ = not_vis_ok_time;
	notHeardOkTime_ = not_heard_ok_time;
}

#include "stalker_combat_planner.h"
#include "stalker_planner.h"

_value_type CStalkerPropertyEvaluatorKnowWhereEnemy::evaluate()
{
	CStalkerCombatPlanner& planner = smart_cast<CStalkerCombatPlanner&>(m_object->brain().current_action());

	if (planner.evaluator(eWorldPropertyEnemyLocalized).evaluate()) // if enemy is locolized, we know where he is
		return true;

	CMemoryManager& mem = m_object->memory();

	if (mem.EnemyManager().selected())
	{
		const CVisibleObject* vis = mem.VisualMManager().GetIfIsInPool(mem.EnemyManager().selected());

		if (vis && vis->m_level_time + notVisOkTime_ >= EngineTimeU())
		{
			const CSoundObject* snd = mem.SoundMManager().GetIfIsInPool(mem.EnemyManager().selected());

			if (snd && snd->m_level_time + notHeardOkTime_ >= EngineTimeU())
				return true;

			const CHitObject* hit = mem.HitMManager().GetIfIsInPool(mem.EnemyManager().selected());

			if (hit && hit->m_level_time + notHeardOkTime_ >= EngineTimeU())
				return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorEnemySeeMe
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorEnemySeeMe::CStalkerPropertyEvaluatorEnemySeeMe(CAI_Stalker* object, LPCSTR evaluator_name) :
	inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorEnemySeeMe::evaluate()
{
	const CEntityAlive* enemy = m_object->memory().EnemyManager().selected();
	if (!enemy)
		return false;

	const CCustomMonster* enemy_monster = smart_cast<const CCustomMonster*>(enemy);

	if (enemy_monster)
		return enemy_monster->memory().VisualMManager().visible_now(m_object);

	const CActor* actor = smart_cast<const CActor*>(enemy);

	if (actor)
		return actor->memory().visual().visible_now(m_object);

	return false;
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorItemToKill
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorItemToKill::CStalkerPropertyEvaluatorItemToKill(CAI_Stalker* object, LPCSTR evaluator_name) :
	inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorItemToKill::evaluate()
{
	return (!!m_object->item_to_kill());
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorItemCanKill
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorItemCanKill::CStalkerPropertyEvaluatorItemCanKill(CAI_Stalker* object, LPCSTR evaluator_name) :
	inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorItemCanKill::evaluate()
{
	return m_object->item_can_kill();
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorFoundItemToKill
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorFoundItemToKill::CStalkerPropertyEvaluatorFoundItemToKill(CAI_Stalker* object, LPCSTR evaluator_name) :
	inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorFoundItemToKill::evaluate()
{
	return m_object->remember_item_to_kill();
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorFoundAmmo
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorFoundAmmo::CStalkerPropertyEvaluatorFoundAmmo(CAI_Stalker* object, LPCSTR evaluator_name) :
	inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorFoundAmmo::evaluate()
{
	return m_object->remember_ammo();
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorReadyToKillSmartCover
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorReadyToKillSmartCover::CStalkerPropertyEvaluatorReadyToKillSmartCover(CAI_Stalker *object, LPCSTR evaluator_name, u32 min_ammo_count) :
	inherited(object, evaluator_name, min_ammo_count)
{
}

_value_type CStalkerPropertyEvaluatorReadyToKillSmartCover::evaluate()
{
	if (m_object->movement().current_params().cover() && !m_object->movement().current_params().cover()->can_fire())
		return		(true);

	return			(inherited::evaluate());
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorReadyToKill
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorReadyToKill::CStalkerPropertyEvaluatorReadyToKill(CAI_Stalker *object, LPCSTR evaluator_name, u32 min_ammo_count) :
	inherited(object ? object->lua_game_object() : 0, evaluator_name),
	m_min_ammo_count(min_ammo_count)
{
}

_value_type CStalkerPropertyEvaluatorReadyToKill::evaluate()
{
	if (!m_object->ready_to_kill())
		return		(false);

	if (!m_min_ammo_count)
		return		(true);

	VERIFY(m_object->best_weapon());
	CWeapon&		best_weapon = smart_cast<CWeapon&>(*m_object->best_weapon());
	if (best_weapon.GetAmmoElapsed() <= (int)m_min_ammo_count) {
		if (best_weapon.GetAmmoMagSize() <= (int)m_min_ammo_count)
			return	(best_weapon.GetState() != CWeapon::eReload);
		else
			return	(false);
	}

	return			(best_weapon.GetState() != CWeapon::eReload);
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorReadyToDetour
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorReadyToDetour::CStalkerPropertyEvaluatorReadyToDetour(CAI_Stalker* object, LPCSTR evaluator_name) :
	inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorReadyToDetour::evaluate()
{
	return m_object->ready_to_detour();
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorAnomaly
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorAnomaly::CStalkerPropertyEvaluatorAnomaly(CAI_Stalker* object, LPCSTR evaluator_name) :
	inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorAnomaly::evaluate()
{
	if (!m_object->undetected_anomaly())
		return false;

	if (!m_object->memory().EnemyManager().selected())
		return true;

	u32 result = dwfChooseAction(2000, m_object->panic_threshold(), 0.f, 0.f, 0.f, m_object->g_Team(), m_object->g_Squad(), m_object->g_Group(), 0, 1, 2, 3, 4, m_object, 300.f);
	
	return !result;
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorInsideAnomaly
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorInsideAnomaly::CStalkerPropertyEvaluatorInsideAnomaly(CAI_Stalker* object, LPCSTR evaluator_name) :
	inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorInsideAnomaly::evaluate()
{
	if (!m_object->inside_anomaly())
		return false;

	if (!m_object->memory().EnemyManager().selected())
		return true;

	u32 result = dwfChooseAction(2000, m_object->panic_threshold(), 0.f, 0.f, 0.f, m_object->g_Team(), m_object->g_Squad(), m_object->g_Group(), 0, 1, 2, 3, 4, m_object, 300.f);
	return !result;
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorPanic
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorPanic::CStalkerPropertyEvaluatorPanic	(CAI_Stalker* object, LPCSTR evaluator_name) :
	inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorPanic::evaluate()
{
	if (object().animation().global_selector())
		return			(false);

	u32 result = dwfChooseAction(2000, m_object->panic_threshold(), 0.f, 0.f, 0.f, m_object->g_Team(), m_object->g_Squad(), m_object->g_Group(), 0, 1, 2, 3, 4, m_object, 300.f);
	return !!result;
}

CStalkerPropertyEvaluatorEnemyToStrong::CStalkerPropertyEvaluatorEnemyToStrong(CAI_Stalker* object, LPCSTR evaluator_name) :
	inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorEnemyToStrong::evaluate()
{
	const CEntityAlive* enemy = object().memory().EnemyManager().selected();

	if (!enemy)
		return false;
	
	if (object().memory().EnemyManager().GetAlliesPower() / object().memory().EnemyManager().GetEnemiesPower() <= 0.25f)
	{
		Msg("! %s Enemies to strong", object().ObjectName().c_str());

		return true;
	}

	return false;
}


//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorSmartTerrainTask
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorSmartTerrainTask::CStalkerPropertyEvaluatorSmartTerrainTask(CAI_Stalker* object, LPCSTR evaluator_name) :
	inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorSmartTerrainTask::evaluate()
{
	if (!ai().get_alife())
		return false;

	CSE_ALifeHumanAbstract* stalker = smart_cast<CSE_ALifeHumanAbstract*>(ai().alife().objects().object(m_object->ID(), true));

	if (!stalker)
		return false;

	VERIFY(stalker);

	stalker->brain().select_task();

	return (stalker->m_smart_terrain_id != 0xffff);
}


//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorEnemyReached
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorEnemyReached::CStalkerPropertyEvaluatorEnemyReached(CAI_Stalker* object, LPCSTR evaluator_name) :
	inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorEnemyReached::evaluate()
{
	const CEntityAlive* enemy = object().memory().EnemyManager().selected();

	if (!enemy)
		return false;

	ALife::_OBJECT_ID processor_id = object().agent_manager().enemy().wounded_processor(enemy);

	if (processor_id != object().ID())
		return false;

	return (object().Position().distance_to_sqr(enemy->Position()) <= _sqr(wounded_enemy_reached_distance));
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorEnemyOnThePath
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorEnemyOnThePath::CStalkerPropertyEvaluatorEnemyOnThePath(CAI_Stalker* object, LPCSTR evaluator_name) :
	inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorEnemyOnThePath::evaluate()
{
	const CEntityAlive* enemy = object().memory().EnemyManager().selected();

	if (!enemy)
		return false;

	if (!m_object->memory().VisualMManager().visible_now(enemy))
		return false;

	return object().movement().is_object_on_the_way(enemy, 2.f);
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorEnemyCriticallyWounded
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorEnemyCriticallyWounded::CStalkerPropertyEvaluatorEnemyCriticallyWounded(CAI_Stalker* object, LPCSTR evaluator_name) :
	inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorEnemyCriticallyWounded::evaluate()
{
	const CEntityAlive* enemy = object().memory().EnemyManager().selected();

	if (!enemy)
		return false;

	const CAI_Stalker* enemy_stalker = smart_cast<const CAI_Stalker*>(enemy);

	if (!enemy_stalker)
		return false;

	return (const_cast<CAI_Stalker*>(enemy_stalker)->critically_wounded());
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorShouldThrowGrenade
//////////////////////////////////////////////////////////////////////////

static float const min_throw_distance		= 10.f;

CStalkerPropertyEvaluatorShouldThrowGrenade::CStalkerPropertyEvaluatorShouldThrowGrenade(CAI_Stalker* object, LPCSTR evaluator_name) :
	inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorShouldThrowGrenade::evaluate()
{
	if (m_storage->property(eWorldPropertyStartedToThrowGrenade))
		return true;

	if (object().last_throw_time() + object().throw_time_interval() >= EngineTimeU())
		return false;

	if (object().inventory().ItemFromSlot(GRENADE_SLOT) == 0)
		return false;

	const CEntityAlive* enemy = object().memory().EnemyManager().selected();

	if (!enemy)
		return false;

	if (!enemy->human_being())
		return false;

	const CMemoryInfo mem_object = object().memory().memory(enemy);

	if (!mem_object.m_object)
		return false;

	if (object().CanHitFriendsWGrenade())
		return false;

	Fvector const& position = mem_object.m_object_params.GetMemorizedPos();
	u32 const& enemy_vertex_id = mem_object.m_object_params.m_level_vertex_id;

	if (object().Position().distance_to_sqr(position) < _sqr(min_throw_distance))
		return false;

	if (!object().agent_manager().member().can_throw_grenade(position))
		return false;

	object().throw_target(position, enemy_vertex_id, const_cast<CEntityAlive*>(enemy));

	if (object().inventory().ItemFromSlot(GRENADE_SLOT) == object().inventory().ActiveItem())
		return true;

	if (!object().throw_enabled())
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorTooFarToKillEnemy
//////////////////////////////////////////////////////////////////////////

extern float FAR_COVER_SEARCH_DIST_K;

CStalkerPropertyEvaluatorTooFarToKillEnemy::CStalkerPropertyEvaluatorTooFarToKillEnemy	(CAI_Stalker* object, LPCSTR evaluator_name) :
	inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorTooFarToKillEnemy::evaluate()
{
	if (!object().memory().EnemyManager().selected())
		return false;

	if (!object().best_weapon())
		return false;

	const CMemoryInfo mem_object = object().memory().memory(object().memory().EnemyManager().selected());

	bool res = object().too_far_to_kill_enemy(mem_object.m_object_params.GetMemorizedPos());

	// if we are getting to cover near enemy
	if (object().brain().initialized() && object().brain().current_action_id() != -1)
	{
		CStalkerCombatPlanner& planner = smart_cast<CStalkerCombatPlanner&>(m_object->brain().current_action());

		if (planner.initialized() && planner.current_action_id() == StalkerDecisionSpace::eWorldOperatorReachFireDist)
		{
			float weapon_fire_dist = 25.f;

			if (object().best_weapon())
			{
				CWeapon* cur_weapon = smart_cast<CWeapon*>(object().best_weapon());

				if (cur_weapon)
					weapon_fire_dist = cur_weapon->GetEffectiveDistance();
			}

			Fvector position = mem_object.m_object_params.GetMemorizedPos();

			// if we are still too far (ex. Enemy fellback)
			bool reached = object().Position().distance_to(position) <= weapon_fire_dist * FAR_COVER_SEARCH_DIST_K;

			res = !reached;
		}
	}

	return res;
}

//////////////////////////////////////////////////////////////////////////
// CStalkerPropertyEvaluatorLowCover
//////////////////////////////////////////////////////////////////////////

CStalkerPropertyEvaluatorLowCover::CStalkerPropertyEvaluatorLowCover(CAI_Stalker *object, LPCSTR evaluator_name) :
	inherited(object ? object->lua_game_object() : 0, evaluator_name)
{
}

_value_type CStalkerPropertyEvaluatorLowCover::evaluate()
{
	return						(false);

#if 0
	if (!m_storage->property(eWorldPropertyInCover))
		return					(false);

	if (!object().memory().enemy().selected())
		return					(false);

	if (!object().best_weapon())
		return					(false);

	CMemoryInfo					mem_object = object().memory().memory(object().memory().enemy().selected());
	const CCoverPoint			*cover = object().best_cover(mem_object.m_object_params.m_position);
	if (!cover)
		return					(false);

	if (object().Position().distance_to_sqr(cover->position()) > .1f)
		return					(false);

	Fvector						direction;
	float						y, p;
	direction.sub(mem_object.m_object_params.m_position, cover->position());
	direction.getHP(y, p);
	float						high_cover_value = ai().level_graph().high_cover_in_direction(y, cover->level_vertex_id());
	float						low_cover_value = ai().level_graph().low_cover_in_direction(y, cover->level_vertex_id());

	if (low_cover_value >= high_cover_value)
		return					(false);

	// should be several other conditions here
	return						(true);
#endif
}