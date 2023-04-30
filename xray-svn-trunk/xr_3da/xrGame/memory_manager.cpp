////////////////////////////////////////////////////////////////////////////
//	Module 		: memory_manager.cpp
//	Created 	: 02.10.2001
//  Modified 	: 19.11.2003
//	Author		: Dmitriy Iassenev
//	Description : Memory manager
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "memory_manager.h"
#include "visual_memory_manager.h"
#include "sound_memory_manager.h"
#include "hit_memory_manager.h"
#include "enemy_manager.h"
#include "item_manager.h"
#include "danger_manager.h"
#include "ai/stalker/ai_stalker.h"
#include "agent_manager.h"
#include "agent_member_manager.h"
#include "memory_space_impl.h"
#include "profiler.h"
#include "agent_enemy_manager.h"
#include "script_game_object.h"
#include "ai\stalker\ai_stalker_impl.h"
CMemoryManager::CMemoryManager(CEntityAlive *entity_alive, CSound_UserDataVisitor *visitor)
{
	VERIFY				(entity_alive);
	m_object			= smart_cast<CCustomMonster*>(entity_alive);
	m_stalker			= smart_cast<CAI_Stalker*>(m_object);

	if (m_stalker)
		m_visual		= xr_new <CVisualMemoryManager>(m_stalker);
	else
		m_visual		= xr_new <CVisualMemoryManager>(m_object);

	m_sound				= xr_new <CSoundMemoryManager>(m_object, m_stalker, visitor);
	m_hit				= xr_new <CHitMemoryManager>(m_object, m_stalker);
	m_enemy				= xr_new <CEnemyManager>(m_object);
	m_item				= xr_new <CItemManager>(m_object);
	m_danger			= xr_new <CDangerManager>(m_object);

	ignoreGroupVMemoryDist_ = 0.f;
	ignoreGroupSMemoryDist_ = 0.f;
	ignoreGroupHMemoryDist_ = 0.f;

	nextFrameCheckOld_		= 0;
}

CMemoryManager::~CMemoryManager()
{
	xr_delete			(m_visual);
	xr_delete			(m_sound);
	xr_delete			(m_hit);
	xr_delete			(m_enemy);
	xr_delete			(m_item);
	xr_delete			(m_danger);
}

void CMemoryManager::LoadCfg(LPCSTR section)
{
	SoundMManager().LoadCfg(section);
	HitMManager().LoadCfg(section);
	EnemyManager().LoadCfg(section);
	ItemManager().LoadCfg(section);
	DangerManager().LoadCfg(section);

	ignoreGroupVMemoryDist_ = READ_IF_EXISTS(pSettings, r_float, section, "ignore_group_vmemory_dist", 150.f);
	ignoreGroupSMemoryDist_ = READ_IF_EXISTS(pSettings, r_float, section, "ignore_group_smemory_dist", 150.f);
	ignoreGroupHMemoryDist_ = READ_IF_EXISTS(pSettings, r_float, section, "ignore_group_hmemory_dist", 150.f);

	playSearchSndChance_	= READ_IF_EXISTS(pSettings, r_s32, section, "play_search_sound_chance", 3);
}

void CMemoryManager::reinit()
{
	VisualMManager().reinit();
	SoundMManager().reinit();
	HitMManager().reinit();
	EnemyManager().reinit();
	ItemManager().reinit();
	DangerManager().reinit();
}

void CMemoryManager::reload(LPCSTR section)
{
	VisualMManager().reload(section);
	SoundMManager().reload(section);
	HitMManager().reload(section);
	EnemyManager().reload(section);
	ItemManager().reload(section);
	DangerManager().reload(section);
}

#ifdef _DEBUG
extern bool g_enemy_manager_second_update;
#endif

