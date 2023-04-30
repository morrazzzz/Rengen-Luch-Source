
//	Created 	: 02.10.2001
//	Author		: Dmitriy Iassenev

#include "pch_script.h"
#include "visual_memory_manager.h"
#include "ai/stalker/ai_stalker.h"
#include "memory_space_impl.h"
#include "../Include/xrRender/Kinematics.h"
#include "clsid_game.h"
#include "ai_object_location.h"
#include "level_graph.h"
#include "stalker_movement_manager_smart_cover.h"
#include "../gamemtllib.h"
#include "agent_manager.h"
#include "agent_member_manager.h"
#include "ai_space.h"
#include "profiler.h"
#include "actor.h"
#include "../camerabase.h"
#include "gamepersistent.h"
#include "actor_memory.h"
#include "client_spawn_manager.h"
#include "client_spawn_manager.h"
#include "memory_manager.h"
#include "ai/monsters/basemonster/base_monster.h"
#include "ai/monsters/zombie/zombie.h"
#include "actor.h"
#include "clsid_game.h"
#include "level.h"
#include "GameConstants.h"
#include "enemy_manager.h"
#include "ai\stalker\ai_stalker_impl.h"
#define VISIBLE_NOW_OK_TIME 3000 // makes NPC continue fire at enemy, even if there is no direct eye contact

struct SRemoveOfflinePredicate
{
	bool operator() (const CVisibleObject &object) const
	{
		if (!object.m_object)
			return true;

		R_ASSERT(object.m_object);

		return	(!!object.m_object->getDestroy() || object.m_object->H_Parent());
	}
	
	bool operator() (const CNotYetVisibleObject &object) const
	{
		if (!object.m_object)
			return true;

		R_ASSERT(object.m_object);

		return	(!!object.m_object->getDestroy() || object.m_object->H_Parent());
	}
};

struct CVisibleObjectPredicate
{
	u32	m_id;

	CVisibleObjectPredicate	(u32 id) : 	m_id(id)
	{
	}

	bool operator() (const CObject *object) const
	{
		VERIFY	(object);

		return	(object->ID() == m_id);
	}
};

struct CNotYetVisibleObjectPredicate
{
	const CGameObject *m_game_object;

	IC CNotYetVisibleObjectPredicate(const CGameObject *game_object)
	{
		m_game_object = game_object;
	}

	IC bool operator() (const CNotYetVisibleObject &object) const
	{
		return(object.m_object->ID() == m_game_object->ID());
	}
};

struct CVisibleObjectPredicateEx
{
	const CObject *m_object;

	CVisibleObjectPredicateEx(const CObject *object) :
		m_object(object)
	{
	}

	bool operator() (const MemorySpace::CVisibleObject &visible_object) const
	{
		if (!m_object)
			return false;

		if (!visible_object.m_object)
			return false;

		return(m_object->ID() == visible_object.m_object->ID());
	}

	bool operator() (const MemorySpace::CNotYetVisibleObject &not_yet_visible_object) const
	{
		if (!m_object)
			return false;

		if (!not_yet_visible_object.m_object)
			return false;

		return(m_object->ID() == not_yet_visible_object.m_object->ID());
	}
};

CVisualMemoryManager::CVisualMemoryManager(CCustomMonster *object)
{
	m_object			= object;
	m_stalker			= 0;
	m_client			= 0;

	initialize();
}

CVisualMemoryManager::CVisualMemoryManager(CAI_Stalker *stalker)
{
	m_object			= stalker;
	m_stalker			= stalker;
	m_client			= 0;

	initialize();
}

CVisualMemoryManager::CVisualMemoryManager(vision_client *client)
{
	m_object			= 0;
	m_stalker			= 0;
	m_client			= client;

	initialize();
}

void CVisualMemoryManager::initialize()
{
	visibleObjects_.clear();

	m_max_object_count	= 128;
	m_enabled			= true;

	countOfSavedVisibleAllies_ = 0;

	ignoreVisMemorySharing_ = false;
}

CVisualMemoryManager::~CVisualMemoryManager()
{
	clear_delayed_objects	();

	if (!m_client)
		return;
}

void CVisualMemoryManager::reinit()
{
	VERIFY(GetMemorizedObjectsP());

	GetMemorizedObjectsP()->clear();

	m_visible_objects.clear();
//	m_visible_objects.reserve			(100);

	m_not_yet_visible_objects.clear();
//	m_not_yet_visible_objects.reserve	(100);

	if (m_object)
		m_object->feel_vision_clear();

	m_last_update_time = u32(-1);
}

