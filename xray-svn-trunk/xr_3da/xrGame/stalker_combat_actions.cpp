
//	Author		: Dmitriy Iassenev
//	Description : Stalker combat action classes

#include "pch_script.h"
#include "stalker_combat_actions.h"
#include "ai/stalker/ai_stalker.h"
#include "script_game_object.h"
#include "stalker_decision_space.h"
#include "inventory.h"
#include "cover_evaluators.h"
#include "cover_point.h"
#include "cover_manager.h"
#include "missile.h"
#include "stalker_movement_restriction.h"
#include "movement_manager_space.h"
#include "detail_path_manager_space.h"
#include "memory_manager.h"
#include "enemy_manager.h"
#include "visual_memory_manager.h"
#include "sight_manager.h"
#include "ai_object_location.h"
#include "stalker_movement_manager_smart_cover.h"
#include "sound_player.h"
#include "stalker_planner.h"
#include "agent_member_manager.h"
#include "agent_location_manager.h"
#include "danger_cover_location.h"
#include "ai/stalker/ai_stalker_space.h"
#include "weapon.h"
#include "danger_manager.h"
#include "detail_path_manager.h"
#include "weaponmagazined.h"
#include "stalker_animation_manager.h"
#include "hit_memory_manager.h"
#include "level_path_manager.h"
#include "level.h"
#include "script_game_object_impl.h"
#define DISABLE_COVER_BEFORE_DETOUR

const float TEMP_DANGER_DISTANCE = 5.f;
const u32	TEMP_DANGER_INTERVAL = 120000;

const float	CLOSE_MOVE_DISTANCE = -10.f;

const u32	CROUCH_LOOK_OUT_DELTA = 5000;
static const u32	s_wait_enemy_in_smart_cover_time	= 30*1000;

using namespace StalkerSpace;
using namespace StalkerDecisionSpace;

typedef CStalkerActionBase::_edge_value_type _edge_value_type;

//#	define SILENT_COMBAT

//////////////////////////////////////////////////////////////////////////
// CStalkerActionGetItemToKill
//////////////////////////////////////////////////////////////////////////

CStalkerActionGetItemToKill::CStalkerActionGetItemToKill(CAI_Stalker* object, LPCSTR action_name) :
inherited(object, action_name)
{
}

void CStalkerActionGetItemToKill::initialize()
{
	inherited::initialize();

	object().sound().remove_active_sounds(u32(eStalkerSoundMaskNoHumming));

	object().sight().setup(CSightAction(object().m_best_found_item_to_kill ? &object().m_best_found_item_to_kill->object() : 0, true));

	object().movement().set_mental_state(eMentalStateDanger);
}

void CStalkerActionGetItemToKill::finalize()
{
	inherited::finalize();

	object().sight().clear();
	object().sight().setup(CSightAction(SightManager::eSightTypePathDirection, false));

	if (!object().g_Alive())
		return;

	object().sound().set_sound_mask(0);
}

void CStalkerActionGetItemToKill::execute()
{
	inherited::execute();

	if (!object().m_best_found_item_to_kill)
		return;

	object().movement().set_level_dest_vertex(object().m_best_found_item_to_kill->object().ai_location().level_vertex_id());
	object().movement().set_desired_position(&object().m_best_found_item_to_kill->object().Position());
	object().movement().set_desired_direction(0);
	object().movement().set_path_type(MovementManager::ePathTypeLevelPath);
	object().movement().set_detail_path_type(DetailPathManager::eDetailPathTypeSmooth);
	object().movement().set_body_state(object().movement().body_state() == eBodyStateCrouch ? eBodyStateCrouch : eBodyStateStand);
	object().movement().set_movement_type(eMovementTypeWalk);

	object().set_goal(eObjectActionIdle);
}


//////////////////////////////////////////////////////////////////////////
// CStalkerActionMakeItemKilling
//////////////////////////////////////////////////////////////////////////

CStalkerActionMakeItemKilling::CStalkerActionMakeItemKilling(CAI_Stalker* object, LPCSTR action_name) :
inherited(object, action_name)
{
}

void CStalkerActionMakeItemKilling::initialize()
{
	inherited::initialize();

	object().sound().remove_active_sounds(u32(eStalkerSoundMaskNoHumming));

	object().sight().clear();
	object().sight().add_action(eSightActionTypeWatchItem, xr_new <CSightControlAction>(1.f, 3000, CSightAction(SightManager::eSightTypePathDirection)));
	object().sight().add_action(eSightActionTypeWatchEnemy, xr_new <CSightControlAction>(1.f, 3000, CSightAction(SightManager::eSightTypePosition, object().memory().EnemyManager().selected()->Position(), false)));

	object().movement().set_mental_state(eMentalStateDanger);
}

void CStalkerActionMakeItemKilling::finalize()
{
	inherited::finalize();

	object().sight().clear();
	object().sight().setup(CSightAction(SightManager::eSightTypePathDirection, false));

	if (!object().g_Alive())
		return;

	object().sound().set_sound_mask(0);
}

void CStalkerActionMakeItemKilling::execute()
{
	inherited::execute();

	if (!object().m_best_found_ammo)
		return;

	object().movement().set_level_dest_vertex(object().m_best_found_ammo->object().ai_location().level_vertex_id());
	object().movement().set_desired_position(&object().m_best_found_ammo->object().Position());
	object().movement().set_desired_direction(0);
	object().movement().set_path_type(MovementManager::ePathTypeLevelPath);
	object().movement().set_detail_path_type(DetailPathManager::eDetailPathTypeSmooth);
	object().movement().set_body_state(eBodyStateStand);
	object().movement().set_movement_type(eMovementTypeWalk);

	object().sight().action(eSightActionTypeWatchEnemy).set_vector3d(object().memory().EnemyManager().selected()->Position());
	object().set_goal(eObjectActionIdle);
}


//////////////////////////////////////////////////////////////////////////
// CStalkerActionRetreatFromEnemy
//////////////////////////////////////////////////////////////////////////

CStalkerActionRetreatFromEnemy::CStalkerActionRetreatFromEnemy(CAI_Stalker* object, LPCSTR action_name) :
inherited(object, action_name)
{
}

void CStalkerActionRetreatFromEnemy::initialize()
{
	inherited::initialize();
}

