#include "stdafx.h"
#include "physicsskeletonobject.h"
#include "../../xrphysics/PhysicsShell.h"
#include "phsynchronize.h"
#include "xrserver_objects_alife.h"
#include "../Include/xrRender/Kinematics.h"
#include "../xr_collide_form.h"

CPhysicsSkeletonObject::CPhysicsSkeletonObject()
{

}

CPhysicsSkeletonObject::~CPhysicsSkeletonObject()
{

}


BOOL CPhysicsSkeletonObject::SpawnAndImportSOData(CSE_Abstract* data_containing_so)
{
	CSE_Abstract			  *e	= (CSE_Abstract*)(data_containing_so);

	inherited::SpawnAndImportSOData(data_containing_so);
	
	xr_delete(collidable.model);
	collidable.model = xr_new <CCF_Skeleton>(this);
	CPHSkeleton::Spawn(e);
	setVisible(TRUE);
	setEnabled(TRUE);

	return TRUE;
}

void	CPhysicsSkeletonObject::SpawnInitPhysics	(CSE_Abstract	*D)
{
	CreatePhysicsShell(D);
	IKinematics* K=smart_cast<IKinematics*>	(Visual());
	if(K)	
	{	
		K->CalculateBones_Invalidate();
		K->CalculateBones(BonesCalcType::force_recalc);
	}
}

void CPhysicsSkeletonObject::DestroyClientObj()
{

	inherited::DestroyClientObj();
	CPHSkeleton::RespawnInit	();

}

void CPhysicsSkeletonObject::LoadCfg(LPCSTR section)
{
	inherited::LoadCfg(section);
	CPHSkeleton::LoadCfg(section);
}

void CPhysicsSkeletonObject::CreatePhysicsShell(CSE_Abstract* e)
{
	CSE_PHSkeleton	*po=smart_cast<CSE_PHSkeleton*>(e);
	if(m_pPhysicsShell) return;
	if (!Visual()) return;
	m_pPhysicsShell=P_build_Shell(this,!po->_flags.test(CSE_PHSkeleton::flActive));

}


void CPhysicsSkeletonObject::ScheduledUpdate(u32 dt)
{
#ifdef MEASURE_UPDATES
	CTimer measure_sc_update; measure_sc_update.Start();
#endif


	inherited::ScheduledUpdate(dt);

	CPHSkeleton::Update(dt);

	
#ifdef MEASURE_UPDATES
	Device.Statistic->scheduler_VariousPhysics_ += measure_sc_update.GetElapsed_sec();
#endif
}

void CPhysicsSkeletonObject::net_Save(NET_Packet &P)
{
	inherited::net_Save(P);
	CPHSkeleton::SaveNetState	   (P);
}



BOOL CPhysicsSkeletonObject::net_SaveRelevant()
{
	return TRUE;//!m_flags.test(CSE_ALifeObjectPhysic::flSpawnCopy);
}


BOOL CPhysicsSkeletonObject::UsedAI_Locations()
{
	return					(FALSE);
}

void CPhysicsSkeletonObject::UpdateCL()
{
#ifdef MEASURE_UPDATES
	CTimer measure_updatecl; measure_updatecl.Start();
#endif


	inherited::UpdateCL();

	PHObjectPositionUpdate();

	
#ifdef MEASURE_UPDATES
	Device.Statistic->updateCL_VariousPhysics_ += measure_updatecl.GetElapsed_sec();
#endif
}

void CPhysicsSkeletonObject::PHObjectPositionUpdate()
{
	if(m_pPhysicsShell)
	{
		m_pPhysicsShell->InterpolateGlobalTransform(&XFORM());
	}
}