void CVisualMemoryManager::reload(LPCSTR section)
{
	m_max_object_count	= READ_IF_EXISTS(pSettings, r_s32, section, "DynamicObjectsCount", 128);

	forgetVisTime_		= READ_IF_EXISTS(pSettings, r_s32, section, "forget_vis_time", 60);
	forgetVisTime_		*= 1000;

	ignoreVisMemorySharing_ = !!READ_IF_EXISTS(pSettings, r_bool, section, "ignore_vis_memory_sharing", FALSE);

	if (m_stalker)
	{
		m_free.LoadCfg		(pSettings->r_string(section, "vision_free_section"), true);
		m_danger.LoadCfg	(pSettings->r_string(section, "vision_danger_section"), true);
	}
	else if (m_object)
	{
		m_free.LoadCfg		(pSettings->r_string(section, "vision_free_section"), !!m_client);
		m_danger.LoadCfg	(pSettings->r_string(section, "vision_danger_section"), !!m_client);
	}
	else
	{
		m_free.LoadCfg		(section, !!m_client);
	}

	if (m_stalker)
		UpdateVisability_RankDependance();
}

void CVisualMemoryManager::UpdateVisability_RankDependance()
{
	if (m_stalker) // stalkers. exclude actor
	{
		LPCSTR section = m_stalker->SectionName().c_str();

		m_stalker->cast_inventory_owner();
		CInventoryOwner* owner = m_stalker ? m_stalker->cast_inventory_owner() : m_object ? m_object->cast_inventory_owner() : nullptr;

		if (owner)
		{
			// Load visibility dependance from rank
			if (owner->GetIORank() >= GameConstants::GetMasterRankStart())
			{
				freeRankCoef_.LoadCfg(pSettings->r_string(section, "vision_free_master_K"));
				dangerRankCoef_.LoadCfg(pSettings->r_string(section, "vision_danger_master_K"));
			}
			else if (owner->GetIORank() >= GameConstants::GetVeteranRankStart())
			{
				freeRankCoef_.LoadCfg(pSettings->r_string(section, "vision_free_veteran_K"));
				dangerRankCoef_.LoadCfg(pSettings->r_string(section, "vision_danger_veteran_K"));
			}
			else if (owner->GetIORank() >= GameConstants::GetExperiencesRankStart())
			{
				freeRankCoef_.LoadCfg(pSettings->r_string(section, "vision_free_experienced_K"));
				dangerRankCoef_.LoadCfg(pSettings->r_string(section, "vision_danger_experienced_K"));
			}
			else
			{
				freeRankCoef_.LoadCfg(pSettings->r_string(section, "vision_free_novice_K"));
				dangerRankCoef_.LoadCfg(pSettings->r_string(section, "vision_danger_novice_K"));
			}
		}
	}
}

IC	const CVisionParameters &CVisualMemoryManager::current_state() const
{
	if (m_stalker)
		return (m_stalker->memory().EnemyManager().selected()) ? m_danger : m_free;
	else if (m_object)
		return m_object->is_base_monster_with_enemy() ? m_danger : m_free;
	else
		return m_free;
}

IC const SVisionParametersKoef &CVisualMemoryManager::RankDependancy() const
{
	if (m_stalker)
		return (m_stalker->memory().EnemyManager().selected()) ? dangerRankCoef_ : freeRankCoef_;
	else if (m_object)
		return m_object->is_base_monster_with_enemy() ? dangerRankCoef_ : freeRankCoef_;
	else
		return freeRankCoef_;
}

u32	CVisualMemoryManager::visible_object_time_last_seen	(const CObject *object) const
{
	VISIBLES::const_iterator I = std::find_if(GetMemorizedObjects().begin(), GetMemorizedObjects().end(), CVisibleObjectPredicateEx(object));
	if (I != GetMemorizedObjects().end())
		return (I->m_level_time);
	else
		return u32(-1);
}

bool CVisualMemoryManager::visible_right_now(const CGameObject *game_object) const
{
	if (should_ignore_object(game_object))
	{
		return false;
	}

	VISIBLES::const_iterator I = std::find_if(GetMemorizedObjects().begin(), GetMemorizedObjects().end(), CVisibleObjectPredicateEx(game_object));

	if ((GetMemorizedObjects().end() == I))
		return(false);

	if (!(*I).visible(mask()))
		return(false);

	if ((*I).m_level_time < m_last_update_time) // only seen after LASTEST update
		return(false);

	return(true);
}

bool CVisualMemoryManager::visible_now(const CGameObject *game_object) const
{
	if (should_ignore_object(game_object))
	{
		return false;
	}

	VISIBLES::const_iterator I = std::find_if(GetMemorizedObjects().begin(), GetMemorizedObjects().end(), CVisibleObjectPredicateEx(game_object));

	if ((GetMemorizedObjects().end() == I))
		return(false);

	if (!(*I).visible(mask()))
		return(false);

	if ((*I).m_level_time + VISIBLE_NOW_OK_TIME < m_last_update_time) // Allow some time it is still visible
		return(false);

	return(true);
}

