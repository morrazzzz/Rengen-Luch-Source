#ifndef PHYSICS_SKELETON_OBJECT_H
#define PHYSICS_SKELETON_OBJECT_H
#include "physicsshellholder.h"
#include "PHSkeleton.h"


class CSE_ALifePHSkeletonObject;
class CPhysicsSkeletonObject : 
	public CPhysicsShellHolder,
	public CPHSkeleton

{
typedef CPhysicsShellHolder inherited;

public:
	CPhysicsSkeletonObject(void);
	virtual ~CPhysicsSkeletonObject(void);

	virtual void					LoadCfg				(LPCSTR section);

	virtual BOOL					SpawnAndImportSOData( CSE_Abstract* data_containing_so);
	virtual void					DestroyClientObj	();

	virtual void					net_Save			(NET_Packet& P);
	virtual	BOOL					net_SaveRelevant	();	
	
	virtual void					UpdateCL			( );// Called each frame, so no need for dt
	virtual void					ScheduledUpdate		(u32 dt);	//
	
	virtual BOOL					UsedAI_Locations	();
protected:
	virtual CPhysicsShellHolder		*PPhysicsShellHolder()													{return PhysicsShellHolder();}
	virtual CPHSkeleton				*PHSkeleton			()																	{return this;}
	virtual void					SpawnInitPhysics	(CSE_Abstract	*D)																;
	virtual void					PHObjectPositionUpdate()																			;
	virtual	void					CreatePhysicsShell	(CSE_Abstract	*e)																;
};



#endif