void CStalkerActionRetreatFromEnemy::finalize()
{
	inherited::finalize();

	if (!object().g_Alive())
		return;

	object().sound().set_sound_mask(0);
}

void CStalkerActionRetreatFromEnemy::execute()
{
	inherited::execute();

	if (!object().memory().EnemyManager().selected())
		return;

	const CMemoryInfo mem_object = object().memory().memory(object().memory().EnemyManager().selected());

	object().movement().set_movement_type(eMovementTypeRun);
	object().movement().set_path_type(MovementManager::ePathTypeLevelPath);
	object().movement().set_detail_path_type(DetailPathManager::eDetailPathTypeSmooth);
	object().movement().set_mental_state(eMentalStatePanic);
	object().movement().set_body_state(eBodyStateStand);

	CCoverPoint const* point = nullptr;

	if (mem_object.m_object)
	{
		object().m_ce_far->setup(mem_object.m_object_params.GetMemorizedPos(), 0.f, 300.f);

		const CCoverPoint* point = ai().cover_manager().best_cover(object().Position(), 30.f, *object().m_ce_far, CStalkerMovementRestrictor(m_object, true));

		if (!point)
		{
			object().m_ce_far->setup(mem_object.m_object_params.GetMemorizedPos(), 0.f, 300.f);
			point = ai().cover_manager().best_cover(object().Position(), 50.f, *object().m_ce_far, CStalkerMovementRestrictor(m_object, true));
		}
	}

	if (point)
	{
		object().movement().set_level_dest_vertex(point->level_vertex_id());
		object().movement().set_desired_position(&point->position());
		object().CObjectHandler::set_goal(eObjectActionIdle);
		object().sight().setup(CSightAction(SightManager::eSightTypePathDirection, false));
	}
	else
	{
		if (object().memory().VisualMManager().visible_now(object().memory().EnemyManager().selected()))
		{
			object().movement().set_mental_state(eMentalStateDanger);
			//			u32												min_queue_size, max_queue_size, min_queue_interval, max_queue_interval;
			//			float											distance = object().memory().enemy().selected()->Position().distance_to(object().Position());
			//			select_queue_params								(distance,min_queue_size, max_queue_size, min_queue_interval, max_queue_interval);
			//			object().CObjectHandler::set_goal				(eObjectActionFire1,object().best_weapon(),min_queue_size, max_queue_size, min_queue_interval, max_queue_interval);

			if (fire_make_sense())
				fire();
			else
				aim_ready();

			object().sight().setup(CSightAction(object().memory().EnemyManager().selected(), true, true));
		}
		else
		{
			object().CObjectHandler::set_goal(eObjectActionIdle);
			object().sight().setup(CSightAction(SightManager::eSightTypeCover, true));
		}
	}

#ifndef SILENT_COMBAT
	play_panic_sound(0, 0, 10000);
#endif
}

_edge_value_type CStalkerActionRetreatFromEnemy::weight(const CSConditionState& condition0, const CSConditionState& condition1) const
{
	return(_edge_value_type(100));
}


//////////////////////////////////////////////////////////////////////////
// CStalkerActionGetReadyToKill
//////////////////////////////////////////////////////////////////////////

CStalkerActionGetReadyToKill::CStalkerActionGetReadyToKill(bool affect_properties, CAI_Stalker* object, LPCSTR action_name) :
inherited(object, action_name)
{
	m_affect_properties = affect_properties;
}

void CStalkerActionGetReadyToKill::initialize()
{
	inherited::initialize();

	m_body_state = object().movement().body_state();
	//	m_movement_type										= Random.randI(2) ? eMovementTypeRun : eMovementTypeWalk;
	//	m_movement_type										= eMovementTypeRun;

	object().movement().set_desired_direction(0);
	object().movement().set_path_type(MovementManager::ePathTypeLevelPath);
	object().movement().set_detail_path_type(DetailPathManager::eDetailPathTypeSmooth);
	object().movement().set_nearest_accessible_position();
	object().movement().set_mental_state(eMentalStateDanger);
	object().movement().set_body_state(m_body_state);
	//	object().movement().set_movement_type				(eMovementTypeRun);
	//	object().sight().setup								(CSightAction(SightManager::eSightTypePathDirection));

	if (m_affect_properties)
	{
		m_storage->set_property(eWorldPropertyInCover, false);
		m_storage->set_property(eWorldPropertyLookedOut, false);
		m_storage->set_property(eWorldPropertyPositionHolded, false);
		m_storage->set_property(eWorldPropertyEnemyDetoured, false);

		m_enable_enemy_change = object().memory().EnemyManager().enable_enemy_change();
		object().memory().EnemyManager().enable_enemy_change(false);
	}
}

void CStalkerActionGetReadyToKill::finalize()
{
	inherited::finalize();

	if (m_affect_properties)
		object().memory().EnemyManager().enable_enemy_change(m_enable_enemy_change);
}

void CStalkerActionGetReadyToKill::execute()
{
	inherited::execute();

	if (!object().m_best_item_to_kill)
		return;

	if (!object().memory().EnemyManager().selected())
		return;

	if (object().movement().detail().distance_to_target() < 2.f)
	{
		object().movement().set_movement_type(eMovementTypeWalk);
		object().sight().setup(CSightAction(SightManager::eSightTypeCurrentDirection));
	}
	else
	{
		object().movement().set_movement_type(eMovementTypeRun);
		object().sight().setup(CSightAction(SightManager::eSightTypePathDirection));
	}

	if (object().movement().detail().distance_to_target() > CLOSE_MOVE_DISTANCE)
		object().movement().set_body_state(eBodyStateStand);
	//	else {
	//		object().movement().set_movement_type(m_movement_type);
	//	}

	const CMemoryInfo mem_object = object().memory().memory(object().memory().EnemyManager().selected());
	Fvector position = mem_object.m_object_params.GetMemorizedPos();

	float weapon_fire_dist = 25.f;

	if (object().best_weapon())
	{
		CWeapon* cur_weapon = smart_cast<CWeapon*>(object().best_weapon());

		if (cur_weapon)
			weapon_fire_dist = cur_weapon->GetEffectiveDistance();
	}

	//object().m_ce_ambush->setup(object().Position(), position, 5.f);
	//const CCoverPoint* point = ai().cover_manager().best_cover(position, _max(15.f, weapon_fire_dist + 10.f), *object().m_ce_ambush, CStalkerMovementRestrictor(m_object, true));

	if (mem_object.m_object)
	{
		CCoverPoint const* const point = object().best_cover(position);

		if (point)
		{
			setup_cover(*point);
			//		object().movement().set_movement_type	(eMovementTypeRun);
			if (object().movement().path_completed() || object().Position().distance_to(point->position()) < 1.f) {
				//			object().movement().set_body_state	(eBodyStateCrouch);
				object().brain().affect_cover(true);
			}
			else
			{
				//			object().movement().set_body_state	(eBodyStateStand);
				object().brain().affect_cover(false);
			}
		}
		else
		{
			object().brain().affect_cover(true);
			object().movement().set_movement_type(eMovementTypeStand);
			//		object().movement().set_body_state		(eBodyStateCrouch);
			object().movement().set_nearest_accessible_position();
		}
	}

	//	if (object().memory().visual().visible_now(object().memory().enemy().selected()))
	//		object().sight().setup	(CSightAction(object().memory().enemy().selected(),true));
	//	else
	//		object().sight().setup	(CSightAction(SightManager::eSightTypePosition,mem_object.m_object_params.m_position,true));

	//	if ((point && !point->position().similar(object().Position(),.5f)) || !object().movement().path_completed())
	//		object().sight().setup			(CSightAction(SightManager::eSightTypePathDirection));

	if (m_affect_properties)
		aim_ready();
	else
		aim_ready_force_full();

	if (object().movement().path_completed())
		object().best_cover_can_try_advance();
}


