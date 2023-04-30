
//	Created 	: 02.10.2001
//	Author		: Dmitriy Iassenev

#include "pch_script.h"
#include "sound_memory_manager.h"
#include "memory_manager.h"
#include "hit_memory_manager.h"
#include "visual_memory_manager.h"
#include "enemy_manager.h"
#include "memory_space_impl.h"
#include "custommonster.h"
#include "agent_manager.h"
#include "agent_member_manager.h"
#include "ai/stalker/ai_stalker.h"
#include "profiler.h"
#include "client_spawn_manager.h"
#include "clsid_game.h"
#include "actor.h"
#include "level.h"
#include "GameConstants.h"
#include "ai\stalker\ai_stalker_impl.h"
#include "ai_sounds.h"
#define SILENCE
//#define SAVE_OWN_SOUNDS
//#define SAVE_OWN_ITEM_SOUNDS
#define SAVE_NON_ALIVE_OBJECT_SOUNDS
#define SAVE_FRIEND_ITEM_SOUNDS
#define SAVE_FRIEND_SOUNDS
//#define SAVE_VISIBLE_OBJECT_SOUNDS

const float COMBAT_SOUND_PERCEIVE_RADIUS_SQR = _sqr(5.f);

CSoundMemoryManager::~CSoundMemoryManager()
{
	clear_delayed_objects();

#ifdef USE_SELECTED_SOUND
	xr_delete(m_selected_sound);
#endif

	countOfSavedAlliesSounds_	= 0;
}

void CSoundMemoryManager::LoadCfg(LPCSTR section)
{
}

void CSoundMemoryManager::reinit()
{
	m_priorities.clear();
	m_last_sound_time = 0;
	m_sound_threshold = m_min_sound_threshold;

	VERIFY(_valid(m_sound_threshold));

#ifdef USE_SELECTED_SOUND
	xr_delete				(m_selected_sound);
#endif
}

void CSoundMemoryManager::reload(LPCSTR section)
{
	m_max_sound_count		= READ_IF_EXISTS(pSettings, r_s32, section, "DynamicSoundsCount", 1);
	m_min_sound_threshold	= READ_IF_EXISTS(pSettings, r_float, section, "sound_threshold", 0.05f);
	m_self_sound_factor		= READ_IF_EXISTS(pSettings, r_float, section, "self_sound_factor", 0.f);
	m_sound_decrease_quant	= READ_IF_EXISTS(pSettings, r_u32, section, "self_decrease_quant", 250);
	m_decrease_factor		= READ_IF_EXISTS(pSettings, r_float, section, "self_decrease_factor", .95f);

	LPCSTR					sound_perceive_section = READ_IF_EXISTS(pSettings, r_string, section, "sound_perceive_section", section);
	m_weapon_factor			= READ_IF_EXISTS(pSettings, r_float, sound_perceive_section, "weapon", 10.f);
	m_item_factor			= READ_IF_EXISTS(pSettings, r_float, sound_perceive_section, "item", 1.f);
	m_npc_factor			= READ_IF_EXISTS(pSettings, r_float, sound_perceive_section, "npc", 1.f);
	m_anomaly_factor		= READ_IF_EXISTS(pSettings, r_float, sound_perceive_section, "anomaly", 1.f);
	m_world_factor			= READ_IF_EXISTS(pSettings, r_float, sound_perceive_section, "world", 1.f);

	forgetSndTime_			= READ_IF_EXISTS(pSettings, r_s32, section, "forget_snd_time", 60);
	forgetSndTime_			*= 1000;

	if (m_stalker)
		UpdateHearing_RankDependance();

	VERIFY(m_max_sound_count);
	VERIFY(_valid(m_self_sound_factor));
	VERIFY(_valid(m_min_sound_threshold));
	VERIFY(!fis_zero(m_decrease_factor));
	VERIFY(m_sound_decrease_quant);

}

