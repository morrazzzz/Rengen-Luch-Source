#pragma once
#include "mosquitobald.h"

class CObjectAnimator;

class CTorridZone :public CMosquitoBald
{
private:
	typedef	CCustomZone	inherited;
	CObjectAnimator		*m_animator;
public:
						CTorridZone			();
	virtual				~CTorridZone		();
	virtual void		UpdateWorkload		(u32 dt);
	virtual void		ScheduledUpdate		(u32 dt);
	BOOL				SpawnAndImportSOData(CSE_Abstract* data_containing_so);

	virtual bool		IsVisibleForZones	() { return true;		}
	virtual	bool		Enable				();
	virtual	bool		Disable				();

	// Lain: added
	virtual bool        light_in_slow_mode  ();
	virtual BOOL        AlwaysInUpdateList();
};