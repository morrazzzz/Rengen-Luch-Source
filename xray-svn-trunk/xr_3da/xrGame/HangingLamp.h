// DummyObject.h: interface for the CHangingLamp class.
//
//////////////////////////////////////////////////////////////////////

#ifndef HangingLampH
#define HangingLampH
#pragma once

#include "gameobject.h"
#include "physicsshellholder.h"
#include "PHSkeleton.h"
#include "script_export_space.h"
// refs
class CLAItem;
class CPhysicsElement;
class CSE_ALifeObjectHangingLamp;
class CPHElement;
class CHangingLamp: 
public CPhysicsShellHolder,
public CPHSkeleton
{//need m_pPhysicShell
	typedef	CPhysicsShellHolder		inherited;
private:
	u16				light_bone;
	u16				ambient_bone;

	ref_light		light_render;
	ref_light		light_ambient;
	CLAItem*		lanim;
	float			ambient_power;
	BOOL			m_bState;

	u32				restoreLampTime_;
	
	ref_glow		glow_render;
	
	float			fHealth;
	float			fBrightness;
	void			CreateBody		(CSE_ALifeObjectHangingLamp	*lamp);
	void			Init();
	void			RespawnInit		();
	bool			Alive			(){return fHealth>0.f;}


public:
					CHangingLamp	();
	virtual			~CHangingLamp	();
	
	void			TurnOn			();
	void			TurnOff			();
	
	virtual void	LoadCfg			( LPCSTR section);
	
	virtual BOOL	SpawnAndImportSOData	( CSE_Abstract* data_containing_so);
	virtual void	DestroyClientObj		();
	virtual void	ExportDataToServer(NET_Packet& P);
	virtual void	SpawnInitPhysics	(CSE_Abstract	*D)																;

	virtual void	net_Save			(NET_Packet& P)																	;
	virtual	BOOL	net_SaveRelevant	();
	
	virtual void	save				(NET_Packet &output_packet);
	virtual void	load				(IReader &input_packet);

	virtual void	ScheduledUpdate	( u32 dt);							// Called by sheduler
	virtual void	UpdateCL		( );								// Called each frame, so no need for dt
	
	virtual BOOL	renderable_ShadowGenerate	( ) { return TRUE;	}
	virtual BOOL	renderable_ShadowReceive	( ) { return TRUE;	}
	
	virtual CPhysicsShellHolder*	PPhysicsShellHolder	()	{return PhysicsShellHolder();}								;
	virtual	void	CopySpawnInit		()																				;
	
	virtual	void	Hit				(SHit* pHDS);

	virtual BOOL	UsedAI_Locations();

	virtual void	Center			(Fvector& C)	const;
	virtual float	Radius			()				const;
	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CHangingLamp)
#undef script_type_list
#define script_type_list save_type_list(CHangingLamp)

#endif //HangingLampH
