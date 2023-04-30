#pragma once
#include "MosquitoBald.h"

class CZoneCampfire : public CMosquitoBald
{
	typedef CMosquitoBald	inherited;
protected:
	CParticlesObject*		m_pEnablingParticles;
	CParticlesObject*		m_pDisabledParticles;
	ref_sound				m_disabled_sound;
	bool					m_turned_on;
	u32						m_turn_time;

		virtual BOOL		AlwaysInUpdateList			();
		virtual	void		PlayIdleParticles			(bool bIdleLight=true);
		virtual	void		StopIdleParticles			(bool bIdleLight=true);
		virtual	void		UpdateWorkload				(u32 dt);

public:
							CZoneCampfire				();
	virtual					~CZoneCampfire				();
	
	virtual		void		LoadCfg						(LPCSTR section);
	
	virtual		void		ScheduledUpdate				(u32 dt);
	
	virtual		void		GoEnabledState				();
	virtual		void		GoDisabledState				();

				void		turn_on_script				();
				void		turn_off_script				();
				bool		is_on						();

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CZoneCampfire)
#undef script_type_list
#define script_type_list save_type_list(CZoneCampfire)