void CSoundMemoryManager::UpdateHearing_RankDependance()
{
	if (m_stalker) // stalkers. exclude actor
	{
		LPCSTR section = m_stalker->SectionName().c_str();

		m_stalker->cast_inventory_owner();
		CInventoryOwner* owner = m_stalker ? m_stalker->cast_inventory_owner() : m_object ? m_object->cast_inventory_owner() : nullptr;

		if (owner)
		{
			// Load hearing dependance from rank
			if (owner->GetIORank() >= GameConstants::GetMasterRankStart())
			{
				soundRankCoef_.LoadCfg(pSettings->r_string(section, "sound_hearing_master_K"));
			}
			else if (owner->GetIORank() >= GameConstants::GetVeteranRankStart())
			{
				soundRankCoef_.LoadCfg(pSettings->r_string(section, "sound_hearing_veteran_K"));
			}
			else if (owner->GetIORank() >= GameConstants::GetExperiencesRankStart())
			{
				soundRankCoef_.LoadCfg(pSettings->r_string(section, "sound_hearing_experienced_K"));
			}
			else
			{
				soundRankCoef_.LoadCfg(pSettings->r_string(section, "sound_hearing_novice_K"));
			}
		}
	}
}

IC void CSoundMemoryManager::update_sound_threshold()
{
	m_sound_threshold = _max(
		m_self_sound_factor*
		m_sound_threshold*
		exp(
			float(EngineTimeU() - m_last_sound_time)/
			float(m_sound_decrease_quant)*
			log(m_decrease_factor)
		),
		m_min_sound_threshold
	);

	VERIFY(_valid(m_sound_threshold));
}

IC	u32	 CSoundMemoryManager::priority(const MemorySpace::CSoundObject &sound) const
{
	u32	priority = u32(-1);

	xr_map<ESoundTypes,u32>::const_iterator	I = m_priorities.begin();
	xr_map<ESoundTypes,u32>::const_iterator	E = m_priorities.end();

	for ( ; I != E; ++I)
		if (((*I).second < priority) && ((*I).first & sound.m_sound_type) == (*I).first)
			priority = (*I).second;

	return(priority);
}

void CSoundMemoryManager::enable(const CObject *object, bool enable)
{
	xr_vector<CSoundObject>::iterator J = std::find(GetMemorizedSoundsP()->begin(), GetMemorizedSoundsP()->end(), object_id(object));

	if (J == GetMemorizedSoundsP()->end())
		return;

	(*J).m_enabled = enable;
}

IC	bool is_sound_type(int s, const ESoundTypes &t)
{
	return((s & t) == t);
}

