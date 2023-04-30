////////////////////////////////////////////////////////////////////////////
//	Module 		: hit_memory_manager.cpp
//	Created 	: 02.10.2001
//  Modified 	: 19.11.2003
//	Author		: Dmitriy Iassenev
//	Description : Hit memory manager
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "hit_memory_manager.h"
#include "memory_space_impl.h"
#include "custommonster.h"
#include "script_callback_ex.h"
#include "script_game_object.h"
#include "agent_manager.h"
#include "agent_member_manager.h"
#include "ai/stalker/ai_stalker.h"
#include "game_object_space.h"
#include "profiler.h"
#include "client_spawn_manager.h"
#include "memory_manager.h"
#include "actor.h"
#include "level.h"
#include "enemy_manager.h"
#include "ai\stalker\ai_stalker_impl.h"
struct CHitObjectPredicate {
	const CObject *m_object;

				CHitObjectPredicate			(const CObject *object) :
					m_object				(object)
	{
	}

	bool		operator()					(const MemorySpace::CHitObject &hit_object) const
	{
		if (!m_object)
			return false;

		if (!hit_object.m_object)
			return false;

		return (m_object->ID() == hit_object.m_object->ID());
	}
};

CHitMemoryManager::~CHitMemoryManager		()
{
	clear_delayed_objects	();

#ifdef USE_SELECTED_HIT
	xr_delete				(m_selected_hit);
#endif
}

const CHitObject *CHitMemoryManager::hit					(const CEntityAlive *object) const
{
	HITS::const_iterator	I = std::find_if(GetMemorizedHits().begin(), GetMemorizedHits().end(), CHitObjectPredicate(object));
	if (GetMemorizedHits().end() != I)
		return				(&*I);

	return					(0);
}

void CHitMemoryManager::add					(const CEntityAlive *entity_alive)
{
	add						(0,Fvector().set(0,0,1),entity_alive,0);
}

void CHitMemoryManager::LoadCfg				(LPCSTR section)
{
}

void CHitMemoryManager::reinit				()
{
	m_last_hit_object_id	= ALife::_OBJECT_ID(-1);
	m_last_hit_time			= 0;
}

void CHitMemoryManager::reload				(LPCSTR section)
{
#ifdef USE_SELECTED_HIT
	xr_delete				(m_selected_hit);
#endif
	m_max_hit_count = READ_IF_EXISTS(pSettings, r_s32, section, "DynamicHitCount", 1);

	forgetHitTime_ = READ_IF_EXISTS(pSettings, r_s32, section, "forget_hit_time", 60);
	forgetHitTime_ *= 1000;
}

extern Flags32 psActorFlags;
void CHitMemoryManager::add					(float amount, const Fvector &vLocalDir, const CObject *who, s16 element)
{
	VERIFY(GetMemorizedHitsP());

	if (who && (m_object->ID() == who->ID()))
		return;

	const CActor 	*actor = smart_cast<const CActor*>(who);
	if (who && actor && psActorFlags.test(AF_INVISIBLE))
		return;

	if (who && !fis_zero(amount))
		object().callback(GameObject::eHit)(
			m_object->lua_game_object(), 
			amount,
			vLocalDir,
			smart_cast<const CGameObject*>(who)->lua_game_object(),
			element
		);

	if (!object().g_Alive())
		return;

	if (who && !fis_zero(amount)) {
		m_last_hit_object_id	= who->ID();
		m_last_hit_time			= EngineTimeU();
	}

	Fvector						direction;
	m_object->XFORM().transform_dir	(direction,vLocalDir);

	const CEntityAlive			*entity_alive = smart_cast<const CEntityAlive*>(who);
	if (!entity_alive || (m_object->tfGetRelationType(entity_alive) == ALife::eRelationTypeFriend))
		return;

	HITS::iterator				J = std::find(GetMemorizedHitsP()->begin(), GetMemorizedHitsP()->end(), object_id(who));
	if (GetMemorizedHitsP()->end() == J) {
		CHitObject						hit_object;

		hit_object.fill					(entity_alive,m_object,!m_stalker ? squad_mask_type(-1) : m_stalker->agent_manager().member().mask(m_stalker));
		
#ifdef USE_FIRST_GAME_TIME
		hit_object.m_first_game_time	= GetGameTime();
#endif
#ifdef USE_FIRST_LEVEL_TIME
		hit_object.m_first_level_time	= EngineTimeU();
#endif
		hit_object.m_amount				= amount;

		if (m_max_hit_count <= GetMemorizedHitsP()->size()) {
			HITS::iterator		I = std::min_element(GetMemorizedHitsP()->begin(), GetMemorizedHitsP()->end(), SLevelTimePredicate<CEntityAlive>());
			VERIFY				(GetMemorizedHitsP()->end() != I);
			*I					= hit_object;
		}
		else
			GetMemorizedHitsP()->push_back(hit_object);
	}
	else {
		(*J).fill				(entity_alive,m_object,(!m_stalker ? (*J).m_squad_mask.get() : ((*J).m_squad_mask.get() | m_stalker->agent_manager().member().mask(m_stalker))));
		(*J).m_amount			= _max(amount,(*J).m_amount);
	}
}

