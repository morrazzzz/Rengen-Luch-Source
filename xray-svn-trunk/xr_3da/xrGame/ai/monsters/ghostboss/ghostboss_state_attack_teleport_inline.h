#pragma once

#include "../../../level.h"

#define TEMPLATE_SPECIALIZATION template <\
	typename _Object\
>

#define CStateGhostBossAttackTeleportAbstract CStateGhostBossTeleport<_Object>

TEMPLATE_SPECIALIZATION

CStateGhostBossAttackTeleportAbstract::CStateGhostBossTeleport(_Object *obj) : inherited(obj)
{
	m_last_teleport_started = 0;
	m_action = ACTION_IDLE;
}

TEMPLATE_SPECIALIZATION
void CStateGhostBossAttackTeleportAbstract::initialize()
{
	inherited::initialize		();

	m_last_teleport_started =  EngineTimeU();
	m_action = ACTION_IDLE;
//	m_shield_start_anim_length_sec = object->anim().motion_time(eAnimShieldStart, 0, object->Visual());
}


TEMPLATE_SPECIALIZATION
void CStateGhostBossAttackTeleportAbstract::finalize()
{
	inherited::finalize();

	object->DeactiveTeleport();
	object->set_script_capture(true);
}

TEMPLATE_SPECIALIZATION
void CStateGhostBossAttackTeleportAbstract::critical_finalize()
{
	inherited::critical_finalize();

	object->DeactiveTeleport();
	object->set_script_capture(false);
	
}

TEMPLATE_SPECIALIZATION
void CStateGhostBossAttackTeleportAbstract::execute()
{
	switch (m_action)
	{
	case ACTION_IDLE:
		object->ActivateTeleport();
		//object->com_man().ta_activate(object->anim_triple_shield);
		m_action = ACTION_COMPLETED;
		break;
	case ACTION_COMPLETED:
		break;
	}
	object->set_action(ACT_STAND_IDLE);
/*
	if (object->EnemyMan.get_enemy())
		object->dir().face_target(object->EnemyMan.get_enemy(), 500);
	else {
		Fvector pos;
		pos.mad(object->Position(), Fvector().set(0.1f,0.0f,0.0f));
		object->dir().face_target(pos, 500);
	}
	object->set_action(ACT_STAND_IDLE);*/
}


TEMPLATE_SPECIALIZATION
bool CStateGhostBossAttackTeleportAbstract::check_start_conditions()
{
	if (EngineTimeU() > m_last_teleport_started + object->m_teleport_length) {
		if (object->EnemyMan.get_enemy() && !object->EnemyMan.enemy_see_me_now()) return false; 
	} else {
		return false;
	}

	if (!object->CanTeleport()) return false;

	return true;
}

TEMPLATE_SPECIALIZATION
bool CStateGhostBossAttackTeleportAbstract::check_completion()
{
	if (EngineTimeU() <= m_last_teleport_started + object->m_teleport_length)
	{
		const CEntityAlive* enemy = object->EnemyMan.get_enemy();
		if ((enemy && enemy != Actor()) || !Actor()->IsReloadingWeapon())
			return enemy == 0;
	}
	return true;
}

#undef TEMPLATE_SPECIALIZATION
#undef CStateGhostBossShieldAbstract