//////////////////////////////////////////////////////////////////////////
// CStalkerActionKillEnemy
//////////////////////////////////////////////////////////////////////////

CStalkerActionKillEnemy::CStalkerActionKillEnemy(CAI_Stalker* object, LPCSTR action_name) :
inherited(object, action_name)
{
}

void CStalkerActionKillEnemy::initialize()
{
	inherited::initialize();

	object().movement().set_desired_direction(0);
	object().movement().set_path_type(MovementManager::ePathTypeLevelPath);
	object().movement().set_detail_path_type(DetailPathManager::eDetailPathTypeSmooth);
	object().movement().set_nearest_accessible_position();
	object().movement().set_mental_state(eMentalStateDanger);
	//	object().movement().set_body_state			(m_storage->property(eWorldPropertyUseCrouchToLookOut) ? eBodyStateCrouch : eBodyStateStand);
	object().movement().set_movement_type(eMovementTypeStand);

#ifndef SILENT_COMBAT
	play_attack_sound(0, 0, 6000, 4000);
#endif

	object().brain().affect_cover(true);
}

void CStalkerActionKillEnemy::finalize()
{
	inherited::finalize();
}

void CStalkerActionKillEnemy::execute()
{
	inherited::execute();

	if (object().memory().VisualMManager().visible_now(object().memory().EnemyManager().selected()))
	{
		object().sight().setup(CSightAction(object().memory().EnemyManager().selected(), true, true));
	}
	else if (object().memory().EnemyManager().CanFireAtEnemyBehindCover())
	{
		object().sight().setup(CSightAction(SightManager::eSightTypePosition, object().memory().EnemyManager().PossibleEnemyLookOutPos(), true));
	}
	else
	{
		object().sight().setup(CSightAction(SightManager::eSightTypePosition, object().memory().EnemyManager().EnemyPosInMemory(), true));
	}

	if (fire_make_sense() || !object().can_kill_member() && object().memory().EnemyManager().CanFireAtEnemyBehindCover())
		fire();
	else
		aim_ready();

	if (object().memory().EnemyManager().selected())
	{
		const CMemoryInfo mem_object = object().memory().memory(object().memory().EnemyManager().selected());

		if (mem_object.m_object)
		{
			object().best_cover(mem_object.m_object_params.GetMemorizedPos());
		}
	}
}


//////////////////////////////////////////////////////////////////////////
// CStalkerActionTakeCover
//////////////////////////////////////////////////////////////////////////

CStalkerActionTakeCover::CStalkerActionTakeCover(CAI_Stalker* object, LPCSTR action_name) :
inherited(object, action_name)
{
}

void CStalkerActionTakeCover::initialize()
{
	inherited::initialize();

	m_body_state = object().movement().body_state();
	//m_movement_type = Random.randI(2) ? eMovementTypeRun : eMovementTypeWalk;
	m_movement_type = eMovementTypeWalk;

	object().movement().set_desired_direction(0);
	object().movement().set_path_type(MovementManager::ePathTypeLevelPath);
	object().movement().set_detail_path_type(DetailPathManager::eDetailPathTypeSmooth);
	object().movement().set_mental_state(eMentalStateDanger);
	object().movement().set_body_state(m_body_state);
	object().movement().set_movement_type(m_movement_type);

	m_storage->set_property(eWorldPropertyLookedOut, false);
	m_storage->set_property(eWorldPropertyPositionHolded, false);
	m_storage->set_property(eWorldPropertyEnemyDetoured, false);

#ifndef SILENT_COMBAT
	if (object().memory().EnemyManager().selected()->human_being())
	{
		if (object().agent_manager().member().can_cry_noninfo_phrase())
			if (object().Position().distance_to_sqr(object().memory().EnemyManager().selected()->Position()) < _sqr(10.f))
				if (object().memory().VisualMManager().visible_now(object().memory().EnemyManager().selected()) && object().memory().EnemyManager().AlliesCount() > 0)
					object().sound().play(eStalkerSoundBackup, 0, 0, 6000, 4000);
	}
#endif
}

void CStalkerActionTakeCover::finalize()
{
	inherited::finalize();
}