void CHitMemoryManager::add					(const CHitObject &_hit_object)
{
	VERIFY(GetMemorizedHitsP());
	if (!object().g_Alive())
		return;

	const CActor 	*actor = smart_cast<const CActor*>(_hit_object.m_object);
	if (_hit_object.m_object && actor && psActorFlags.test(AF_INVISIBLE))
		return;

	CHitObject					hit_object = _hit_object;
	squad_mask_type mask = squad_mask_type(-1);
	if (m_stalker)
		mask = m_stalker->agent_manager().member().mask(m_stalker);
	hit_object.m_squad_mask.set	(mask,TRUE);

	const CEntityAlive			*entity_alive = hit_object.m_object;
	HITS::iterator	J = std::find(GetMemorizedHitsP()->begin(), GetMemorizedHitsP()->end(), object_id(entity_alive));
	if (GetMemorizedHitsP()->end() == J) {
		if (m_max_hit_count <= GetMemorizedHitsP()->size()) {
			HITS::iterator	I = std::min_element(GetMemorizedHitsP()->begin(), GetMemorizedHitsP()->end(), SLevelTimePredicate<CEntityAlive>());
			VERIFY(GetMemorizedHitsP()->end() != I);
			*I					= hit_object;
		}
		else
			GetMemorizedHitsP()->push_back(hit_object);
	}
	else {
		hit_object.m_squad_mask.assign	(hit_object.m_squad_mask.get() | (*J).m_squad_mask.get());
		*J						= hit_object;
	}
}

const CHitObject* CHitMemoryManager::GetIfIsInPool(const CGameObject *game_object)
{
	R_ASSERT2(GetMemorizedHitsP()->size() <= m_max_hit_count, make_string("cur %u max %u", GetMemorizedHitsP()->size(), m_max_hit_count));

	HITS::const_iterator I = std::find_if(GetMemorizedHitsP()->begin(), GetMemorizedHitsP()->end(), CHitObjectPredicate(game_object));

	if ((GetMemorizedHits().end() == I))
		return nullptr;

	const CHitObject* object_from_pull = (&*I);

	if (object_from_pull->m_object)
		R_ASSERT(object_from_pull->m_object->SectionNameStr());

	return object_from_pull;
}

struct CRemoveOfflinePredicate {
	bool		operator()						(const CHitObject &object) const
	{
		VERIFY	(object.m_object);
		return	( !object.m_object || !!object.m_object->getDestroy() || object.m_object->H_Parent() );
	}
};

void CHitMemoryManager::update()
{
	START_PROFILE("Memory Manager/hits::update")

	clear_delayed_objects		();

	VERIFY(GetMemorizedHitsP());
	GetMemorizedHitsP()->erase(
		std::remove_if(	
		GetMemorizedHitsP()->begin(),
		GetMemorizedHitsP()->end(),
			CRemoveOfflinePredicate()
		),
		GetMemorizedHitsP()->end()
	);

#ifdef USE_SELECTED_HIT
	xr_delete					(m_selected_hit);
	u32							level_time = 0;
	HITS::const_iterator		I = GetMemorizedHitsP()->begin();
	HITS::const_iterator		E = GetMemorizedHitsP()->end();
	for ( ; I != E; ++I) {
		if ((*I).m_level_time > level_time) {
			xr_delete			(m_selected_hit);
			m_selected_hit		= xr_new <CHitObject>(*I);
			level_time			= (*I).m_level_time;
		}
	}
#endif
	STOP_PROFILE

	R_ASSERT2(GetMemorizedHitsP()->size() <= m_max_hit_count, make_string("cur %u max %u", GetMemorizedHitsP()->size(), m_max_hit_count));

	if (GetMemorizedHitsP()->size() > m_max_hit_count)
		GetMemorizedHitsP()->clear();
}

