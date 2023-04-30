
//	Created 	: 25.03.2004
//	Author		: Dmitriy Iassenev

#include "pch_script.h"
#include "stalker_search_actions.h"
#include "ai/stalker/ai_stalker.h"
#include "script_game_object.h"
#include "movement_manager_space.h"
#include "agent_manager.h"
#include "agent_member_manager.h"
#include "memory_manager.h"
#include "enemy_manager.h"
#include "hit_memory_manager.h"
#include "visual_memory_manager.h"
#include "sight_manager.h"
#include "cover_evaluators.h"
#include "cover_manager.h"
#include "stalker_movement_restriction.h"
#include "cover_point.h"
#include "detail_path_manager_space.h"
#include "stalker_movement_manager_smart_cover.h"
#include "script_game_object_impl.h"
using namespace StalkerSpace;
using namespace StalkerDecisionSpace;

//////////////////////////////////////////////////////////////////////////
// CStalkerActionReachEnemyLocation
//////////////////////////////////////////////////////////////////////////

CStalkerActionReachEnemyLocation::CStalkerActionReachEnemyLocation(CAI_Stalker* object, CPropertyStorage* combat_storage, LPCSTR action_name) :
	inherited(object, action_name),
	m_combat_storage(combat_storage)
{
}

void CStalkerActionReachEnemyLocation::initialize()
{
	inherited::initialize();

	crouchStateUsed_ = false;
	goingToEnemyPoint_ = false;

	object().movement().set_desired_direction(0);
	object().movement().set_path_type(MovementManager::ePathTypeLevelPath);
	object().movement().set_detail_path_type(DetailPathManager::eDetailPathTypeSmooth);
	object().movement().set_mental_state(eMentalStateDanger);
	object().movement().set_body_state(eBodyStateStand);
	object().movement().set_movement_type(eMovementTypeWalk);

	aim_ready();

	object().agent_manager().member().member(m_object).cover(0);
}

void CStalkerActionReachEnemyLocation::finalize()
{
	inherited::finalize();
}

void CStalkerActionReachEnemyLocation::execute()
{
	inherited::execute();

	if (!memParamsInitialized_)
		return;

	if (!goingToEnemyPoint_)
	{
		if (object().movement().accessible(enemyVertexIDSaved_))
		{
			object().movement().set_level_dest_vertex(enemyVertexIDSaved_, 2.f);
		}
		else
		{
			object().movement().set_nearest_accessible_position(enemyPositionSaved_, enemyVertexIDSaved_, 2.f);
		}

		goingToEnemyPoint_ = true;
	}

	object().sight().setup(CSightAction(SightManager::eSightTypePosition, Fvector(enemyPositionSaved_).add(Fvector().set(0.f, .5f, 0.f)), true));

	if (object().movement().path_completed())
	{
#if 0
		object().m_ce_ambush->setup(mem_object.m_object_params.m_position, mem_object.m_self_params.m_position, 10.f);
		const CCoverPoint				*point = ai().cover_manager().best_cover(mem_object.m_object_params.m_position, 10.f, *object().m_ce_ambush, CStalkerMovementRestrictor(m_object, true));
		if (!point) {
			object().m_ce_ambush->setup(mem_object.m_object_params.m_position, mem_object.m_self_params.m_position, 10.f);
			point = ai().cover_manager().best_cover(mem_object.m_object_params.m_position, 30.f, *object().m_ce_ambush, CStalkerMovementRestrictor(m_object, true));
		}

		if (point) {
			object().movement().set_level_dest_vertex(point->level_vertex_id());
			object().movement().set_desired_position(&point->position());
		}
		else
			object().movement().set_nearest_accessible_position();
#else

		m_storage->set_property(eWorldPropertyEnemyLocationReached, true);
#endif
#ifndef SILENT_COMBAT
		if (object().memory().EnemyManager().AlliesCount() > 0)
			play_start_search_sound(0, 0, 10000, 10000);
#endif
	}
	else if (!crouchStateUsed_ && object().movement().is_path_varify_state() && !object().movement().distance_to_destination_greater(10.f))
	{
		object().movement().set_body_state(eBodyStateCrouch);
		object().movement().set_movement_type(eMovementTypeRun);

		crouchStateUsed_ = true;
	}
}

//////////////////////////////////////////////////////////////////////////
// CStalkerActionReachAmbushLocation
//////////////////////////////////////////////////////////////////////////

CStalkerActionReachAmbushLocation::CStalkerActionReachAmbushLocation(CAI_Stalker* object, CPropertyStorage* combat_storage, LPCSTR action_name) :
	inherited(object, action_name),

	m_combat_storage(combat_storage)
{
}

void CStalkerActionReachAmbushLocation::initialize()
{
	inherited::initialize();

	goingToAmbushPoint_ = false;
	standStateUsed_ = false;
}

void CStalkerActionReachAmbushLocation::finalize()
{
	inherited::finalize();
}

