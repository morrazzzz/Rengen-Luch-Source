////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_schedule_registry_inline.h
//	Created 	: 15.01.2003
//  Modified 	: 12.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife schedule registry inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CALifeScheduleRegistry::CALifeScheduleRegistry()
{
	scheduleRegistryObjectsPerUpdate_ = 1;
	scheduleRegistryProcessTimeLimit_ = 0.02;
}

IC	const u32 &CALifeScheduleRegistry::objects_per_update() const
{
	return (scheduleRegistryObjectsPerUpdate_);
}

IC	void CALifeScheduleRegistry::SetUpUpdateLimits(const u32 &objects_per_update, const float time_limit)
{
	scheduleRegistryObjectsPerUpdate_ = objects_per_update;
	scheduleRegistryProcessTimeLimit_ = time_limit;
}

IC	void CALifeScheduleRegistry::update						()
{
	inherited::set_process_time(scheduleRegistryProcessTimeLimit_);
	inherited::set_is_time_limited(true);

	objects().empty() ? 0 : inherited::update(CUpdatePredicate(scheduleRegistryObjectsPerUpdate_), false);

#ifdef DEBUG
	if (psAI_Flags.test(aiALife)) {
//		Msg						("[LSS][SU][%d : %d]",count, objects().size());
	}
#endif
}

IC	CSE_ALifeSchedulable *CALifeScheduleRegistry::object(const ALife::_OBJECT_ID &id, bool no_assert) const
{
	_const_iterator	I = objects().find(id);

	if (I == objects().end())
	{
		THROW2 (no_assert, "The spesified object hasn't been found in the schedule registry!");

		return(0);
	}

	return((*I).second);
}