void CStalkerActionTakeCover::execute()
{
	inherited::execute();

	const CMemoryInfo mem_object = object().memory().memory(object().memory().EnemyManager().selected());

	if (!mem_object.m_object)
		return;

	if (object().movement().detail().distance_to_target() > CLOSE_MOVE_DISTANCE)
		object().movement().set_body_state(eBodyStateStand);
	else
		object().movement().set_movement_type(m_movement_type);

	Fvector position = mem_object.m_object_params.GetMemorizedPos();

	float weapon_fire_dist = 25.f;

	if (object().best_weapon())
	{
		CWeapon* cur_weapon = smart_cast<CWeapon*>(object().best_weapon());

		if (cur_weapon)
			weapon_fire_dist = cur_weapon->GetEffectiveDistance();
	}

	const CCoverPoint* point = object().best_cover(position);

	if (point)
	{
		setup_cover(*point);

		if (object().movement().path_completed() && object().Position().distance_to(point->position()) < 1.f)
			object().brain().affect_cover(true);
		else
			object().brain().affect_cover(false);
	}
	else
	{
		object().movement().set_nearest_accessible_position();
		object().brain().affect_cover(true);
	}

	if (fire_make_sense())
	{
		fire();
	}
	else
	{
		aim_ready();
	}

	if (object().movement().path_completed())
	{// && (object().memory().enemy().selected()->Position().distance_to_sqr(object().Position()) >= 10.f))
		object().best_cover_can_try_advance();
		m_storage->set_property(eWorldPropertyInCover, true);
	}

	if (object().memory().VisualMManager().visible_now(object().memory().EnemyManager().selected()))
		object().sight().setup(CSightAction(object().memory().EnemyManager().selected(), true, true));
	else
		object().sight().setup(CSightAction(SightManager::eSightTypePosition, mem_object.m_object_params.GetMemorizedPos(), true));
}


//////////////////////////////////////////////////////////////////////////
// CStalkerActionLookOut
//////////////////////////////////////////////////////////////////////////

#define TRY_CHANGE_MIND_LOOK_OUT_STATE_TIME_MIN 25000
#define TRY_CHANGE_MIND_LOOK_OUT_STATE_TIME_MAX 60000

CStalkerActionLookOut::CStalkerActionLookOut(CAI_Stalker* object, LPCSTR action_name) :
inherited(object, action_name)
{
	m_last_change_time = 0;
	waitInLookOutState_ = Random.randI(TRY_CHANGE_MIND_LOOK_OUT_STATE_TIME_MIN, TRY_CHANGE_MIND_LOOK_OUT_STATE_TIME_MAX);

	timeLookOutStateStarted_ = EngineTimeU();
}

void CStalkerActionLookOut::initialize()
{
	inherited::initialize();

	if (EngineTimeU() >= m_last_change_time + CROUCH_LOOK_OUT_DELTA)
	{
		m_storage->set_property(eWorldPropertyUseCrouchToLookOut, Random.randI(0, 2) == 1);
		m_last_change_time = EngineTimeU();
	}

	object().movement().set_desired_direction(0);
	object().movement().set_path_type(MovementManager::ePathTypeLevelPath);
	object().movement().set_detail_path_type(DetailPathManager::eDetailPathTypeSmooth);
	object().movement().set_mental_state(eMentalStateDanger);

	object().movement().set_body_state(m_storage->property(eWorldPropertyUseCrouchToLookOut) ? eBodyStateCrouch : eBodyStateStand);
	object().movement().set_movement_type(eMovementTypeWalk);
	object().movement().set_nearest_accessible_position();

	if (object().ready_to_detour())
		aim_ready();
	else
	{
		aim_ready_force_full();
		object().movement().set_movement_type(eMovementTypeStand);
	}

	set_inertia_time(1000);
	object().brain().affect_cover(true);

	timeLookOutStateStarted_ = EngineTimeU();
}

float current_cover(CAI_Stalker* object)
{
	Fvector								position, direction;
	position = object->eye_matrix.c;
	direction = object->eye_matrix.k;
	collide::rq_result					ray_query_result;

	BOOL result = Level().ObjectSpace.RayPick(position, direction, 10.f, collide::rqtStatic, ray_query_result, NULL);

	if (!result)
		return(100.f);

	return(ray_query_result.range);
}

void CStalkerActionLookOut::finalize()
{
	inherited::finalize();
}

void CStalkerActionLookOut::execute()
{
	inherited::execute();

	const CMemoryInfo mem_object = object().memory().memory(object().memory().EnemyManager().selected());

	if (!mem_object.m_object)
		return;

	object().sight().setup(CSightAction(SightManager::eSightTypePosition, mem_object.m_object_params.GetMemorizedPos(), true));

	if (current_cover(m_object) >= 3.f)
	{
		object().movement().set_nearest_accessible_position();
		m_storage->set_property(eWorldPropertyLookedOut, true);

		return;
	}

	Fvector position = mem_object.m_object_params.GetMemorizedPos();
	object().m_ce_close->setup(position, 0.f, 170.f, 10.f);

	const CCoverPoint* point = ai().cover_manager().best_cover(object().Position(), 10.f, *object().m_ce_close);//,CStalkerMovementRestrictor(m_object,true,false));

	if (!point || (point->position().similar(object().Position()) && object().movement().path_completed()))
	{
		object().m_ce_close->setup(position, 0.f, 170.f, 10.f);
		point = ai().cover_manager().best_cover(object().Position(), 30.f, *object().m_ce_close);//,CStalkerMovementRestrictor(m_object,true,false));
	}

	if (point)
	{
		object().movement().set_level_dest_vertex(point->level_vertex_id());
		object().movement().set_desired_position(&point->position());
	}
	else
		object().movement().set_nearest_accessible_position();

	//	if (point && point->position().similar(object().Position(),.5f) && object().movement().path_completed()) {
	//		m_storage->set_property			(eWorldPropertyLookedOut,true);
	//		object().movement().set_nearest_accessible_position	();
	//	}

	if (mem_object.m_object)
	{
		object().best_cover(mem_object.m_object_params.GetMemorizedPos());
	}

	if (timeLookOutStateStarted_ + waitInLookOutState_ < EngineTimeU())
	{
		object().PokeNPCInFight();

		timeLookOutStateStarted_ = EngineTimeU();
	}
}


//////////////////////////////////////////////////////////////////////////
// CStalkerActionHoldPosition
//////////////////////////////////////////////////////////////////////////

CStalkerActionHoldPosition::CStalkerActionHoldPosition(CAI_Stalker* object, LPCSTR action_name) :
inherited(object, action_name)
{
}

void CStalkerActionHoldPosition::initialize()
{
	inherited::initialize();

	object().movement().set_desired_direction(0);
	object().movement().set_path_type(MovementManager::ePathTypeLevelPath);
	object().movement().set_detail_path_type(DetailPathManager::eDetailPathTypeSmooth);
	object().movement().set_nearest_accessible_position();
	object().movement().set_mental_state(eMentalStateDanger);
	object().movement().set_body_state(m_storage->property(eWorldPropertyUseCrouchToLookOut) ? eBodyStateCrouch : eBodyStateStand);
	object().movement().set_movement_type(eMovementTypeStand);

	aim_ready();

	set_inertia_time(1000 + ::Random32.random(2000));
	object().brain().affect_cover(true);
}