extern Flags32 psActorFlags;
void CSoundMemoryManager::feel_sound_new(CObject *object, int sound_type, CSound_UserDataPtr user_data, const Fvector &position, float sound_power)
{
	if (object && smart_cast<CActor*>(object) && psActorFlags.test(AF_INVISIBLE))
		return;

	VERIFY(_valid(sound_power));

	if (!GetMemorizedSoundsP())
		return;

	if (user_data)
		user_data->accept	(m_visitor);

	VERIFY(m_object);

#ifndef SILENCE
	Msg ("^Sound Memory: %s (%d) - sound type %x from %s at %d in (%.2f,%.2f,%.2f) with power %.2f",
		*self->ObjectName(), EngineTimeU(), sound_type, object ? *object->ObjectName() : "world", EngineTimeU(), position.x, position.y, position.z, sound_power);
#endif

	m_object->sound_callback(object, sound_type, position, sound_power);

	if (object && (m_object->ID() == object->ID()))
		return;

	update_sound_threshold();

	CEntityAlive* self_entity_alive = m_object;

	if (!self_entity_alive->g_Alive())
		return;

	if (is_sound_type(sound_type, SOUND_TYPE_WEAPON))
		sound_power *= GetWeaponSoundValue();
	
	if (is_sound_type(sound_type, SOUND_TYPE_ITEM))
		sound_power *= GetItemSoundValue();

	if (is_sound_type(sound_type, SOUND_TYPE_MONSTER))
		sound_power *= GetNpcSoundValue();

	if (is_sound_type(sound_type, SOUND_TYPE_ANOMALY))
		sound_power *= GetAnomalySoundValue();
	
	if (is_sound_type(sound_type, SOUND_TYPE_WORLD))
		sound_power *= GetWorldSoundValue();
	
	VERIFY(_valid(sound_power));

	if (sound_power >= GetSoundThreshold())
	{
		const CEntityAlive* current_enemy = m_stalker->memory().EnemyManager().selected();
		CEntityAlive* sound_entity_alive = smart_cast<CEntityAlive*>(object);

		if (!m_stalker) // if we are not NPC
		{
			add(object, sound_type, position, sound_power);
		}
		else if (current_enemy && !m_object->memory().VisualMManager().visible_now(current_enemy) && m_stalker->Position().distance_to(current_enemy->Position()) >= m_stalker->Position().distance_to(object->Position()))
		{ // add sound, if sound is closer than current enemy and current enemy is not visible right now
			add(object, sound_type, position, sound_power);

			if (sound_entity_alive && (m_object->ID() != sound_entity_alive->ID()))
			{ 
				if ((sound_entity_alive->g_Team() != self_entity_alive->g_Team())) // if im an enemy - use hit memory to momentaraly focus on me
				{
					const CAI_Stalker* stalker = smart_cast<const CAI_Stalker*>(sound_entity_alive);

					if (stalker)
						if(!stalker->wounded())
							m_object->memory().HitMManager().add(sound_entity_alive);
					else
						m_object->memory().HitMManager().add(sound_entity_alive);
					
				}
			}
		}
		else
		{
			add(object, sound_type, position, sound_power);

			if (sound_entity_alive && sound_entity_alive->g_Team() == self_entity_alive->g_Team() && (m_object->ID() != sound_entity_alive->ID())) // if NPC hears ally which is having a selected enemy - add his visualized enemy to NPC mem if he is not in danger
			{
				CAI_Stalker* ally = smart_cast<CAI_Stalker*>(sound_entity_alive);

				if (ally && ally->memory().EnemyManager().selected())
				{
					const CAI_Stalker* slected_stalker = smart_cast<const CAI_Stalker*>(ally->memory().EnemyManager().selected());

					if (!slected_stalker || (slected_stalker && !slected_stalker->wounded()))
					{
						const CVisibleObject* obj = ally->memory().VisualMManager().GetIfIsInPool(ally->memory().EnemyManager().selected());

						if (obj)
						{
							m_object->memory().VisualMManager().add_visible_object(*obj, true);

							//Msg("Adding visible from ally being indangered 2 %s", ally->memory().EnemyManager().selected()->ObjectName().c_str());
						}
					}
				}
			}
		}
	}

	m_last_sound_time = EngineTimeU();

	m_sound_threshold = _max(m_sound_threshold, sound_power);

	VERIFY(_valid(m_sound_threshold));
}

void CSoundMemoryManager::add(const CSoundObject &sound_object, bool check_for_existance)
{
	if (check_for_existance)
	{
		if (GetMemorizedSoundsP()->end() != std::find(GetMemorizedSoundsP()->begin(), GetMemorizedSoundsP()->end(), object_id(sound_object.m_object)))
			return;
	}

	if (m_max_sound_count <= GetMemorizedSoundsP()->size())
	{
		xr_vector<CSoundObject>::iterator I = std::min_element(GetMemorizedSoundsP()->begin(), GetMemorizedSoundsP()->end(), SLevelTimePredicate<CGameObject>());

		VERIFY(GetMemorizedSoundsP()->end() != I);

		*I = sound_object;
	}
	else
		GetMemorizedSoundsP()->push_back(sound_object);
}