void CHitMemoryManager::enable			(const CObject *object, bool enable)
{
	HITS::iterator				J = std::find(GetMemorizedHitsP()->begin(), GetMemorizedHitsP()->end(), object_id(object));
	if (J == GetMemorizedHitsP()->end())
		return;

	(*J).m_enabled				= enable;
}

void CHitMemoryManager::remove_links	(CObject *object)
{
	if (m_last_hit_object_id == object->ID()) {
		m_last_hit_object_id	= ALife::_OBJECT_ID(-1);
		m_last_hit_time			= 0;
	}

	VERIFY(GetMemorizedHitsP());
	HITS::iterator				I = std::find_if(GetMemorizedHitsP()->begin(), GetMemorizedHitsP()->end(), CHitObjectPredicate(object));
	if (I != GetMemorizedHitsP()->end())
		GetMemorizedHitsP()->erase(I);

#ifdef USE_SELECTED_HIT
	if (!m_selected_hit)
		return;

	if (!m_selected_hit->m_object)
		return;

	if (m_selected_hit->m_object->ID() != object->ID())
		return;

	xr_delete			(m_selected_hit);
#endif
}

#define SAVE_DISTANCE_IF_NOT_SELF_HITED 25.f

// A function for sorting valuable objects in memory for saving. Beware, dont save a lot objects, since NetPacket is not a stretching rubber =)
static inline bool is_object_valuable_to_save(CCustomMonster const* const self, MemorySpace::CHitObject const& object)
{
	CEntityAlive const* const entity_alive = smart_cast<CEntityAlive const*>(object.m_object);

	if (!entity_alive)
		return false;

	if (!entity_alive->g_Alive())
		return false;

	if (entity_alive == self->memory().EnemyManager().selected())
		return true;

	float distance_from_me = self->Position().distance_to(object.m_object_params.GetMemorizedPos());

	if (self != object.m_deriving_hmemory_owner && distance_from_me > SAVE_DISTANCE_IF_NOT_SELF_HITED)
		return false;

	return self->is_relation_enemy(entity_alive);
}

void CHitMemoryManager::save	(NET_Packet &packet)
{
	if (!m_object->g_Alive())
		return;

	HITS::const_iterator		II = GetMemorizedHits().begin();
	HITS::const_iterator		EE = GetMemorizedHits().end();

	hObjectsForSaving_.clear();

	for (; II != EE; ++II)
	{
		if (is_object_valuable_to_save(m_object, *II))
		{
			hObjectsForSaving_.push_back(*II);
		}
	}

	R_ASSERT(hObjectsForSaving_.size() < 255);

	packet.w_u8((u8)hObjectsForSaving_.size());

	if (!hObjectsForSaving_.size())
		return;

	for (u32 i = 0; i < hObjectsForSaving_.size(); i++)//VISIBLES::const_iterator I = vObjectsForSaving_.begin(); I != vObjectsForSaving_.end(); ++I)
	{
		CHitObject* mem_obj = &hObjectsForSaving_[i];

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
#endif // USE_ORIENTATION
		// self params
		packet.w_u32			(mem_obj->m_self_params.m_level_vertex_id);
		packet.w_vec3			(mem_obj->m_self_params.GetMemorizedPos());
#ifdef USE_ORIENTATION
		packet.w_float			(mem_obj->m_self_params.m_orientation.yaw);
		packet.w_float			(mem_obj->m_self_params.m_orientation.pitch);
		packet.w_float			(mem_obj->m_self_params.m_orientation.roll);
#endif // USE_ORIENTATION
#ifdef USE_LEVEL_TIME
		packet.w_u32			((EngineTimeU() >= mem_obj->m_level_time) ? (EngineTimeU() - mem_obj->m_level_time) : 0);
#endif // USE_LAST_LEVEL_TIME
#ifdef USE_LEVEL_TIME
		packet.w_u32			((EngineTimeU() >= mem_obj->m_level_time) ? (EngineTimeU() - mem_obj->m_last_level_time) : 0);
#endif // USE_LAST_LEVEL_TIME
#ifdef USE_FIRST_LEVEL_TIME
		packet.w_u32			((EngineTimeU() >= mem_obj->m_level_time) ? (EngineTimeU() - mem_obj->m_first_level_time) : 0);
#endif // USE_FIRST_LEVEL_TIME
		packet.w_vec3			(mem_obj->m_direction);
		packet.w_u16			(mem_obj->m_bone_index);
		packet.w_float			(mem_obj->m_amount);

		packet.w_u32			(mem_obj->m_deriving_hmemory_owner ? mem_obj->m_deriving_hmemory_owner->ID(): u32(-1));
	}

	hObjectsForSaving_.clear();
}