void CStalkerActionHoldPosition::finalize()
{
	inherited::finalize();
}

void CStalkerActionHoldPosition::execute()
{
	inherited::execute();

	const CMemoryInfo mem_object = object().memory().memory(object().memory().EnemyManager().selected());

	if (!mem_object.m_object)
		return;

	if (current_cover(m_object) < 3.f)
		m_storage->set_property(eWorldPropertyLookedOut, false);

	object().sight().setup(CSightAction(SightManager::eSightTypePosition, mem_object.m_object_params.GetMemorizedPos(), true));

	if (completed())
	{
		if (object().agent_manager().member().can_detour() || !object().agent_manager().member().cover_detouring() || !fire_make_sense())
		{
			m_storage->set_property(eWorldPropertyPositionHolded, true);
		}
	}

	if (fire_make_sense())
	{
		if (object().agent_manager().member().cover_detouring())
			object().sound().play(eStalkerSoundNeedBackup, 3000, 3000, 10000, 10000);

		fire();
	}
	else
	{
		aim_ready();
	}

	if (mem_object.m_object)
	{
		object().best_cover(mem_object.m_object_params.GetMemorizedPos());
	}
}


//////////////////////////////////////////////////////////////////////////
// CStalkerActionDetourEnemy
//////////////////////////////////////////////////////////////////////////

CStalkerActionDetourEnemy::CStalkerActionDetourEnemy(CAI_Stalker* object, LPCSTR action_name) :
inherited(object, action_name)
{
}

void CStalkerActionDetourEnemy::initialize()
{
	inherited::initialize();

	object().agent_manager().member().member(&object()).detour(true);
	object().movement().set_desired_direction(0);
	object().movement().set_path_type(MovementManager::ePathTypeLevelPath);
	object().movement().set_detail_path_type(DetailPathManager::eDetailPathTypeSmooth);
	object().movement().set_mental_state(eMentalStateDanger);
	object().movement().set_body_state(eBodyStateStand);
	object().movement().set_movement_type(eMovementTypeRun);

	aim_ready();

#ifdef DISABLE_COVER_BEFORE_DETOUR
	if (/**(Random.randF(1.f) < .8f) && /**/object().agent_manager().member().member(m_object).cover())
		object().agent_manager().location().add(xr_new <CDangerCoverLocation>(object().agent_manager().member().member(m_object).cover(), EngineTimeU(), TEMP_DANGER_INTERVAL, TEMP_DANGER_DISTANCE, object().agent_manager().member().mask(&object())));
#endif

	object().agent_manager().member().member(m_object).cover(0);

#ifndef SILENT_COMBAT
	if (object().memory().EnemyManager().selected()->human_being() && object().memory().EnemyManager().AlliesCount() > 0)
		object().sound().play(eStalkerSoundDetour);
#endif
}

void CStalkerActionDetourEnemy::finalize()
{
	inherited::finalize();

	if (object().g_Alive())
		object().agent_manager().member().member(&object()).detour(false);
}

void CStalkerActionDetourEnemy::execute()
{
	inherited::execute();

	const CMemoryInfo mem_object = object().memory().memory(object().memory().EnemyManager().selected());

	if (!mem_object.m_object)
		return;

	if (object().movement().path_completed())
	{
		Fvector position = mem_object.m_object_params.GetMemorizedPos();

		object().m_ce_angle->setup(position, 10.f, object().GetEyeRangeValue(), mem_object.m_object_params.m_level_vertex_id);

		const CCoverPoint* point = ai().cover_manager().best_cover(object().Position(), 10.f, *object().m_ce_angle, CStalkerMovementRestrictor(m_object, true));

		if (!point)
		{
			object().m_ce_angle->setup(position, 10.f, object().GetEyeRangeValue(), mem_object.m_object_params.m_level_vertex_id);
			point = ai().cover_manager().best_cover(object().Position(), 30.f, *object().m_ce_angle, CStalkerMovementRestrictor(m_object, true));
		}

		if (point)
		{
			object().movement().set_level_dest_vertex(point->level_vertex_id());
			object().movement().set_desired_position(&point->position());
		}
		else
			object().movement().set_nearest_accessible_position();

		if (object().movement().path_completed())
			m_storage->set_property(eWorldPropertyEnemyDetoured, true);
	}

	object().sight().setup(CSightAction(SightManager::eSightTypePosition, mem_object.m_object_params.GetMemorizedPos(), true));
}


//////////////////////////////////////////////////////////////////////////
// CStalkerActionPostCombatWait
//////////////////////////////////////////////////////////////////////////

CStalkerActionPostCombatWait::CStalkerActionPostCombatWait(CAI_Stalker* object, LPCSTR action_name) :
inherited(object, action_name)
{
}

void CStalkerActionPostCombatWait::initialize()
{
	inherited::initialize();

	object().movement().set_movement_type(eMovementTypeStand);

	EObjectAction action = eObjectActionAimReady1;

	if (m_storage->property(eWorldPropertyKilledWounded))
		action = eObjectActionIdle;

	if (object().inventory().ActiveItem() && object().best_weapon() && (object().inventory().ActiveItem()->object().ID() == object().best_weapon()->object().ID()))
		object().set_goal	(action,object().best_weapon());
	else {
		if (object().inventory().ItemFromSlot(RIFLE_SLOT)) {
			CWeaponMagazined				*temp = smart_cast<CWeaponMagazined*>(object().inventory().ItemFromSlot(RIFLE_SLOT));
			if (object().inventory().ActiveItem() && temp && (object().inventory().ActiveItem()->object().ID() == temp->ID()))
				object().set_goal			(action,object().inventory().ItemFromSlot(RIFLE_SLOT));
		}
	}

	if (m_storage->property(eWorldPropertyKilledWounded))
		return;

	if (object().memory().EnemyManager().last_enemy() && object().memory().VisualMManager().visible_now(object().memory().EnemyManager().last_enemy()))
		object().sight().setup(CSightAction(object().memory().EnemyManager().last_enemy(), true, true));
}

void CStalkerActionPostCombatWait::execute()
{
	inherited::execute();
}

void CStalkerActionPostCombatWait::finalize()
{
	inherited::finalize();
}