void CSoundMemoryManager::add(const CObject *object, int sound_type, const Fvector &position, float sound_power)
{
#ifndef SAVE_OWN_SOUNDS
	// we do not want to save our own sounds
	if (object && (m_object->ID() == object->ID()))
		return;
#endif

#ifndef SAVE_OWN_ITEM_SOUNDS
	// we do not want to save the sounds which was from the items we own
	if (object && object->H_Parent() && (object->H_Parent()->ID() == m_object->ID()))
		return;
#endif

#ifndef SAVE_NON_ALIVE_OBJECT_SOUNDS
	// we do not want to save sounds from the non-alive objects (?!)
	if (object && !m_object->memory().enemy().selected() && !smart_cast<const CEntityAlive*>(object))
		return;
#endif

#ifndef SAVE_FRIEND_ITEM_SOUNDS
	// we do not want to save sounds from the teammates items
	CEntityAlive	*me				= m_object;
	if (object && object->H_Parent() && (me->tfGetRelationType(smart_cast<const CEntityAlive*>(object->H_Parent())) == ALife::eRelationTypeFriend))
		return;
#endif

#ifndef SAVE_FRIEND_SOUNDS
	const CEntityAlive	*entity_alive	= smart_cast<const CEntityAlive*>(object);

	// we do not want to save sounds from the teammates
	if (entity_alive && me && (me->tfGetRelationType(entity_alive) == ALife::eRelationTypeFriend))
		return;
#endif

#ifndef SAVE_VISIBLE_OBJECT_SOUNDS
#	ifdef SAVE_FRIEND_SOUNDS
//		const CEntityAlive	*entity_alive	= smart_cast<const CEntityAlive*>(object);
#	endif

	// we do not save sounds from the objects we see (?!)
	//	if (m_object->memory().VisualMManager().visible_now(entity_alive))
	//		return;
#endif

	const CGameObject* game_object = smart_cast<const CGameObject*>(object);

	if (!game_object && object)
		return;

	const CGameObject* self = m_object;

	xr_vector<CSoundObject>::iterator J = std::find(GetMemorizedSoundsP()->begin(), GetMemorizedSoundsP()->end(), object_id(object));

	if (GetMemorizedSoundsP()->end() == J)
	{
		CSoundObject sound_object;

		sound_object.fill(game_object,self,ESoundTypes(sound_type),sound_power,!m_stalker ? squad_mask_type(-1) : m_stalker->agent_manager().member().mask(m_stalker));

		if (!game_object)
			sound_object.m_object_params.GetMemorizedPos() = position;

#ifdef USE_FIRST_GAME_TIME
		sound_object.m_first_game_time	= GetGameTime();
#endif

#ifdef USE_FIRST_LEVEL_TIME
		sound_object.m_first_level_time	= EngineTimeU();
#endif

		add(sound_object);
	}
	else
	{
		(*J).fill(game_object,self,ESoundTypes(sound_type),sound_power,(!m_stalker ? (*J).m_squad_mask.get() : ((*J).m_squad_mask.get() | m_stalker->agent_manager().member().mask(m_stalker))));

		if (!game_object)
			(*J).m_object_params.GetMemorizedPos() = position;
	}
}

struct CRemoveOfflinePredicate
{
	bool operator()	(const CSoundObject &object) const
	{
		if (!object.m_object)
			return (false);

		return (!!object.m_object->H_Parent());
	}
};

void CSoundMemoryManager::UpdateStoredHeard()
{
	storedHeardAllies_.clear();
	storedHeardEnemies_.clear();

	xr_vector<CSoundObject>::const_iterator I = GetMemorizedSoundsP()->begin();
	xr_vector<CSoundObject>::const_iterator E = GetMemorizedSoundsP()->end();

	for (; I != E; ++I)
	{
		auto mem = (*I);
		const CEntityAlive* ea = smart_cast<const CEntityAlive*>(mem.m_object);

		if (ea)
		{
			if (m_object->is_relation_enemy(ea))
				storedHeardEnemies_.push_back(mem);
			else
				storedHeardAllies_.push_back(mem);
		}
	}
}