const CVisibleObject* CVisualMemoryManager::GetIfIsInPool(const CGameObject *game_object)
{
	R_ASSERT2(GetMemorizedObjectsP()->size() <= m_max_object_count, make_string("cur %u max %u", GetMemorizedObjectsP()->size(), m_max_object_count));

	VISIBLES::const_iterator I = std::find_if(GetMemorizedObjectsP()->begin(), GetMemorizedObjectsP()->end(), CVisibleObjectPredicateEx(game_object));

	if ((GetMemorizedObjects().end() == I))
		return nullptr;

	const CVisibleObject* object_from_pull = (&*I);

	if (object_from_pull->m_object)
		R_ASSERT(object_from_pull->m_object->SectionNameStr());

	return object_from_pull;
}

void CVisualMemoryManager::enable(const CObject *object, bool enable)
{
	VISIBLES::iterator J = std::find_if(GetMemorizedObjectsP()->begin(), GetMemorizedObjectsP()->end(), CVisibleObjectPredicateEx(object));

	if (J == GetMemorizedObjectsP()->end())
		return;

	(*J).m_enabled = enable;
}

float CVisualMemoryManager::object_visible_distance(const CGameObject *game_object, float &object_distance) const
{
	Fvector	eye_position = Fvector().set(0.f,0.f,0.f), eye_direction;
	Fmatrix	eye_matrix;
	float object_range = flt_max, object_fov = flt_max;

	if (m_object)
	{
		eye_matrix = smart_cast<IKinematics*>(m_object->Visual())->LL_GetTransform(u16(m_object->eye_bone));

		Fvector	temp;

		eye_matrix.transform_tiny(temp,eye_position);
		m_object->XFORM().transform_tiny(eye_position,temp);

		if (m_stalker)
		{
			eye_direction.setHP(-m_stalker->movement().m_head.current.yaw, -m_stalker->movement().m_head.current.pitch);
		}
		else
		{
			// if its a monster
			const MonsterSpace::SBoneRotation &head_orient = m_object->head_orientation();
			eye_direction.setHP(-head_orient.current.yaw, -head_orient.current.pitch);
		}
	} 
	else
	{
		Fvector dummy;
		float _0, _1;

		m_client->camera(eye_position, eye_direction, dummy, object_fov, _0, _1, object_range);
	}

	Fvector object_direction;
	game_object->Center(object_direction);

	object_distance = object_direction.distance_to(eye_position);

	object_direction.sub(eye_position);
	object_direction.normalize_safe();
	
	if (m_object)
		m_object->update_range_fov(object_range, object_fov, m_object->GetEyeRangeValue() * GetMaxViewDistance(), deg2rad(m_object->GetEyeFovValue()));

	float								fov = object_fov * 0.5f;
	float								cos_alpha = eye_direction.dotproduct(object_direction);

	clamp								(cos_alpha,-.99999f,.99999f);

	float								alpha = acosf(cos_alpha);

	clamp								(alpha,0.f,fov);

	float max_view_distance = object_range;
	float min_view_distance = _min(object_range, (m_object ? m_object->GetEyeRangeValue() * GetMinViewDistance() : object_range));

	float								distance = (1.f - alpha/fov)*(max_view_distance - min_view_distance) + min_view_distance;

	return								(distance);
}

float CVisualMemoryManager::object_luminocity (const CGameObject *game_object) const
{
	float luminocity = const_cast<CGameObject*>(game_object)->ROS()->get_ai_luminocity();

	float result = (luminocity > .0001f ? luminocity : .0001f) * GetLuminocityFactor();

	return result;
}

float CVisualMemoryManager::get_object_velocity(const CGameObject *game_object, const CNotYetVisibleObject &not_yet_visible_object) const
{
	if ((game_object->ps_Size() < 2) || (not_yet_visible_object.m_prev_time == game_object->ps_Element(game_object->ps_Size() - 2).dwTime))
		return(0.f);

	CObject::SavedPosition	pos0 = game_object->ps_Element	(game_object->ps_Size() - 2);
	CObject::SavedPosition	pos1 = game_object->ps_Element	(game_object->ps_Size() - 1);

	return(
		pos1.vPosition.distance_to(pos0.vPosition)/
		(
			float(pos1.dwTime)/1000.f - 
			float(pos0.dwTime)/1000.f
		)
	);
}

