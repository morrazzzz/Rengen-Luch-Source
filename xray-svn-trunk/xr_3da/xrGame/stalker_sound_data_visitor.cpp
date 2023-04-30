
//	Module 		: stalker_sound_data_visitor.cpp
//	Created 	: 02.02.2005
//	Author		: Dmitriy Iassenev
//	Description : Stalker sound data visitor

#include "pch_script.h"
#include "stalker_sound_data_visitor.h"
#include "ai/stalker/ai_stalker.h"
#include "stalker_sound_data.h"
#include "agent_manager.h"
#include "agent_member_manager.h"
#include "memory_manager.h"
#include "hit_memory_manager.h"
#include "visual_memory_manager.h"
#include "enemy_manager.h"
#include "danger_manager.h"

CStalkerSoundDataVisitor::~CStalkerSoundDataVisitor()
{
}

void CStalkerSoundDataVisitor::visit(CStalkerSoundData* data)
{
	if (object().memory().EnemyManager().selected())
		return;

	if (object().is_relation_enemy(&data->object()))
		return;

	if (!data->object().memory().EnemyManager().selected())
	{
		if (!object().memory().DangerManager().selected() && data->object().memory().DangerManager().selected())
			object().memory().DangerManager().add(*data->object().memory().DangerManager().selected());

		return;
	}

	if (data->object().memory().EnemyManager().selected()->getDestroy())
		return;

	if (!object().is_relation_enemy(data->object().memory().EnemyManager().selected()))
		return;

	if (!data->object().g_Alive())
		return;

	if (!object().g_Alive())
		return;

	Msg("%s : Adding fiction hit by sound info from stalker %s", *object().ObjectName(), *data->object().ObjectName());

	object().memory().make_object_visible_somewhen(data->object().memory().EnemyManager().selected());
}