void CSoundMemoryManager::update()
{
	START_PROFILE("Memory Manager/sounds::update")

	clear_delayed_objects();

	VERIFY(GetMemorizedSoundsP());

	GetMemorizedSoundsP()->erase(std::remove_if(GetMemorizedSoundsP()->begin(), GetMemorizedSoundsP()->end(), CRemoveOfflinePredicate()), GetMemorizedSoundsP()->end());

#ifdef USE_SELECTED_SOUND
	xr_delete(m_selected_sound);

	u32	priority = u32(-1);

	xr_vector<CSoundObject>::const_iterator	I = GetMemorizedSoundsP()->begin();
	xr_vector<CSoundObject>::const_iterator	E = GetMemorizedSoundsP()->end();

	for ( ; I != E; ++I)
	{
		u32	cur_priority = this->priority(*I);

		if (cur_priority < priority)
		{
			m_selected_sound	= xr_new <CSoundObject>(*I);
			priority			= cur_priority;
		}
	}
#endif

	R_ASSERT2(GetMemorizedSoundsP()->size() <= m_max_sound_count, make_string("cur %u max %u", GetMemorizedSoundsP()->size(), m_max_sound_count));

	if (GetMemorizedSoundsP()->size() > m_max_sound_count)
		GetMemorizedSoundsP()->clear();

	STOP_PROFILE
}

struct CSoundObjectPredicate
{
	const CObject* m_object;

	CSoundObjectPredicate(const CObject *object) : m_object(object)
	{
	}

	bool operator()(const MemorySpace::CSoundObject &sound_object) const
	{
		if (!m_object)
			return false;

		if (!sound_object.m_object)
			return false;

		return (m_object->ID() == sound_object.m_object->ID());
	}
};


const CSoundObject* CSoundMemoryManager::GetIfIsInPool(const CGameObject *game_object)
{
	R_ASSERT2(GetMemorizedSoundsP()->size() <= m_max_sound_count, make_string("cur %u max %u", GetMemorizedSoundsP()->size(), m_max_sound_count));

	SOUNDS::const_iterator I = std::find_if(GetMemorizedSoundsP()->begin(), GetMemorizedSoundsP()->end(), CSoundObjectPredicate(game_object));

	if ((GetMemorizedSounds().end() == I))
		return nullptr;

	const CSoundObject* object_from_pull = (&*I);

	if (object_from_pull->m_object)
		R_ASSERT(object_from_pull->m_object->SectionNameStr());

	return object_from_pull;
}


void CSoundMemoryManager::remove_links(CObject *object)
{
	VERIFY(GetMemorizedSoundsP());

	SOUNDS::iterator I = std::find_if(GetMemorizedSoundsP()->begin(), GetMemorizedSoundsP()->end(), CSoundObjectPredicate(object));

	if (I != GetMemorizedSoundsP()->end())
		GetMemorizedSoundsP()->erase(I);

#ifdef USE_SELECTED_SOUND
	if (!m_selected_sound)
		return;
	
	if (!m_selected_sound->m_object)
		return;
	
	if (m_selected_sound->m_object->ID() != object->ID())
		return;

	xr_delete(m_selected_sound);
#endif
}

#define SAVE_FAINT_POWER 5.f
#define SAVE_DISTANCE_IF_FAINT 12.f
#define SAVE_DISTANCE_IF_NOT_SELF_HEARD 25.f
#define SAVE_ALIES_SNDS_COUNT 3

// A function for sorting valuable onjects in memory for saving. Beware, dont save a lot objects, since NetPacket is not a stretching rubber =)
static inline bool is_object_valuable_to_save(CCustomMonster const* const self, MemorySpace::CSoundObject const& object)
{
	CEntityAlive const* const entity_alive = smart_cast<CEntityAlive const*>(object.m_object);

	if (!entity_alive)
		return false;

	if (!entity_alive->g_Alive())
		return false;

	if (entity_alive == self->memory().EnemyManager().selected())
		return true;

	if (!self->is_relation_enemy(entity_alive) && self->memory().SoundMManager().countOfSavedAlliesSounds_ < SAVE_ALIES_SNDS_COUNT) // Save some allies sounds, to avoid entering panic after save\load
	{
		self->memory().SoundMManager().countOfSavedAlliesSounds_++;

		return true;
	}

	float distance_from_me = self->Position().distance_to(object.m_object_params.GetMemorizedPos());

	if (self != object.m_deriving_smemory_owner && distance_from_me > SAVE_DISTANCE_IF_NOT_SELF_HEARD)
		return false;

	if (object.m_power < SAVE_FAINT_POWER && distance_from_me > SAVE_DISTANCE_IF_FAINT)
		return false;

	return self->is_relation_enemy(entity_alive);
}

