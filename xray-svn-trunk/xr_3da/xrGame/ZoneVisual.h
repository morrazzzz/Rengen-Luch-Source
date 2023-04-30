#pragma once

#include "../../Include/xrRender/animation_motion.h "

class CVisualZone :
	public CCustomZone
{
	typedef				CCustomZone		inherited	;
	MotionID			m_idle_animation			;
	MotionID			m_attack_animation			;
	u32					m_dwAttackAnimaionStart		;
	u32					m_dwAttackAnimaionEnd		;
public:
	CVisualZone				()						;
	virtual			~CVisualZone					()						;
	
	virtual void	LoadCfg							(LPCSTR section)		;
	
	virtual BOOL	SpawnAndImportSOData			(CSE_Abstract* data_containing_so);
	virtual void	DestroyClientObj				();
	
	virtual void    AffectObjects					()						;
	virtual void	SwitchZoneState					(EZoneState new_state)	;
	virtual void	UpdateBlowout					()						;
protected:
private:
};