float CVisualMemoryManager::get_visible_value	(float distance, float object_distance, float time_delta, float object_velocity, float luminocity) const
{
	float always_visible_distance = GetAlwaysVisibleDistance();

	if (distance <= always_visible_distance + EPS_L)
		return (GetVisibilityThreshold());

	float result = time_delta / GetTimeQuant() * luminocity * (1.f + GetVelocityFactor() * object_velocity) * (distance - object_distance) / (distance - always_visible_distance);

	return result;
}

CNotYetVisibleObject *CVisualMemoryManager::not_yet_visible_object(const CGameObject *game_object)
{
	START_PROFILE("Memory Manager/visuals/not_yet_visible_object")

	xr_vector<CNotYetVisibleObject>::iterator I = std::find_if(
		m_not_yet_visible_objects.begin(),
		m_not_yet_visible_objects.end(),
		CNotYetVisibleObjectPredicate(game_object)
		);

	if (I == m_not_yet_visible_objects.end())
		return(0);

	return(&*I);

	STOP_PROFILE
}

void CVisualMemoryManager::add_not_yet_visible_object(const CNotYetVisibleObject &not_yet_visible_object)
{
	m_not_yet_visible_objects.push_back	(not_yet_visible_object);
}

u32	 CVisualMemoryManager::get_prev_time(const CGameObject *game_object) const
{
	if (!game_object->ps_Size())
		return(0);

	if (game_object->ps_Size() == 1)
		return(game_object->ps_Element(0).dwTime);

	return(game_object->ps_Element(game_object->ps_Size() - 2).dwTime);
}

bool CVisualMemoryManager::visible (const CGameObject *game_object, float time_delta)
{
	VERIFY(game_object);

	if (should_ignore_object(game_object))
	{
		return false;
	}

	if (game_object->getDestroy())
		return (false);

#ifndef USE_STALKER_VISION_FOR_MONSTERS
	if (!m_stalker && !m_client)
		return					(true);
#endif

	float object_distance, distance = object_visible_distance(game_object, object_distance);

	CNotYetVisibleObject *object = not_yet_visible_object(game_object);

	if (distance < object_distance)
	{
		if (object)
		{
			object->m_value -= GetDecreaseValue();

			if (object->m_value < 0.f)
				object->m_value = 0.f;
			else
				object->m_update_time = EngineTimeU();

			return (object->m_value >= GetVisibilityThreshold());
		}
		return (false);
	}

	float o_lum = object_luminocity(game_object);
	float o_velocity = 0.f;

	if (!object)
	{
		CNotYetVisibleObject new_object;

		new_object.m_object = game_object;
		new_object.m_prev_time = 0;

		o_velocity = get_object_velocity(game_object, new_object);

		new_object.m_value = get_visible_value(distance, object_distance, time_delta, o_velocity, o_lum);

		clamp(new_object.m_value, 0.f, GetVisibilityThreshold());

		new_object.m_update_time = EngineTimeU();
		new_object.m_prev_time = get_prev_time(game_object);

		add_not_yet_visible_object(new_object);

		return (new_object.m_value >= GetVisibilityThreshold());
	}

	o_velocity = get_object_velocity(game_object, *object);

	object->m_update_time = EngineTimeU();
	object->m_value += get_visible_value(distance, object_distance, time_delta, o_velocity, o_lum);

	clamp(object->m_value, 0.f, GetVisibilityThreshold());

	object->m_prev_time = get_prev_time(game_object);


	return (object->m_value >= GetVisibilityThreshold());
}

extern Flags32 psActorFlags;
bool CVisualMemoryManager::should_ignore_object (CObject const* object) const
{
	if (!object)
		return true;

	VERIFY(object->SectionNameStr());

	if (smart_cast<CActor const*>(object) && psActorFlags.test(AF_INVISIBLE))
		return	true;

	CBaseMonster const* monster = smart_cast<CBaseMonster const*>(object);
	
	if (monster)
	{
		if (!monster->can_be_seen())
			return true;

		CZombie const* zombie = smart_cast<CZombie const*>(object);

		if (zombie && zombie->fake_death_is_active())
			return true;
	}

	return false;
}