void CMemoryManager::update_enemies	(const bool registered_in_combat)
{
#ifdef _DEBUG
	g_enemy_manager_second_update = false;
#endif

	bool enemy_unselected_or_enemy_wounded = (!EnemyManager().selected() || (smart_cast<const CAI_Stalker*>(EnemyManager().selected()) && smart_cast<const CAI_Stalker*>(EnemyManager().selected())->wounded()));
	
	if (m_stalker && enemy_unselected_or_enemy_wounded && registered_in_combat)
	{
		m_stalker->agent_manager().enemy().distribute_enemies();

#ifdef _DEBUG
		g_enemy_manager_second_update = true;
#endif
	}

	EnemyManager().update();

}

void CMemoryManager::update(float time_delta)
{
	START_PROFILE("Memory Manager")

	VisualMManager().update(time_delta);
	SoundMManager().update();
	HitMManager().update();
	
	bool registered_in_combat = true;

	if (m_stalker)
		registered_in_combat = m_stalker->agent_manager().member().registered_in_combat(m_stalker) && !m_stalker->wounded();

	// update enemies and items
	ItemManager().reset();
	EnemyManager().reset();

	if (VisualMManager().enabled())		
		update_v(registered_in_combat);

	update_s(registered_in_combat);
	update_h(registered_in_combat);
	
	update_enemies(registered_in_combat);

	if (registered_in_combat && m_stalker && m_stalker->ActiveSoundsNum() <= 0)
		if (EnemyManager().selected() && EnemyManager().LastTimeEnemyFound() + (EnemyManager().clearEnemiesTime_ / 2) < EngineTimeU())
			if (::Random.randI(1, 100) <= playSearchSndChance_)
				m_stalker->PlaySearchSound();

	ItemManager().update();
	DangerManager().update();

	if (CurrentFrame() >= nextFrameCheckOld_) // every %% frames check for nulled pointers or itterators in the npc memory
	{
		if (SoundMManager().IsOKToCheckInvalid() && VisualMManager().IsOKToCheckInvalid() && HitMManager().IsOKToCheckInvalid())
			EraseOldAndInvalidMem();

		nextFrameCheckOld_ = CurrentFrame() + ::Random.randI(50, 70);
	}
	
	STOP_PROFILE
}

void CMemoryManager::enable(const CObject *object, bool enable)
{
	VisualMManager().enable(object, enable);
	SoundMManager().enable(object, enable);
	HitMManager().enable(object, enable);
}

void CMemoryManager::update_v(bool add_enemies)
{
	xr_vector<CVisibleObject>::const_iterator	I = VisualMManager().GetMemorizedObjectsP()->begin();
	xr_vector<CVisibleObject>::const_iterator	E = VisualMManager().GetMemorizedObjectsP()->end();

	for ( ; I != E; ++I)
	{
		if (!(*I).m_enabled)
			continue;

		//if (m_stalker && !(*I).m_squad_mask.test(mask))
		//	continue;

		if ((*I).m_deriving_vmemory_owner && (*I).m_deriving_vmemory_owner != m_object) // check if it is a grouped enemy(located by somebody else))
		{
			if (((*I).m_level_time + VisualMManager().forgetVisTime_ < EngineTimeU() || (*I).m_level_time > u32(-1) - 500000))
				continue;

			if ((*I).m_object)
			{
				float object_distance = (*I).m_object->Position().distance_to(m_object->Position());

				if (object_distance > ignoreGroupVMemoryDist_)
					continue;
			}
		}

		DangerManager().add(*I);

		const CEntityAlive* entity_alive = smart_cast<const CEntityAlive*>((*I).m_object);

		if (entity_alive && m_object->is_relation_enemy(entity_alive))
			if (add_enemies && EnemyManager().AddEnemy(entity_alive))
				continue;

		const CAI_Stalker* stalker = smart_cast<const CAI_Stalker*>((*I).m_object);

		if (m_stalker && stalker)
			continue;

		if ((*I).m_object)
			ItemManager().add((*I).m_object);
	}
}