//////////////////////////////////////////////////////////////////////////
// CStalkerActionHideFromGrenade
//////////////////////////////////////////////////////////////////////////

CStalkerActionHideFromGrenade::CStalkerActionHideFromGrenade(CAI_Stalker* object, LPCSTR action_name) :
inherited(object, action_name)
{
}

void CStalkerActionHideFromGrenade::initialize()
{
	inherited::initialize();

	m_storage->set_property(eWorldPropertyUseSuddenness, false);

	object().movement().set_desired_direction(0);
	object().movement().set_path_type(MovementManager::ePathTypeLevelPath);
	object().movement().set_detail_path_type(DetailPathManager::eDetailPathTypeSmooth);
	object().movement().set_mental_state(eMentalStateDanger);
	object().movement().set_body_state(eBodyStateStand);
	object().movement().set_movement_type(eMovementTypeRun);

	object().m_ce_best->invalidate();

	m_storage->set_property(eWorldPropertyInCover, false);
	m_storage->set_property(eWorldPropertyLookedOut, false);
	m_storage->set_property(eWorldPropertyPositionHolded, false);
	m_storage->set_property(eWorldPropertyEnemyDetoured, false);
}

void CStalkerActionHideFromGrenade::execute()
{
	inherited::execute();

	if (!object().memory().DangerManager().selected())
		return;

	Fvector position = object().memory().DangerManager().selected()->position();
	const CCoverPoint* point = object().best_cover(position);

	if (point)
	{
		setup_cover(*point);
		object().movement().set_movement_type(eMovementTypeRun);
	}
	else
	{
		object().movement().set_movement_type(eMovementTypeStand);
		object().movement().set_body_state(eBodyStateCrouch);
	}

	if (!object().memory().EnemyManager().selected())
		object().sight().setup(CSightAction(SightManager::eSightTypePathDirection, true, true));
	else
	{
		const CMemoryInfo mem_object = object().memory().memory(object().memory().EnemyManager().selected());

		if (!mem_object.m_object)
		{
			object().sight().setup(CSightAction(SightManager::eSightTypePathDirection, true, true));
			aim_ready();
		}
		else
		{
			if (!m_object->memory().VisualMManager().visible_now(object().memory().EnemyManager().selected()))
			{
				if (position.distance_to_sqr(object().Position()) < _sqr(5.f))
					object().sight().setup(CSightAction(SightManager::eSightTypePathDirection, true, true));
				else
					object().sight().setup(CSightAction(SightManager::eSightTypePosition, mem_object.m_object_params.GetMemorizedPos(), true));

				aim_ready();
			}
			else
			{
				object().sight().setup(CSightAction(object().memory().EnemyManager().selected(), true, true));
				
				if (fire_make_sense())
					fire();
			}
		}
	}

	if (object().movement().path_completed())
		object().movement().set_body_state(eBodyStateCrouch);
}

void CStalkerActionHideFromGrenade::finalize()
{
	inherited::finalize();
}


//////////////////////////////////////////////////////////////////////////
// CStalkerActionSuddenAttack
//////////////////////////////////////////////////////////////////////////

CStalkerActionSuddenAttack::CStalkerActionSuddenAttack(CAI_Stalker *object, LPCSTR action_name) :
inherited(object, action_name)
{
}

void CStalkerActionSuddenAttack::initialize()
{
	inherited::initialize();

	object().movement().set_desired_direction(0);
	object().movement().set_path_type(MovementManager::ePathTypeLevelPath);
	object().movement().set_detail_path_type(DetailPathManager::eDetailPathTypeSmooth);
	object().movement().set_mental_state(eMentalStateDanger);

	if (!object().memory().EnemyManager().selected())
		return;

	aim_ready();
}

void CStalkerActionSuddenAttack::finalize()
{
	inherited::finalize();
}

void CStalkerActionSuddenAttack::execute()
{
	inherited::execute();

	if (object().agent_manager().member().combat_members().size() > 1)
		m_storage->set_property(eWorldPropertyUseSuddenness, false);

	if (!object().memory().EnemyManager().selected())
		return;

	const CMemoryInfo mem_object = object().memory().memory(object().memory().EnemyManager().selected());

	if (!mem_object.m_object)
		return;

	bool visible_now = object().memory().VisualMManager().visible_now(object().memory().EnemyManager().selected());

	if (visible_now)
		object().sight().setup(CSightAction(object().memory().EnemyManager().selected(), true));
	else
		object().sight().setup(CSightAction(SightManager::eSightTypePosition, mem_object.m_object_params.GetMemorizedPos(), true));

	if (object().movement().accessible(mem_object.m_object_params.m_level_vertex_id))
		object().movement().set_level_dest_vertex(mem_object.m_object_params.m_level_vertex_id);
	else
		object().movement().set_nearest_accessible_position(ai().level_graph().vertex_position(mem_object.m_object_params.m_level_vertex_id), mem_object.m_object_params.m_level_vertex_id);

	if (!visible_now)
	{
		u32 target_vertex_id = object().movement().level_path().dest_vertex_id();

		if (object().ai_location().level_vertex_id() == target_vertex_id)
		{
			m_storage->set_property(eWorldPropertyUseSuddenness, false);

			return;
		}
	}

	float distance = object().Position().distance_to(mem_object.m_object_params.GetMemorizedPos());

	if (distance >= 15.f)
	{
		object().movement().set_body_state(eBodyStateStand);
		object().movement().set_movement_type(eMovementTypeRun);
	}
	else
	{
		if (distance >= 8.f)
		{
			object().movement().set_body_state(eBodyStateStand);
			object().movement().set_movement_type(eMovementTypeWalk);
		}
		else
		{
			if (distance >= 6.f)
			{
				object().movement().set_body_state(eBodyStateCrouch);
				object().movement().set_movement_type(eMovementTypeRun);
			}
			else
			{
				if ((distance >= 4.f) || !visible_now)
				{
					object().movement().set_body_state(eBodyStateCrouch);
					object().movement().set_movement_type(eMovementTypeRun);
				}
				else
				{
					object().movement().set_body_state(eBodyStateCrouch);
					object().movement().set_movement_type(eMovementTypeStand);

					if (object().memory().EnemyManager().selected()->g_Alive() && fire_make_sense())
						fire();
				}
			}
		}
	}

	CVisualMemoryManager* visual_memory_manager = object().memory().EnemyManager().selected()->visual_memory();

	VERIFY(visual_memory_manager);

	if (!visual_memory_manager->visible_now(&object()))
		return;

	m_storage->set_property(eWorldPropertyUseSuddenness, false);
}