void CVisualMemoryManager::add_visible_object(const CObject *object, float time_delta, bool fictitious)
{
	if (!fictitious && should_ignore_object(object))
	{
		return;
	}

	xr_vector<CVisibleObject>::iterator	J;

	const CGameObject *game_object;
	const CGameObject *self;

	game_object	= smart_cast<const CGameObject*>(object);

	if (!game_object)
		return;

	if(!fictitious && !visible(game_object, time_delta))
		return;

	const CEntityAlive* visualized = smart_cast<const CEntityAlive*>(game_object);

	if (!ignoreVisMemorySharing_ && m_stalker && visualized)
	{
		if (visualized->g_Team() == m_object->g_Team() && (m_object->ID() != visualized->ID())) // if NPC sees ally which is having a selected enemy - add his visualized enemy to NPC mem if he is not in danger
		{
			const CAI_Stalker* ally = smart_cast<const CAI_Stalker*>(visualized);

			if (ally && ally->memory().EnemyManager().selected())
			{
				const CEntityAlive* selected_entity_alive = smart_cast<const CEntityAlive*>(ally->memory().EnemyManager().selected());
				const CAI_Stalker* selected_stalker = smart_cast<const CAI_Stalker*>(ally->memory().EnemyManager().selected());

				if ((!selected_stalker && selected_entity_alive) || (selected_stalker && !selected_stalker->wounded()))
				{
					const CVisibleObject* obj = ally->memory().VisualMManager().GetIfIsInPool(ally->memory().EnemyManager().selected());

					if (obj)
					{
						VERIFY(obj->m_object);
						VERIFY(obj->m_object->SectionNameStr());

						m_object->memory().VisualMManager().add_visible_object(*obj, true);

						//Msg("Adding visible from Seeing ally being indangered %s", ally->memory().EnemyManager().selected()->ObjectName().c_str());
					}
				}
			}
		}
	}

	self						= m_object;
	J = std::find_if(GetMemorizedObjectsP()->begin(), GetMemorizedObjectsP()->end(), CVisibleObjectPredicateEx(game_object));

	if (GetMemorizedObjectsP()->end() == J)
	{
		CVisibleObject			visible_object;

		visible_object.fill		(game_object,self,mask(),mask());

#ifdef USE_FIRST_GAME_TIME
		visible_object.m_first_game_time	= GetGameTime();
#endif

#ifdef USE_FIRST_LEVEL_TIME
		visible_object.m_first_level_time	= EngineTimeU();
#endif

		if (m_max_object_count <= GetMemorizedObjectsP()->size()) // overwrite the most inimportant one
		{
			xr_vector<CVisibleObject>::iterator	I = std::min_element(GetMemorizedObjectsP()->begin(), GetMemorizedObjectsP()->end(), SLevelTimePredicate<CGameObject>());

			VERIFY(GetMemorizedObjectsP()->end() != I);

			*I = visible_object;
		}
		else
			GetMemorizedObjectsP()->push_back(visible_object);

	}
	else
	{
		if (!fictitious)
			(*J).fill			(game_object,self,(*J).m_squad_mask.get() | mask(),(*J).m_visible.get() | mask());
		else
		{
			(*J).m_visible.assign	((*J).m_visible.get() | mask());
			(*J).m_squad_mask.assign((*J).m_squad_mask.get() | mask());
			(*J).m_enabled			= true;
		}
	}

//	STOP_PROFILE
}

void CVisualMemoryManager::add_visible_object(CVisibleObject visible_object, bool from_somebody_else)
{
	if ( should_ignore_object(visible_object.m_object) )
	{
		return;
	}

	VERIFY(GetMemorizedObjectsP());

	if (from_somebody_else && visible_object.m_level_time + VISIBLE_NOW_OK_TIME < EngineTime()) // Avoid endless loop of adding enemy from allies
		return;

	if (from_somebody_else)
	{
		if (visible_object.m_level_time > VISIBLE_NOW_OK_TIME * 2) // avoid u32 problem
			visible_object.m_level_time -= VISIBLE_NOW_OK_TIME; // Make it was visible sometime ago, to avoid npc starting fire through wall
	}

	xr_vector<CVisibleObject>::iterator J = std::find_if(GetMemorizedObjectsP()->begin(), GetMemorizedObjectsP()->end(), CVisibleObjectPredicateEx(visible_object.m_object));

	if (GetMemorizedObjectsP()->end() != J)
	{
		if (from_somebody_else)
		{
			if ((*J).m_level_time > visible_object.m_level_time) // Keep our object, if data is older
				return;
		}

		*J = visible_object;
	}
	else
		if (m_max_object_count <= GetMemorizedObjectsP()->size())
		{
			xr_vector<CVisibleObject>::iterator	I = std::min_element(GetMemorizedObjectsP()->begin(), GetMemorizedObjectsP()->end(), SLevelTimePredicate<CGameObject>());

			VERIFY(GetMemorizedObjectsP()->end() != I);

			*I = visible_object;
		}
		else
			GetMemorizedObjectsP()->push_back(visible_object);
}

#ifdef DEBUG
void CVisualMemoryManager::check_visibles	() const
{
	squad_mask_type						mask = this->mask();

	xr_vector<CVisibleObject>::const_iterator I = GetMemorizedObjects().begin();
	xr_vector<CVisibleObject>::const_iterator E = GetMemorizedObjects().end();
	for ( ; I != E; ++I) {
		if (!(*I).visible(mask))
			continue;
		
		xr_vector<Feel::Vision::feel_visible_Item>::iterator	i = m_object->feel_visible.begin();
		xr_vector<Feel::Vision::feel_visible_Item>::iterator	e = m_object->feel_visible.end();
		for (; i!=e; ++i)
			if (i->O->ID() == (*I).m_object->ID()) {
				VERIFY						(i->fuzzy > 0.f);
				break;
			}
	}
}
#endif

