////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_stalker_misc.cpp
//	Created 	: 27.02.2003
//  Modified 	: 27.02.2003
//	Author		: Dmitriy Iassenev
//	Description : Miscellaneous functions for monster "Stalker"
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "ai_stalker.h"
#include "ai_stalker_space.h"
#include "../../bolt.h"
#include "../../inventory.h"
#include "../../character_info.h"
#include "../../relation_registry.h"
#include "../../memory_manager.h"
#include "../../item_manager.h"
#include "../../stalker_movement_manager_smart_cover.h"
#include "../../explosive.h"
#include "../../agent_manager.h"
#include "../../agent_member_manager.h"
#include "../../agent_explosive_manager.h"
#include "../../agent_location_manager.h"
#include "../../danger_object_location.h"
#include "../../member_order.h"
#include "../../level.h"
#include "../../sound_player.h"
#include "../../enemy_manager.h"
#include "../../danger_manager.h"
#include "../../visual_memory_manager.h"
#include "../../agent_enemy_manager.h"
#include "ai_stalker_impl.h"

const u32 TOLLS_INTERVAL					= 2000;
const u32 GRENADE_INTERVAL					= 0*1000;
const float FRIENDLY_GRENADE_ALARM_DIST		= 5.f;
const u32 DANGER_INFINITE_INTERVAL			= 60000000;
const float DANGER_EXPLOSIVE_DISTANCE		= 10.f;

bool CAI_Stalker::useful(const CItemManager* manager, const CGameObject* object) const
{
	const CExplosive* explosive = smart_cast<const CExplosive*>(object);

	if (explosive && smart_cast<const CInventoryItem*>(object))
		agent_manager().location().add(xr_new <CDangerObjectLocation>(object, EngineTimeU(), DANGER_INFINITE_INTERVAL, DANGER_EXPLOSIVE_DISTANCE));

	if (explosive && (explosive->CurrentParentID() != 0xffff))
	{
		agent_manager().explosive().register_explosive(explosive, object);
		CEntityAlive* entity_alive = smart_cast<CEntityAlive*>(Level().Objects.net_Find(explosive->CurrentParentID()));

		if (entity_alive)
			memory().DangerManager().add(CDangerObject(entity_alive, object->Position(), EngineTimeU(), CDangerObject::eDangerTypeGrenade, CDangerObject::eDangerPerceiveTypeVisual, object));
	}

	if (!memory().ItemManager().useful(object))
		return(false);

	const CInventoryItem* inventory_item = smart_cast<const CInventoryItem*>(object);

	if (!inventory_item || !inventory_item->useful_for_NPC())
		return(false);

	if (!const_cast<CAI_Stalker*>(this)->can_take(inventory_item))
		return(false);

	const CBolt* bolt = smart_cast<const CBolt*>(object);

	if (bolt)
		return(false);

	CInventory* inventory_non_const = const_cast<CInventory*>(&inventory());
	CInventoryItem* inventory_item_non_const = const_cast<CInventoryItem*>(inventory_item);

	if (!inventory_non_const->CanTakeItem(inventory_item_non_const))
		return(false);

	return(true);
}

float CAI_Stalker::evaluate(const CItemManager* manager, const CGameObject* object) const
{
	float distance = Position().distance_to_sqr(object->Position());
	distance = !fis_zero(distance) ? distance : EPS_L;
	return(distance);
}

bool CAI_Stalker::useful(const CEnemyManager* manager, const CEntityAlive* object) const
{
	if (!agent_manager().enemy().useful_enemy(object, this))
		return(false);

	return(memory().EnemyManager().useful(object));
}

ALife::ERelationType CAI_Stalker::tfGetRelationType(const CEntityAlive *tpEntityAlive) const
{
	const CInventoryOwner* pOtherIO = smart_cast<const CInventoryOwner*>(tpEntityAlive);

	ALife::ERelationType relation = ALife::eRelationTypeDummy;

	if (pOtherIO && !(const_cast<CEntityAlive*>(tpEntityAlive)->cast_base_monster()))
		relation = RELATION_REGISTRY().GetRelationType(static_cast<const CInventoryOwner*>(this), pOtherIO);

	if (ALife::eRelationTypeDummy != relation)
		return relation;
	else
		return inherited::tfGetRelationType(tpEntityAlive);
}