void CMemoryManager::update_s(bool add_enemies)
{
	xr_vector<CSoundObject>::const_iterator	I = SoundMManager().GetMemorizedSoundsP()->begin();
	xr_vector<CSoundObject>::const_iterator	E = SoundMManager().GetMemorizedSoundsP()->end();

	for (; I != E; ++I)
	{
		if (!(*I).m_enabled)
			continue;

		//if (m_stalker && !(*I).m_squad_mask.test(mask))
		//	continue;

		if ((*I).m_deriving_smemory_owner && (*I).m_deriving_smemory_owner != m_object) // check if it is a grouped enemy(located by somebody else))
		{
			if (((*I).m_level_time + SoundMManager().forgetSndTime_ < EngineTimeU() || (*I).m_level_time > u32(-1) - 500000))
				continue;

			if ((*I).m_object)
			{
				float object_distance = (*I).m_object->Position().distance_to(m_object->Position());

				if (object_distance > ignoreGroupSMemoryDist_)
					continue;
			}
		}

		DangerManager().add(*I);

		const CEntityAlive* entity_alive = smart_cast<const CEntityAlive*>((*I).m_object);

		if (entity_alive && m_object->is_relation_enemy(entity_alive))
			if (add_enemies && EnemyManager().AddEnemy(entity_alive))
				continue;

		const CAI_Stalker* stalker = smart_cast<const CAI_Stalker*>((*I).m_object);

		if (m_stalker && stalker)
			continue;

		if ((*I).m_object)
			ItemManager().add((*I).m_object);
	}
}


void CMemoryManager::update_h(bool add_enemies)
{
	xr_vector<CHitObject>::const_iterator	I = HitMManager().GetMemorizedHitsP()->begin();
	xr_vector<CHitObject>::const_iterator	E = HitMManager().GetMemorizedHitsP()->end();

	for (; I != E; ++I)
	{
		if (!(*I).m_enabled)
			continue;

		//if (m_stalker && !(*I).m_squad_mask.test(mask))
		//	continue;

		if ((*I).m_deriving_hmemory_owner && (*I).m_deriving_hmemory_owner != m_object) // check if it is a grouped enemy(located by somebody else))
		{
			if (((*I).m_level_time + HitMManager().forgetHitTime_ < EngineTimeU() || (*I).m_level_time > u32(-1) - 500000))
				continue;

			if ((*I).m_object)
			{
				float object_distance = (*I).m_object->Position().distance_to(m_object->Position());

				if (object_distance > ignoreGroupHMemoryDist_)
					continue;
			}
		}

		DangerManager().add(*I);

		const CEntityAlive* entity_alive = smart_cast<const CEntityAlive*>((*I).m_object);

		if (entity_alive && m_object->is_relation_enemy(entity_alive))
			if (add_enemies && EnemyManager().AddEnemy(entity_alive))
				continue;

		const CAI_Stalker* stalker = smart_cast<const CAI_Stalker*>((*I).m_object);

		if (m_stalker && stalker)
			continue;

		if ((*I).m_object)
			ItemManager().add((*I).m_object);
	}
}


CMemoryInfo CMemoryManager::memory(const CObject *object) const
{
	CMemoryInfo	result;

	if (!this->object().g_Alive())
		return(result);

	u32 level_time = 0;
	const CGameObject* game_object = smart_cast<const CGameObject*>(object);

	VERIFY(game_object);

	squad_mask_type					mask = m_stalker ? m_stalker->agent_manager().member().mask(m_stalker) : squad_mask_type(-1);

	{
		xr_vector<CVisibleObject>::const_iterator	I = std::find(VisualMManager().GetMemorizedObjects().begin(), VisualMManager().GetMemorizedObjects().end(), object_id(object));

		if (VisualMManager().GetMemorizedObjects().end() != I)
		{
			(CMemoryObject<CGameObject>&)result	= (CMemoryObject<CGameObject>&)(*I);
			result.visible						(mask, (*I).visible(mask));
			result.m_visual_info				= true;
			level_time							= (*I).m_level_time;

			VERIFY(result.m_object);
		}
	}

	{
		xr_vector<CSoundObject>::const_iterator	I = std::find(SoundMManager().GetMemorizedSounds().begin(), SoundMManager().GetMemorizedSounds().end(), object_id(object));

		if ((SoundMManager().GetMemorizedSounds().end() != I) && (level_time < (*I).m_level_time))
		{
			(CMemoryObject<CGameObject>&)result = (CMemoryObject<CGameObject>&)(*I);
			result.m_sound_info						= true;
			level_time								= (*I).m_level_time;

			VERIFY(result.m_object);
		}
	}
	
	{
		xr_vector<CHitObject>::const_iterator	I = std::find(HitMManager().GetMemorizedHits().begin(), HitMManager().GetMemorizedHits().end(), object_id(object));

		if ((HitMManager().GetMemorizedHits().end() != I) && (level_time < (*I).m_level_time))
		{
			(CMemoryObject<CGameObject>&)result = (CMemoryObject<CGameObject>&)(*I);
			result.m_object							= game_object;
			result.m_hit_info						= true;

			VERIFY(result.m_object);
		}
	}

	return (result);
}