bool CVisualMemoryManager::visible(u32 _level_vertex_id, float yaw, float eye_fov) const
{
	Fvector direction;
	direction.sub			(ai().level_graph().vertex_position(_level_vertex_id),m_object->Position());
	direction.normalize_safe();

	float y, p;
	direction.getHP(y,p);

	if (angle_difference(yaw, y) <= eye_fov * PI / 180.f / 2.f)
		return(ai().level_graph().check_vertex_in_direction(m_object->ai_location().level_vertex_id(), m_object->Position(), _level_vertex_id));
	else
		return(false);
}

float CVisualMemoryManager::feel_vision_mtl_transp(CObject* O, u32 element)
{
	float vis = 1.f;

	if (O)
	{
		IKinematics* V = smart_cast<IKinematics*>(O->Visual());
		if (0!=V)
		{
			CBoneData& B = V->LL_GetData((u16)element);
			vis = GMLib.GetMaterialByIdx(B.game_mtl_idx)->fVisTransparencyFactor;
		}
	}
	else
	{
		CDB::TRI* T = Level().ObjectSpace.GetStaticTris()+element;
		vis = GMLib.GetMaterialByIdx(T->material)->fVisTransparencyFactor;
	}

	return vis;
}

void CVisualMemoryManager::remove_links	(CObject *object)
{
	{
		VERIFY(GetMemorizedObjectsP());

		VISIBLES::iterator I = std::find_if(GetMemorizedObjectsP()->begin(), GetMemorizedObjectsP()->end(), CVisibleObjectPredicateEx(object));

		if (I != GetMemorizedObjectsP()->end())
			GetMemorizedObjectsP()->erase(I);
	}
	{
		NOT_YET_VISIBLES::iterator I = std::find_if(m_not_yet_visible_objects.begin(), m_not_yet_visible_objects.end(), CVisibleObjectPredicateEx(object));
		if (I != m_not_yet_visible_objects.end())
			m_not_yet_visible_objects.erase	(I);
	}
}

CVisibleObject *CVisualMemoryManager::visible_object(const CGameObject *game_object)
{
	VISIBLES::iterator I = std::find_if(GetMemorizedObjectsP()->begin(), GetMemorizedObjectsP()->end(), CVisibleObjectPredicateEx(game_object));

	if (I == GetMemorizedObjectsP()->end())
		return(0);

	return(&*I);
}

IC	squad_mask_type CVisualMemoryManager::mask() const
{
	if (!m_stalker || !&m_stalker->agent_manager() || !&m_stalker->agent_manager().member())
		return (squad_mask_type(-1));

	return(m_stalker->agent_manager().member().mask(m_stalker));
}

void CVisualMemoryManager::UpdateStoredVisibles()
{
	storedVisibleNowAllies_.clear();
	storedVisibleNowEnemies_.clear();

	xr_vector<CVisibleObject>::const_iterator I = GetMemorizedObjectsP()->begin();
	xr_vector<CVisibleObject>::const_iterator E = GetMemorizedObjectsP()->end();

	for (; I != E; ++I)
	{
		auto mem = (*I);
		const CEntityAlive* ea = smart_cast<const CEntityAlive*>(mem.m_object);

		if (ea && visible_now(ea))
		{
			if (m_object->is_relation_enemy(ea))
				storedVisibleNowEnemies_.push_back(mem);
			else
				storedVisibleNowAllies_.push_back(mem);
		}
	}
}