//////////////////////////////////////////////////////////////////////////
// CStalkerActionKillEnemyOnThePath
//////////////////////////////////////////////////////////////////////////

CStalkerActionKillEnemyOnThePath::CStalkerActionKillEnemyOnThePath(CAI_Stalker* object, LPCSTR action_name) :
inherited(object, action_name)
{
}

void CStalkerActionKillEnemyOnThePath::initialize()
{
	inherited::initialize();

	object().movement().set_mental_state(eMentalStateDanger);
	object().movement().set_movement_type(eMovementTypeStand);
	object().movement().force_update(true);

	m_storage->set_property(eWorldPropertyInCover, false);
	m_storage->set_property(eWorldPropertyLookedOut, false);
	m_storage->set_property(eWorldPropertyPositionHolded, false);
	m_storage->set_property(eWorldPropertyEnemyDetoured, false);

	play_attack_sound(0, 0, 6000, 4000);

	object().brain().affect_cover(true);
}

void CStalkerActionKillEnemyOnThePath::finalize()
{
	inherited::finalize();

	object().movement().force_update(false);
}

void CStalkerActionKillEnemyOnThePath::execute()
{
	inherited::execute();

	object().sight().setup(CSightAction(object().memory().EnemyManager().selected(), true, true));

	if (fire_make_sense())
		fire();

	const CMemoryInfo mem_object = object().memory().memory(object().memory().EnemyManager().selected());
	Fvector position = mem_object.m_object_params.GetMemorizedPos();

	const CCoverPoint* point = object().best_cover(position);

	if (!point)
		return;

	setup_cover(*point);
}


//////////////////////////////////////////////////////////////////////////
// CStalkerActionCriticalHit
//////////////////////////////////////////////////////////////////////////

CStalkerActionCriticalHit::CStalkerActionCriticalHit(CAI_Stalker* object, LPCSTR action_name) :
inherited(object, action_name)
{
}

void CStalkerActionCriticalHit::initialize()
{
	inherited::initialize();

	object().brain().affect_cover(false);
	object().movement().set_movement_type(eMovementTypeStand);

	if (object().memory().EnemyManager().selected())
	{
		u32 min_queue_size, max_queue_size, min_queue_interval, max_queue_interval;
		float distance = object().memory().EnemyManager().selected()->Position().distance_to(object().Position());

		select_queue_params(distance, min_queue_size, max_queue_size, min_queue_interval, max_queue_interval);

		object().CObjectHandler::set_goal(eObjectActionIdle, object().best_weapon(), min_queue_size, max_queue_size, min_queue_interval, max_queue_interval);
	}
	else
		object().CObjectHandler::set_goal(eObjectActionIdle, object().best_weapon());

	object().sight().setup(CSightAction(SightManager::eSightTypeCurrentDirection, true, true));
	object().sound().play(eStalkerSoundInjuring);
}

void CStalkerActionCriticalHit::finalize()
{
	inherited::finalize();

	m_storage->set_property(eWorldPropertyCriticallyWounded, false);
}

void CStalkerActionCriticalHit::execute()
{
	inherited::execute();
}


//////////////////////////////////////////////////////////////////////////
// CStalkerCombatActionThrowGrenade
//////////////////////////////////////////////////////////////////////////

CStalkerCombatActionThrowGrenade::CStalkerCombatActionThrowGrenade(CAI_Stalker* object, LPCSTR action_name) :
inherited(object, action_name)
{
}

void CStalkerCombatActionThrowGrenade::initialize()
{
	inherited::initialize();

	object().movement().set_mental_state(eMentalStateDanger);

	const CInventoryItem* grenade = object().inventory().ItemFromSlot(GRENADE_SLOT);

	VERIFY(grenade);

	m_grenade_id = grenade->object().ID();

	object().movement().set_movement_type(eMovementTypeStand);
	object().movement().set_body_state(eBodyStateStand);
	object().sound().play(eStalkerSoundThrowGrenade);

	m_storage->set_property(eWorldPropertyStartedToThrowGrenade, true);
}

void CStalkerCombatActionThrowGrenade::finalize()
{
	inherited::finalize();

	m_storage->set_property(eWorldPropertyStartedToThrowGrenade, false);
}

void CStalkerCombatActionThrowGrenade::execute()
{
	inherited::execute();

	const CInventoryItem* grenade = object().inventory().ItemFromSlot(GRENADE_SLOT);

	if (!grenade || grenade->object().ID() != m_grenade_id)
	{
		object().on_throw_completed();
		m_storage->set_property(eWorldPropertyStartedToThrowGrenade, false);

		return;
	}

	const CEntityAlive* enemy = object().memory().EnemyManager().selected();

	if (!enemy)
	{
		m_storage->set_property(eWorldPropertyStartedToThrowGrenade, false);

		return;
	}

	const CMemoryInfo mem_object = object().memory().memory(enemy);

	if (!mem_object.m_object)
	{
		m_storage->set_property(eWorldPropertyStartedToThrowGrenade, false);

		return;
	}

	u32 enemy_vertex_id;
	Fvector	enemy_position;

	if (object().memory().VisualMManager().visible_now(enemy))
	{
		enemy_position = enemy->Position();
		enemy_vertex_id = enemy->ai_location().level_vertex_id();
		object().sight().setup(CSightAction(enemy, true, true));
	}
	else
	{
		enemy_position = mem_object.m_object_params.GetMemorizedPos();
		enemy_vertex_id = mem_object.m_object_params.m_level_vertex_id;
		object().sight().setup(CSightAction(SightManager::eSightTypePosition, mem_object.m_object_params.GetMemorizedPos(), true));
	}

	Fvector const enemy_direction = Fvector().sub(enemy_position, object().Position()).normalize_safe();
	Fvector const head_direction = Fvector().setHP(-object().movement().m_head.current.yaw, -object().movement().m_head.current.pitch);

	float const cos_alpha = head_direction.dotproduct(enemy_direction);

	if (_abs(acosf(cos_alpha)) >= PI_DIV_8)
		return;

	object().throw_target(enemy_position, enemy_vertex_id, const_cast<CEntityAlive*>(enemy));

	u32 min_queue_size, max_queue_size, min_queue_interval, max_queue_interval;
	float distance = enemy->Position().distance_to(object().Position());

	select_queue_params(distance, min_queue_size, max_queue_size, min_queue_interval, max_queue_interval);

	object().CObjectHandler::set_goal(eObjectActionFire1, &grenade->object(), min_queue_size, max_queue_size, min_queue_interval, max_queue_interval);
}