void CSoundMemoryManager::save(NET_Packet &packet)
{
	if (!m_object->g_Alive())
		return;

	SOUNDS::const_iterator II = GetMemorizedSounds().begin();
	SOUNDS::const_iterator EE = GetMemorizedSounds().end();

	countOfSavedAlliesSounds_ = 0;
	sObjectsForSaving_.clear();

	for (; II != EE; ++II)
	{
		if (is_object_valuable_to_save(m_object, *II))
		{
			sObjectsForSaving_.push_back(*II);
		}
	}

	R_ASSERT(sObjectsForSaving_.size() < 255);

	packet.w_u8((u8)sObjectsForSaving_.size());

	if (!sObjectsForSaving_.size())
		return;

	for (u32 i = 0; i < sObjectsForSaving_.size(); i++)//VISIBLES::const_iterator I = vObjectsForSaving_.begin(); I != vObjectsForSaving_.end(); ++I)
	{
		CSoundObject* mem_obj = &sObjectsForSaving_[i];

		R_ASSERT(mem_obj);
		R_ASSERT(mem_obj->m_object);

		packet.w_u16			(mem_obj->m_object->ID());
		// object params
		packet.w_u32			(mem_obj->m_object_params.m_level_vertex_id);
		packet.w_vec3			(mem_obj->m_object_params.GetMemorizedPos());

#ifdef USE_ORIENTATION
		packet.w_float			(mem_obj->m_object_params.m_orientation.yaw);
		packet.w_float			(mem_obj->m_object_params.m_orientation.pitch);
		packet.w_float			(mem_obj->m_object_params.m_orientation.roll);
#endif

		// self params
		packet.w_u32			(mem_obj->m_self_params.m_level_vertex_id);
		packet.w_vec3			(mem_obj->m_self_params.GetMemorizedPos());

#ifdef USE_ORIENTATION
		packet.w_float			(mem_obj->m_self_params.m_orientation.yaw);
		packet.w_float			(mem_obj->m_self_params.m_orientation.pitch);
		packet.w_float			(mem_obj->m_self_params.m_orientation.roll);
#endif

#ifdef USE_LEVEL_TIME
		packet.w_u32			((EngineTimeU() >= mem_obj->m_level_time) ? (EngineTimeU() - mem_obj->m_level_time) : 0);
		packet.w_u32			((EngineTimeU() >= mem_obj->m_level_time) ? (EngineTimeU() - mem_obj->m_last_level_time) : 0);
#endif

#ifdef USE_FIRST_LEVEL_TIME
		packet.w_u32			((EngineTimeU() >= mem_obj->m_level_time) ? (EngineTimeU() - mem_obj->m_first_level_time) : 0);
#endif

		packet.w_u32			(mem_obj->m_sound_type);
		packet.w_float			(mem_obj->m_power);

		packet.w_u32			(mem_obj->m_deriving_smemory_owner ? mem_obj->m_deriving_smemory_owner->ID(): u32(-1));
	}

	sObjectsForSaving_.clear();
}