void CVisualMemoryManager::update(float time_delta)
{
	START_PROFILE("Memory Manager/visuals/update")

	clear_delayed_objects();

	if (!enabled())
		return;

	m_last_update_time = EngineTimeU();

	squad_mask_type mask = this->mask();

	VERIFY(GetMemorizedObjectsP());

	m_visible_objects.clear();

	START_PROFILE("Memory Manager/visuals/update/feel_vision_get")

	if (m_object)
		m_object->feel_vision_get(m_visible_objects);
	else
	{
		VERIFY(m_client);
		m_client->feel_vision_get(m_visible_objects);
	}

	STOP_PROFILE

	START_PROFILE("Memory Manager/visuals/update/make_invisible")
	{
		xr_vector<CVisibleObject>::iterator	I = GetMemorizedObjectsP()->begin();
		xr_vector<CVisibleObject>::iterator	E = GetMemorizedObjectsP()->end();

		for ( ; I != E; ++I)
			if ((*I).m_level_time + GetStillVisibleTime() < EngineTimeU())
				(*I).visible(mask,false);
	}
	STOP_PROFILE

	START_PROFILE("Memory Manager/visuals/update/add_visibles")
	{
		xr_vector<CObject*>::const_iterator	I = m_visible_objects.begin();
		xr_vector<CObject*>::const_iterator	E = m_visible_objects.end();
		for ( ; I != E; ++I)
			add_visible_object(*I,time_delta);
	}
	STOP_PROFILE

	START_PROFILE("Memory Manager/visuals/update/make_not_yet_visible")
	{
		xr_vector<CNotYetVisibleObject>::iterator I = m_not_yet_visible_objects.begin();
		xr_vector<CNotYetVisibleObject>::iterator E = m_not_yet_visible_objects.end();
		for ( ; I != E; ++I)
			if ((*I).m_update_time < EngineTimeU())
				(*I).m_value = 0.f;
	}
	STOP_PROFILE

	START_PROFILE("Memory Manager/visuals/update/removing_offline")
	// verifying if object is online
	{
		GetMemorizedObjectsP()->erase(std::remove_if(GetMemorizedObjectsP()->begin(), GetMemorizedObjectsP()->end(), SRemoveOfflinePredicate()), GetMemorizedObjectsP()->end());
	}

	// verifying if object is online
	{
		m_not_yet_visible_objects.erase	(
			std::remove_if(
				m_not_yet_visible_objects.begin(),
				m_not_yet_visible_objects.end(),
				SRemoveOfflinePredicate()
			),
			m_not_yet_visible_objects.end()
		);
	}
	STOP_PROFILE

	if (m_object && g_actor && m_object->is_relation_enemy(Actor()))
	{
		xr_vector<CNotYetVisibleObject>::iterator	I = std::find_if(
			m_not_yet_visible_objects.begin(),
			m_not_yet_visible_objects.end(),
			CNotYetVisibleObjectPredicate(Actor())
		);

		if (I != m_not_yet_visible_objects.end())
		{
			Actor()->SetActorVisibility				(
				m_object->ID(),
				clampr(
				(*I).m_value / GetVisibilityThreshold(),
					0.f,
					1.f
				)
			);
		}
		else
			Actor()->SetActorVisibility(m_object->ID(),0.f);
	}

	R_ASSERT2(GetMemorizedObjectsP()->size() <= m_max_object_count, make_string("cur %u max %u", GetMemorizedObjectsP()->size(), m_max_object_count));

	if (GetMemorizedObjectsP()->size() > m_max_object_count)
		GetMemorizedObjectsP()->clear();

	STOP_PROFILE
}

#define SAVE_DISTANCE_IF_NOT_VISIBLE 12.f
#define SAVE_DISTANCE_IF_NOT_SELF_SEEN 25.f
#define SAVE_VIS_ALIES_COUNT 3

// A function for sorting valuable objects in memory for saving. Beware, dont save a lot objects, since NetPacket is not a stretching rubber =)
static inline bool is_object_valuable_to_save(CCustomMonster const* const self, MemorySpace::CVisibleObject const& object)
{
	CEntityAlive const* const entity_alive = smart_cast<CEntityAlive const*>(object.m_object);

	if (!entity_alive)
		return false;

	if (!entity_alive->g_Alive())
		return false;

	if (entity_alive == self->memory().EnemyManager().selected())
		return true;

	if (!self->is_relation_enemy(entity_alive) && self->memory().VisualMManager().countOfSavedVisibleAllies_ < SAVE_VIS_ALIES_COUNT) // Save some allies, to avoid entering panic after save\load
	{
		self->memory().VisualMManager().countOfSavedVisibleAllies_++;

		return true;
	}

	float distance_from_me = self->Position().distance_to(object.m_object_params.GetMemorizedPos());

	if (self != object.m_deriving_vmemory_owner && distance_from_me > SAVE_DISTANCE_IF_NOT_SELF_SEEN)
		return false;

	if (!object.visible(self->memory().VisualMManager().mask()) && distance_from_me > SAVE_DISTANCE_IF_NOT_VISIBLE)
		return false;

	return self->is_relation_enemy(entity_alive);
}

