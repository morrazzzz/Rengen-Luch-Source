#pragma once

class ENGINE_API ISheduled
{
public:
	struct {
		u32		t_min		:	14;		// minimal bound of update time (sample: 20ms)
		u32		t_max		:	14;		// maximal bound of update time (sample: 200ms)
		u32		b_RT		:	1;
		u32		b_locked	:	1;
	}	shedule;

	u32			deltaTime;
#ifdef DEBUG
	u32									dbg_startframe;
	u32									dbg_update_shedule;
#endif

				ISheduled				();
	virtual ~	ISheduled				();

	void								shedule_register	();
	void								shedule_unregister	();

	virtual float						shedule_Scale		()			= 0;
	virtual void						ScheduledUpdate		(u32 dt);
	virtual void _stdcall				ScheduledFrameEnd	();
	virtual	shared_str					SchedulerName		() const	{ return shared_str("unknown"); };
	virtual bool						shedule_Needed		()			= 0;

};