void CSoundMemoryManager::load(IReader &packet)
{
	if (!m_object->g_Alive())
		return;

	typedef CClientSpawnManager::CALLBACK_TYPE CALLBACK_TYPE;

	CALLBACK_TYPE callback;
	callback.bind (&m_object->memory(),&CMemoryManager::on_requested_spawn);

	int	count = packet.r_u8();

	for (int i=0; i<count; ++i)
	{
		CDelayedSoundObject			delayed_object;
		delayed_object.m_object_id	= packet.r_u16();

		CSoundObject				&object = delayed_object.m_sound_object;
		if (delayed_object.m_object_id != ALife::_OBJECT_ID(-1))
			object.m_object			= smart_cast<CGameObject*>(Level().Objects.net_Find(delayed_object.m_object_id));
		else
			object.m_object			= 0;

		// object params
		object.m_object_params.m_level_vertex_id = packet.r_u32();

		Fvector temp;
		packet.r_fvector3			(temp);

		object.m_object_params.SetMemorizedPos(temp);

#ifdef USE_ORIENTATION
		packet.r_float				(object.m_object_params.m_orientation.yaw);
		packet.r_float				(object.m_object_params.m_orientation.pitch);
		packet.r_float				(object.m_object_params.m_orientation.roll);
#endif
		// self params
		object.m_self_params.m_level_vertex_id	= packet.r_u32();

		temp;
		packet.r_fvector3			(temp);

		object.m_self_params.SetMemorizedPos(temp);

#ifdef USE_ORIENTATION
		packet.r_float				(object.m_self_params.m_orientation.yaw);
		packet.r_float				(object.m_self_params.m_orientation.pitch);
		packet.r_float				(object.m_self_params.m_orientation.roll);
#endif

#ifdef USE_LEVEL_TIME

		VERIFY						(EngineTimeU() >= object.m_level_time);

		object.m_level_time			= packet.r_u32();
		object.m_level_time			= EngineTimeU() - object.m_level_time;
#endif

#ifdef USE_LAST_LEVEL_TIME

		VERIFY						(EngineTimeU() >= object.m_last_level_time);

		object.m_last_level_time	= packet.r_u32();
		object.m_last_level_time	= EngineTimeU() - object.m_last_level_time;
#endif

#ifdef USE_FIRST_LEVEL_TIME
		VERIFY						(EngineTimeU() >= (*I).m_first_level_time);
		object.m_first_level_time	= packet.r_u32();
		object.m_first_level_time	+= EngineTimeU();
#endif

		object.m_sound_type			= (ESoundTypes)packet.r_u32();
		object.m_power				= packet.r_float();

		u32 group_owner_of_this_memory_obj = packet.r_u32();
		if (group_owner_of_this_memory_obj != u32(-1))
			object.m_deriving_smemory_owner = smart_cast<CEntityAlive*>(Level().Objects.net_Find(group_owner_of_this_memory_obj));

		if (object.m_object)
		{
			add(object,true);

			continue;
		}

		m_delayed_objects.push_back(delayed_object);

		const CClientSpawnManager::CSpawnCallback* spawn_callback = Level().client_spawn_manager().callback(delayed_object.m_object_id, m_object->ID());

		if (!spawn_callback || !spawn_callback->m_object_callback)
			Level().client_spawn_manager().add_spawn_callback(delayed_object.m_object_id, m_object->ID(), callback);

#ifdef DEBUG
		else 
		{
			if (spawn_callback && spawn_callback->m_object_callback)
			{
				VERIFY(spawn_callback->m_object_callback == callback);
			}
		}
#endif
	}
}

void CSoundMemoryManager::clear_delayed_objects()
{
	if (m_delayed_objects.empty())
		return;

	CClientSpawnManager& manager = Level().client_spawn_manager();

	DELAYED_SOUND_OBJECTS::const_iterator I = m_delayed_objects.begin();
	DELAYED_SOUND_OBJECTS::const_iterator E = m_delayed_objects.end();

	for ( ; I != E; ++I)
		if (manager.callback((*I).m_object_id,m_object->ID()))
			manager.remove_spawn_callback((*I).m_object_id, m_object->ID());

	m_delayed_objects.clear();
}

void CSoundMemoryManager::on_requested_spawn(CObject *object)
{
	DELAYED_SOUND_OBJECTS::iterator	I = m_delayed_objects.begin();
	DELAYED_SOUND_OBJECTS::iterator	E = m_delayed_objects.end();

	for (; I != E; ++I)
	{
		if ((*I).m_object_id != object->ID())
			continue;

		if (m_object->g_Alive())
		{
			(*I).m_sound_object.m_object = smart_cast<CGameObject*>(object);

			VERIFY((*I).m_sound_object.m_object);

			add((*I).m_sound_object, true);
		}

		m_delayed_objects.erase(I);

		return;
	}
}
