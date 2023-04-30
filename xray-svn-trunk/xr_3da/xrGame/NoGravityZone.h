#pragma once
#include "CustomZone.h"

class CNoGravityZone :
	public CCustomZone
{
typedef CCustomZone inherited;
public:
	CNoGravityZone();

	float			p_inside_gravity;
	float			p_impulse_factor;
protected:
	virtual	void	enter_Zone						(SZoneObjectInfo& io);
	virtual	void	exit_Zone						(SZoneObjectInfo& io);
private:
	virtual	void	LoadCfg							(LPCSTR section);
			void	switchGravity					(SZoneObjectInfo& io,bool val);
	virtual	void	UpdateWorkload					(u32 dt);
};