u32 CMemoryManager::memory_time(const CObject *object) const
{
	u32					result = 0;
	if (!this->object().g_Alive())
		return			(0);

	const CGameObject	*game_object = smart_cast<const CGameObject*>(object);
	VERIFY				(game_object);

	{
		xr_vector<CVisibleObject>::const_iterator	I = std::find(VisualMManager().GetMemorizedObjects().begin(), VisualMManager().GetMemorizedObjects().end(), object_id(object));
		if (VisualMManager().GetMemorizedObjects().end() != I)
			result		= (*I).m_level_time;
	}

	{
		xr_vector<CSoundObject>::const_iterator	I = std::find(SoundMManager().GetMemorizedSounds().begin(), SoundMManager().GetMemorizedSounds().end(), object_id(object));
		if ((SoundMManager().GetMemorizedSounds().end() != I) && (result < (*I).m_level_time))
			result		= (*I).m_level_time;
	}
	
	{
		xr_vector<CHitObject>::const_iterator	I = std::find(HitMManager().GetMemorizedHits().begin(), HitMManager().GetMemorizedHits().end(), object_id(object));
		if ((HitMManager().GetMemorizedHits().end() != I) && (result < (*I).m_level_time))
			result		= (*I).m_level_time;
	}

	return(result);
}

Fvector CMemoryManager::memory_position	(const CObject *object) const
{
	u32 time = 0;
	Fvector result = Fvector().set(0.f,0.f,0.f);

	if (!this->object().g_Alive())
		return (result);

	const CGameObject* game_object = smart_cast<const CGameObject*>(object);

	VERIFY(game_object);

	{
		xr_vector<CVisibleObject>::const_iterator	I = std::find(VisualMManager().GetMemorizedObjects().begin(), VisualMManager().GetMemorizedObjects().end(), object_id(object));

		if (VisualMManager().GetMemorizedObjects().end() != I)
		{
			time		= (*I).m_level_time;
			result = (*I).m_object_params.GetMemorizedPos();
		}
	}

	{
		xr_vector<CSoundObject>::const_iterator	I = std::find(SoundMManager().GetMemorizedSounds().begin(), SoundMManager().GetMemorizedSounds().end(), object_id(object));

		if ((SoundMManager().GetMemorizedSounds().end() != I) && (time < (*I).m_level_time))
		{
			time		= (*I).m_level_time;
			result = (*I).m_object_params.GetMemorizedPos();
		}
	}
	
	{
		xr_vector<CHitObject>::const_iterator	I = std::find(HitMManager().GetMemorizedHits().begin(), HitMManager().GetMemorizedHits().end(), object_id(object));

		if ((HitMManager().GetMemorizedHits().end() != I) && (time < (*I).m_level_time))
		{
			time		= (*I).m_level_time;
			result = (*I).m_object_params.GetMemorizedPos();
		}
	}

	return(result);
}

void CMemoryManager::remove_links(CObject *object)
{
	VisualMManager().remove_links(object);
	SoundMManager().remove_links(object);
	HitMManager().remove_links(object);

	DangerManager().remove_links(object);
	EnemyManager().remove_links(object);
	ItemManager().remove_links(object);
}