//////////////////////////////////////////////////////////////////////////
// CStalkerActionReachFireDistance
//////////////////////////////////////////////////////////////////////////

CStalkerActionReachFireDistance::CStalkerActionReachFireDistance(CAI_Stalker *object, LPCSTR action_name) :
inherited(object, action_name)
{

}

void CStalkerActionReachFireDistance::initialize()
{
	inherited::initialize();

	m_body_state = object().movement().body_state();

	object().movement().set_desired_direction(0);
	object().movement().set_path_type(MovementManager::ePathTypeLevelPath);
	object().movement().set_detail_path_type(DetailPathManager::eDetailPathTypeSmooth);
	object().movement().set_nearest_accessible_position();
	object().movement().set_mental_state(eMentalStateDanger);
	object().movement().set_body_state(m_body_state);
}

void CStalkerActionReachFireDistance::finalize()
{
	inherited::finalize();
}

float NEAR_COVER_SEARCH_DIST_K = 0.5f; // look near enemy
float FAR_COVER_SEARCH_DIST_K = 0.8f; // look further from enemy

void CStalkerActionReachFireDistance::execute()
{
	inherited::execute();

	if (!object().m_best_item_to_kill)
		return;

	auto enemy = object().memory().EnemyManager().selected();

	if (!enemy)
		return;

	const CMemoryInfo mem_object = object().memory().memory(enemy); 
	Fvector position = mem_object.m_object_params.GetMemorizedPos();

	float weapon_fire_dist = 25.f;

	if (object().best_weapon())
	{
		CWeapon* cur_weapon = smart_cast<CWeapon*>(object().best_weapon());

		if (cur_weapon)
			weapon_fire_dist = cur_weapon->GetEffectiveDistance();
	}

	object().m_ce_far->setup(position, 5.f, _max(15.f, weapon_fire_dist * NEAR_COVER_SEARCH_DIST_K), 10.f);
	const CCoverPoint* point = ai().cover_manager().best_cover(position, _max(15.f, weapon_fire_dist * NEAR_COVER_SEARCH_DIST_K), *object().m_ce_far);

	if (!point)	// look for cover point further from enemy
	{
		object().m_ce_far->setup(position, 5.f, _max(15.f, weapon_fire_dist * FAR_COVER_SEARCH_DIST_K), 10.f);
		point = ai().cover_manager().best_cover(position, _max(15.f, weapon_fire_dist * FAR_COVER_SEARCH_DIST_K), *object().m_ce_far);
	}

	if (point)
	{
		object().movement().set_level_dest_vertex(point->level_vertex_id());
		object().movement().set_desired_position(&point->position());
		object().movement().set_movement_type(eMovementTypeRun);

		if (object().movement().path_completed() || object().Position().distance_to(point->position()) < 10.f)
		{
			object().movement().set_body_state(eBodyStateStand);

			object().brain().affect_cover(true);
		}
		else
		{
			object().movement().set_body_state(eBodyStateCrouch);

			object().brain().affect_cover(false);
		}
	}
	else
	{
		object().brain().affect_cover(true);
		object().movement().set_movement_type(eMovementTypeRun);
		object().movement().set_body_state(eBodyStateCrouch);

		object().movement().set_level_dest_vertex(mem_object.m_object_params.m_level_vertex_id);

		if (object().movement().accessible(position))
			object().movement().set_desired_position(&position);
	}

	if (object().movement().detail().distance_to_target() < 2.f)
	{
		if (object().memory().VisualMManager().visible_now(enemy))
			object().sight().setup(CSightAction(enemy, true));
		else
			object().sight().setup(CSightAction(SightManager::eSightTypePosition, position, true));
	}
	else
		object().sight().setup(CSightAction(SightManager::eSightTypePathDirection));

	if (object().movement().path_completed())
		object().best_cover_can_try_advance();
}

//////////////////////////////////////////////////////////////////////////
// CStalkerCombatActionSmartCover
//////////////////////////////////////////////////////////////////////////

CStalkerCombatActionSmartCover::CStalkerCombatActionSmartCover(CAI_Stalker *object, LPCSTR action_name) :
	inherited(object, action_name),
	m_check_can_kill_enemy(false)
{
}

void CStalkerCombatActionSmartCover::initialize()
{
	inherited::initialize();

	m_check_can_kill_enemy = object().movement().check_can_kill_enemy();
	object().movement().check_can_kill_enemy(true);

	object().movement().combat_behaviour(true);
}

void CStalkerCombatActionSmartCover::finalize()
{
	inherited::finalize();

	object().movement().check_can_kill_enemy(m_check_can_kill_enemy);
	object().movement().combat_behaviour(false);
}

void CStalkerCombatActionSmartCover::execute()
{
	inherited::execute();

	// just for debug purposes
	// this can be only in case when solution cannot be built
	if (!object().memory().EnemyManager().selected())
		return;

	CMemoryInfo const mem_object = object().memory().memory(object().memory().EnemyManager().selected());
	if (!mem_object.m_object)
		return;

	Fvector const position = mem_object.m_object_params.GetMemorizedPos();
	CCoverPoint const* cover = object().best_cover(position);
	if (!cover) {
		object().movement().set_movement_type(eMovementTypeStand);
		object().movement().set_nearest_accessible_position();
		return;
	}

	setup_cover(*cover);

	CEntityAlive const*				enemy = object().memory().EnemyManager().selected();
	if (!enemy)
		return;

	u32 const level_time = object().memory().VisualMManager().visible_object_time_last_seen(enemy);
	if (level_time + s_wait_enemy_in_smart_cover_time >= EngineTimeU())
		return;

	if (
		object().agent_manager().member().can_detour() ||
		!object().agent_manager().member().cover_detouring() ||
		!fire_make_sense()
		)
		return;

	m_storage->set_property(eWorldPropertyPositionHolded, true);
	m_storage->set_property(eWorldPropertyInCover, false);
}