void CStalkerActionReachAmbushLocation::execute()
{
	inherited::execute();

	if (!memParamsInitialized_)
		return;

	if (!goingToAmbushPoint_)
	{
		object().m_ce_ambush->setup(object().Position(), enemyPositionSaved_, 10.f);
		const CCoverPoint* point = ai().cover_manager().best_cover(enemyPositionSaved_, 15.f, *object().m_ce_ambush, CStalkerMovementRestrictor(m_object, true));

		if (!point)
		{
			object().m_ce_ambush->setup(object().Position(), enemyPositionSaved_, 10.f);
			point = ai().cover_manager().best_cover(enemyPositionSaved_, 30.f, *object().m_ce_ambush, CStalkerMovementRestrictor(m_object, true));
		}

		if (point)
		{
			object().movement().set_level_dest_vertex(point->level_vertex_id(), 1.2f);
			object().movement().set_desired_position(&point->position());

			goingToAmbushPoint_ = true;
		}
		else
			object().movement().set_nearest_accessible_position();
	}

	if (!object().movement().path_completed())
	{
#ifndef SILENT_COMBAT
		if (object().memory().EnemyManager().AlliesCount() > 0)
			play_enemy_lost_sound(0, 0, 10000, 10000);
#endif
		// Stand up when closer to ambush point
		if (!standStateUsed_ && object().movement().is_path_varify_state() && !object().movement().distance_to_destination_greater(10.f))
		{
			object().movement().set_body_state(eBodyStateStand);
			object().movement().set_movement_type(eMovementTypeWalk);

			standStateUsed_ = true;
		}

		return;
	}

	m_storage->set_property(eWorldPropertyAmbushLocationReached, true);
}

//////////////////////////////////////////////////////////////////////////
// CStalkerActionHoldAmbushLocation
//////////////////////////////////////////////////////////////////////////

CStalkerActionHoldAmbushLocation::CStalkerActionHoldAmbushLocation(CAI_Stalker* object, CPropertyStorage* combat_storage, LPCSTR action_name) :
	inherited			(object, action_name),
	m_combat_storage	(combat_storage)
{
}

void CStalkerActionHoldAmbushLocation::initialize()
{
	inherited::initialize();

	otherAmbushPointMinDist_ = 5.f;

	if (!check_if_need_other_ambush_point())
	{
		// Set look and body state
		Fvector look_pos;
		look_pos = object().sight().current_action().vector3d();

		object().movement().set_body_state(eBodyStateCrouch);
		object().sight().setup(CSightAction(SightManager::eSightTypeCoverLookOver, look_pos, true));
	}
}

void CStalkerActionHoldAmbushLocation::finalize()
{
	inherited::finalize();
}

void CStalkerActionHoldAmbushLocation::execute()
{
	inherited::execute();

	if (object().movement().path_completed())
	{
		if (!check_if_need_other_ambush_point())
		{
			// Set look and body state
			Fvector look_pos;
			look_pos = object().sight().current_action().vector3d();

			object().movement().set_body_state(eBodyStateCrouch);
			object().sight().setup(CSightAction(SightManager::eSightTypeCoverLookOver, look_pos, true));
		}
	}

	if (!memObject_.m_object)
		return;

	if (!completed())
		return;

	if (memObject_.m_last_level_time + 60000 < EngineTimeU())
		return;

	object().memory().enable(object().memory().EnemyManager().selected(), false);
}

bool CStalkerActionHoldAmbushLocation::check_if_need_other_ambush_point()
{
	if (object().movement().path_completed())
	{
		// Try to make NPC find other cover place, if this is already taken
		auto vis_mem_objs = object().memory().VisualMManager().GetMemorizedObjectsP();

		for (u32 i = 0; i < vis_mem_objs->size(); i++)
		{
			if (vis_mem_objs->at(i).m_object)
			{
				CEntityAlive const* const entity_alive = smart_cast<CEntityAlive const*>(vis_mem_objs->at(i).m_object);

				// Check if we have crowded up cover point
				if (entity_alive && entity_alive->g_Alive() && !object().is_relation_enemy(entity_alive) && object().Position().similar(vis_mem_objs->at(i).m_object_params.GetMemorizedPos(), 2.f))
				{
#ifdef DEBUG
					Msg("! Find Other for cover %s ! ", object().ObjectNameStr());
#endif
					float min_dist = otherAmbushPointMinDist_ + Random.randF(-3.f, 3.f); // Random is to avoid two npcs always selecting same point
					min_dist = _max(2.f, min_dist);

					object().m_ce_ambush->setup(object().Position(), object().Position(), min_dist);
					const CCoverPoint* point = ai().cover_manager().best_cover(object().Position(), _max(15.f, min_dist + 10.f), *object().m_ce_ambush, CStalkerMovementRestrictor(m_object, true));

					if (point)
					{
						object().movement().set_body_state(eBodyStateCrouch);
						object().movement().set_movement_type(eMovementTypeRun);

						object().movement().set_level_dest_vertex(point->level_vertex_id(), 1.2f);
						object().movement().set_desired_position(&point->position());

#ifdef DEBUG
						Msg("! Found Cover for %s ! cur %f %f %f covp %f %f %f, dist %f", object().ObjectNameStr(), VPUSH(point->m_position), VPUSH(object().Position()), object().Position().distance_to(point->m_position));
#endif
						return true;
					}

					otherAmbushPointMinDist_ += 3.f;
				}
			}
		}
	}

	return false;
}