void CMemoryManager::remove_links_from_sorted(CObject *object)
{
	if (strstr(object->ObjectName().c_str(), "single_player"))
	{
		Msg("!Removing links because old %s", object->ObjectName().c_str());
		LogStackTrace("!");
	}

	DangerManager().remove_links(object);
	EnemyManager().remove_links(object);
	ItemManager().remove_links(object);
}

void CMemoryManager::on_restrictions_change()
{
	if (!m_object->g_Alive())
		return;

//	danger().on_restrictions_change	();
//	enemy().on_restrictions_change	();
	ItemManager().on_restrictions_change();
}

void CMemoryManager::make_object_visible_somewhen(const CEntityAlive *enemy)
{

	squad_mask_type				mask = stalker().agent_manager().member().mask(&stalker());
	MemorySpace::CVisibleObject	*obj = VisualMManager().visible_object(enemy);
//	if (obj) {
//		Msg						("------------------------------------------------------");
//		Msg						("[%6d] make_object_visible_somewhen [%s] = %x",EngineTimeU(),*enemy->ObjectName(),obj->m_squad_mask.get());
//	}
//	LogStackTrace				("-------------make_object_visible_somewhen-------------");
	bool						prev = obj ? obj->visible(mask) : false;
	VisualMManager().add_visible_object(enemy, .001f, true);

	MemorySpace::CVisibleObject	*obj1 = object().memory().VisualMManager().visible_object(enemy);

	VERIFY						(obj1);
//	if (obj1)
//		Msg						("[%6d] make_object_visible_somewhen [%s] = %x",EngineTimeU(),*enemy->ObjectName(),obj1->m_squad_mask.get());

	obj1->visible				(mask,prev);

}

void CMemoryManager::save(NET_Packet &packet)
{
	EraseOldAndInvalidMem();

	VisualMManager().save		(packet);
	SoundMManager().save(packet);
	HitMManager().save(packet);
	DangerManager().save		(packet);
}

void CMemoryManager::load(IReader &packet)
{
	VisualMManager().load		(packet);
	SoundMManager().load(packet);
	HitMManager().load(packet);
	DangerManager().load		(packet);
}

// we do this due to the limitation of client spawn manager
// should be revisited from the acrhitectural point of view
void CMemoryManager::on_requested_spawn(CObject *object)
{
	VisualMManager().on_requested_spawn(object);
	SoundMManager().on_requested_spawn(object);
	HitMManager().on_requested_spawn(object);
}

struct SRemoveOldPredicate
{
	u32 time_limit;

	SRemoveOldPredicate(u32 time) : time_limit(time)
	{
	}

	bool operator() (const CVisibleObject &object) const
	{
		if (!object.m_object)
			return true;

		return(object.m_level_time + time_limit < EngineTimeU() || object.m_level_time == 0);
	}

	bool operator() (const CSoundObject &object) const
	{
		if (!object.m_object)
			return true;

		return(object.m_level_time + time_limit < EngineTimeU() || object.m_level_time == 0);
	}

	bool operator() (const CHitObject &object) const
	{
		if (!object.m_object)
			return true;

		return(object.m_level_time + time_limit < EngineTimeU() || object.m_level_time == 0);
	}
};


void CMemoryManager::EraseOldAndInvalidMem()
{
	VisualMManager().GetMemorizedObjectsP()->erase(std::remove_if(VisualMManager().GetMemorizedObjectsP()->begin(), VisualMManager().GetMemorizedObjectsP()->end(), SRemoveOldPredicate(VisualMManager().forgetVisTime_)), VisualMManager().GetMemorizedObjectsP()->end());
	SoundMManager().GetMemorizedSoundsP()->erase(std::remove_if(SoundMManager().GetMemorizedSoundsP()->begin(), SoundMManager().GetMemorizedSoundsP()->end(), SRemoveOldPredicate(SoundMManager().forgetSndTime_)), SoundMManager().GetMemorizedSoundsP()->end());
	HitMManager().GetMemorizedHitsP()->erase(std::remove_if(HitMManager().GetMemorizedHitsP()->begin(), HitMManager().GetMemorizedHitsP()->end(), SRemoveOldPredicate(HitMManager().forgetHitTime_)), HitMManager().GetMemorizedHitsP()->end());
}