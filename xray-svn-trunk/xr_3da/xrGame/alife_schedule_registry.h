////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_schedule_registry.h
//	Created 	: 15.01.2003
//  Modified 	: 12.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife schedule registry
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "safe_map_iterator.h"
#include "xrServer_Objects_ALife.h"
#include "ai_debug.h"
#include "profiler.h"

extern ENGINE_API BOOL logPerfomanceProblems_;

class CALifeScheduleRegistry : public CSafeMapIterator<ALife::_OBJECT_ID,CSE_ALifeSchedulable,std::less<ALife::_OBJECT_ID>, true> {
private:
	struct CUpdatePredicate
	{
		u32								m_count;
		mutable u32						m_current;

		IC CUpdatePredicate (const u32 &count)
		{
			m_count						= count;
			m_current					= 0;
		}

		IC bool operator() (_iterator &i, u64 cycle_count, bool) const
		{
			if ((*i).second->m_schedule_counter	== cycle_count)
				return(false);

			if (m_current >= m_count)
				return(false);

			++m_current;
			(*i).second->m_schedule_counter	= cycle_count;

			return(true);
		}

		IC void operator() (_iterator &i, u64 cycle_count) const
		{
#ifdef MEASURE_UPDATES
			CTimer debug_lag; debug_lag.Start();
#endif


			START_PROFILE("ALife/scheduled/update")

			(*i).second->update();

			STOP_PROFILE


#ifdef MEASURE_UPDATES
			if (logPerfomanceProblems_ && debug_lag.GetElapsed_sec() * 1000.f > 3.0f)
			{
				CSE_Abstract* server_abstr = (*i).second->cast_abstract();
				LPCSTR debug_name = "no name?!";

				if (server_abstr)
				{
					debug_name = server_abstr->name() ? server_abstr->name() : "no name?!";
				}

				Msg(LINE_SPACER);
				Msg("!Perfomance Warning: server update of: %s took %f ms", debug_name, debug_lag.GetElapsed_sec() * 1000.f);
			}
#endif
		}
	};

protected:
	typedef CSafeMapIterator<ALife::_OBJECT_ID,CSE_ALifeSchedulable,std::less<ALife::_OBJECT_ID>, true> inherited;

protected:
	u32		scheduleRegistryObjectsPerUpdate_;
	float	scheduleRegistryProcessTimeLimit_;

public:
	IC								CALifeScheduleRegistry	();
	virtual							~CALifeScheduleRegistry	();
			void					add						(CSE_ALifeDynamicObject *object);
			void					remove					(CSE_ALifeDynamicObject *object, bool no_assert = false);
	IC		void					update					();
	IC		CSE_ALifeSchedulable	*object					(const ALife::_OBJECT_ID &id, bool no_assert = false) const;
	IC		const u32				&objects_per_update		() const;
	IC		void					SetUpUpdateLimits		(const u32 &objects_per_update, const float time_limit);
};

#include "alife_schedule_registry_inline.h"