void CVisualMemoryManager::save	(NET_Packet &packet)
{
	if (m_client)
		return;

	if (!m_object->g_Alive())
		return;

	VISIBLES::const_iterator II = GetMemorizedObjects().begin();
	VISIBLES::const_iterator const EE = GetMemorizedObjects().end();

	countOfSavedVisibleAllies_ = 0;
	vObjectsForSaving_.clear();

	for (; II != EE; ++II)
	{
		if (is_object_valuable_to_save(m_object, *II))
		{
			vObjectsForSaving_.push_back(*II);
		}
	}

	packet.w_u8((u8)vObjectsForSaving_.size());

	if (!vObjectsForSaving_.size())
		return;

	for (u32 i = 0; i < vObjectsForSaving_.size(); i++)
	{
		CVisibleObject* mem_obj = &vObjectsForSaving_[i];

		R_ASSERT(mem_obj);

		//Msg("^ Saving %u for %s: name %s, ID %u, lvid %u, %3.2f %3.2f %3.2f || Self lvid %u, %3.2f %3.2f %3.2f level time %f last l time %f flags %u deriving ovner id %u",
		//	i,
		//	m_object->ObjectName().c_str(), mem_obj->m_object->ObjectName().c_str(), mem_obj->m_object->ID(), mem_obj->m_object_params.m_level_vertex_id, VPUSH(mem_obj->m_object_params.m_position), mem_obj->m_self_params.m_level_vertex_id, VPUSH(mem_obj->m_self_params.m_position),
		//	mem_obj->m_level_time, mem_obj->m_last_level_time, mem_obj->m_visible.flags, mem_obj->m_deriving_vmemory_owner ? mem_obj->m_deriving_vmemory_owner->ID() : u32(-1));

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

		packet.w_u32			(mem_obj->m_visible.flags);

		packet.w_u32			(mem_obj->m_deriving_vmemory_owner ? mem_obj->m_deriving_vmemory_owner->ID(): u32(-1));
	}

	vObjectsForSaving_.clear();
}

void CVisualMemoryManager::load	(IReader &packet)
{
	if (m_client)
		return;

	if (!m_object->g_Alive())
		return;

	typedef CClientSpawnManager::CALLBACK_TYPE	CALLBACK_TYPE;
	CALLBACK_TYPE					callback;
	callback.bind					(&m_object->memory(),&CMemoryManager::on_requested_spawn);

	int								count = packet.r_u8();
	for (int i=0; i<count; ++i) {
		CDelayedVisibleObject		delayed_object;
		delayed_object.m_object_id	= packet.r_u16();

		CVisibleObject				&object = delayed_object.m_visible_object;
		object.m_object				= smart_cast<CGameObject*>(Level().Objects.net_Find(delayed_object.m_object_id));
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
		R_ASSERT(EngineTimeU() >= object.m_level_time);
		object.m_level_time			= packet.r_u32();
		object.m_level_time			= EngineTimeU() - object.m_level_time;
#endif

#ifdef USE_LAST_LEVEL_TIME
		R_ASSERT(EngineTimeU() >= object.m_last_level_time);
		object.m_last_level_time	= packet.r_u32();
		object.m_last_level_time	= EngineTimeU() - object.m_last_level_time;
#endif

#ifdef USE_FIRST_LEVEL_TIME
		VERIFY						(EngineTimeU() >= (*I).m_first_level_time);
		object.m_first_level_time	= packet.r_u32();
		object.m_first_level_time	+= EngineTimeU();
#endif

		object.m_visible.assign		(packet.r_u32());

		u32 group_owner_of_this_memory_obj = packet.r_u32();
		if (group_owner_of_this_memory_obj != u32(-1))
			object.m_deriving_vmemory_owner = smart_cast<CEntityAlive*>(Level().Objects.net_Find(group_owner_of_this_memory_obj));

		if (object.m_object)
		{
			add_visible_object(object, false);
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

void CVisualMemoryManager::clear_delayed_objects()
{
	if (m_client)
		return;

	if (m_delayed_objects.empty())
		return;

	CClientSpawnManager& manager = Level().client_spawn_manager();
	DELAYED_VISIBLE_OBJECTS::const_iterator	I = m_delayed_objects.begin();
	DELAYED_VISIBLE_OBJECTS::const_iterator	E = m_delayed_objects.end();

	for ( ; I != E; ++I)
		manager.remove_spawn_callback((*I).m_object_id, m_object->ID());

	m_delayed_objects.clear();
}

void CVisualMemoryManager::on_requested_spawn	(CObject *object)
{
	DELAYED_VISIBLE_OBJECTS::iterator I = m_delayed_objects.begin();
	DELAYED_VISIBLE_OBJECTS::iterator E = m_delayed_objects.end();

	for ( ; I != E; ++I)
	{
		if ((*I).m_object_id != object->ID())
			continue;
		
		if (m_object->g_Alive())
		{
			(*I).m_visible_object.m_object	= smart_cast<CGameObject*>(object);

			VERIFY((*I).m_visible_object.m_object);

			add_visible_object((*I).m_visible_object, false);
		}

		m_delayed_objects.erase(I);

		return;
	}
}