void CHitMemoryManager::load	(IReader &packet)
{
	if (!m_object->g_Alive())
		return;

	typedef CClientSpawnManager::CALLBACK_TYPE	CALLBACK_TYPE;
	CALLBACK_TYPE					callback;
	callback.bind					(&m_object->memory(),&CMemoryManager::on_requested_spawn);

	int								count = packet.r_u8();
	for (int i=0; i<count; ++i) {
		CDelayedHitObject			delayed_object;
		delayed_object.m_object_id	= packet.r_u16();

		CHitObject					&object = delayed_object.m_hit_object;
		object.m_object				= smart_cast<CEntityAlive*>(Level().Objects.net_Find(delayed_object.m_object_id));
		// object params
		object.m_object_params.m_level_vertex_id	= packet.r_u32();

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
#endif // USE_LEVEL_TIME
#ifdef USE_LAST_LEVEL_TIME
		VERIFY						(EngineTimeU() >= object.m_last_level_time);
		object.m_last_level_time	= packet.r_u32();
		object.m_last_level_time	= EngineTimeU() - object.m_last_level_time;
#endif // USE_LAST_LEVEL_TIME
#ifdef USE_FIRST_LEVEL_TIME
		VERIFY						(EngineTimeU() >= (*I).m_first_level_time);
		object.m_first_level_time	= packet.r_u32();
		object.m_first_level_time	+= EngineTimeU();
#endif // USE_FIRST_LEVEL_TIME
		packet.r_fvector3			(object.m_direction);
		object.m_bone_index			= packet.r_u16();
		object.m_amount				= packet.r_float();

		u32 group_owner_of_this_memory_obj = packet.r_u32();
		if (group_owner_of_this_memory_obj != u32(-1))
			object.m_deriving_hmemory_owner = smart_cast<CEntityAlive*>(Level().Objects.net_Find(group_owner_of_this_memory_obj));

		if (object.m_object) {
			add						(object);
			continue;
		}

		m_delayed_objects.push_back	(delayed_object);

		const CClientSpawnManager::CSpawnCallback	*spawn_callback = Level().client_spawn_manager().callback(delayed_object.m_object_id,m_object->ID());
		if (!spawn_callback || !spawn_callback->m_object_callback)
			Level().client_spawn_manager().add_spawn_callback(delayed_object.m_object_id, m_object->ID(), callback);
#ifdef DEBUG
		else {
			if (spawn_callback && spawn_callback->m_object_callback) {
				VERIFY				(spawn_callback->m_object_callback == callback);
			}
		}
#endif // DEBUG
	}
}

void CHitMemoryManager::clear_delayed_objects()
{
	if (m_delayed_objects.empty())
		return;

	CClientSpawnManager						&manager = Level().client_spawn_manager();
	DELAYED_HIT_OBJECTS::const_iterator		I = m_delayed_objects.begin();
	DELAYED_HIT_OBJECTS::const_iterator		E = m_delayed_objects.end();
	for ( ; I != E; ++I)
		if (manager.callback((*I).m_object_id,m_object->ID()))
			manager.remove_spawn_callback((*I).m_object_id, m_object->ID());

	m_delayed_objects.clear					();
}

void CHitMemoryManager::on_requested_spawn	(CObject *object)
{
	DELAYED_HIT_OBJECTS::iterator		I = m_delayed_objects.begin();
	DELAYED_HIT_OBJECTS::iterator		E = m_delayed_objects.end();
	for ( ; I != E; ++I) {
		if ((*I).m_object_id != object->ID())
			continue;
		
		if (m_object->g_Alive()) {
			(*I).m_hit_object.m_object= smart_cast<CEntityAlive*>(object);
			VERIFY						((*I).m_hit_object.m_object);
			add							((*I).m_hit_object);
		}

		m_delayed_objects.erase			(I);
		return;
	}
}