void CAI_Stalker::react_on_grenades()
{
	CMemberOrder::CGrenadeReaction	&reaction = agent_manager().member().member(this).grenade_reaction();

	if (!reaction.m_processing)
		return;

	if (EngineTimeU() < reaction.m_time + GRENADE_INTERVAL)
		return;

	//	u32	interval = AFTER_GRENADE_DESTROYED_INTERVAL;
	const CMissile* missile = smart_cast<const CMissile*>(reaction.m_grenade);
	//	if (missile && (missile->destroy_time() > EngineTimeU()))
	//	interval = missile->destroy_time() - EngineTimeU() + AFTER_GRENADE_DESTROYED_INTERVAL;
	//	m_object->agent_manager().add_danger_location(reaction.m_game_object->Position(),EngineTimeU(),interval,GRENADE_RADIUS);

	if (missile && memory().EnemyManager().AlliesCount() > 0)
	{
		CEntityAlive* initiator = smart_cast<CEntityAlive*>(Level().Objects.net_Find(reaction.m_grenade->CurrentParentID()));

		if (initiator)
		{
			if (is_relation_enemy(initiator))
				sound().play(StalkerSpace::eStalkerSoundGrenadeAlarm);
			else
				if (missile->Position().distance_to(Position()) < FRIENDLY_GRENADE_ALARM_DIST)
				{
					u32 const time = missile->destroy_time() >= EngineTimeU() ? u32(missile->destroy_time() - EngineTimeU()) : 0;
					sound().play(StalkerSpace::eStalkerSoundFriendlyGrenadeAlarm, time + 1500, time + 1000);
				}
		}
	}

	reaction.clear();
}

void CAI_Stalker::react_on_member_death()
{
	CMemberOrder::CMemberDeathReaction& reaction = agent_manager().member().member(this).member_death_reaction();
	if (!reaction.m_processing)
		return;

	if (EngineTimeU() < reaction.m_time + TOLLS_INTERVAL)
		return;

	if (agent_manager().member().group_behaviour()) {
		if (!reaction.m_member->g_Alive())
			sound().play		( StalkerSpace::eStalkerSoundTolls, 3000, 2000 );
		else
			sound().play		( StalkerSpace::eStalkerSoundWounded, 3000, 2000 );
	}

	reaction.clear();
}

void CAI_Stalker::process_enemies()
{
	if (memory().EnemyManager().selected())
		return;

	typedef MemorySpace::squad_mask_type	squad_mask_type;
	typedef CVisualMemoryManager::VISIBLES	VISIBLES;

	squad_mask_type mask = memory().VisualMManager().mask();

	VISIBLES::const_iterator I = memory().VisualMManager().GetMemorizedObjects().begin();
	VISIBLES::const_iterator E = memory().VisualMManager().GetMemorizedObjects().end();

	for (; I != E; ++I)
	{
		if (!(*I).visible(mask))
			continue;

		const CAI_Stalker* member = smart_cast<const CAI_Stalker*>((*I).m_object);

		if (!member)
			continue;

		if (is_relation_enemy(member))
			continue;

		if (!member->g_Alive())
			continue;

		if (!member->memory().EnemyManager().selected())
		{
			if (!memory().DangerManager().selected() && member->memory().DangerManager().selected())
				memory().DangerManager().add(*member->memory().DangerManager().selected());

			continue;
		}


		const CVisibleObject* obj = member->memory().VisualMManager().GetIfIsInPool(member->memory().EnemyManager().selected());

		if (obj)
		{
			memory().VisualMManager().add_visible_object(*obj, true);

			//Msg("Adding visible from member 2 %s", member->memory().EnemyManager().selected()->ObjectName().c_str());
		}

		break;
	}
}

