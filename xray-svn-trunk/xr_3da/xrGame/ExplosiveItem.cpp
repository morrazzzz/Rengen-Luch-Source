//////////////////////////////////////////////////////////////////////
// ExplosiveItem.cpp:	класс для вещи которая взрывается под 
//						действием различных хитов (канистры,
//						балоны с газом и т.д.)
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "ExplosiveItem.h"


CExplosiveItem::CExplosiveItem(void)
{
}
CExplosiveItem::~CExplosiveItem(void)
{
}

void CExplosiveItem::LoadCfg(LPCSTR section)
{
	inherited::LoadCfg						(section);
	CExplosive::LoadCfg						(section);
	m_flags.set								(FUsingCondition, TRUE);
	CDelayedActionFuse::Initialize			(pSettings->r_float(section,"time_to_explode"),pSettings->r_float(section,"condition_to_explode"));
	VERIFY(pSettings->line_exist			(section,"set_timer_particles"));
}

void CExplosiveItem::DestroyClientObj()
{
	inherited::DestroyClientObj();
	CExplosive::DestroyClientObj();
}

//void CExplosiveItem::Hit(float P, Fvector &dir,	CObject* who, s16 element,
//						Fvector position_in_object_space, float impulse, 
//						ALife::EHitType hit_type)
void	CExplosiveItem::Hit					(SHit* pHDS)
{
//	inherited::Hit(P,dir,who,element,position_in_object_space,impulse,hit_type);
	if(CDelayedActionFuse::isActive())pHDS->power=0.f;
	inherited::Hit(pHDS);
	VERIFY(pHDS->who);
	if(!CDelayedActionFuse::isActive() &&
		CDelayedActionFuse::CheckCondition(GetCondition() &&
		pHDS->who)/*&&CExplosive::Initiator()==u16(-1)*/)
	{
		//запомнить того, кто взорвал вещь
		SetInitiator( pHDS->who->ID());

	}
}
void	CExplosiveItem::StartTimerEffects	()
{
	CParticlesPlayer::StartParticles(pSettings->r_string(*SectionName(),"set_timer_particles"),Fvector().set(0,1,0),ID());

}
void  CExplosiveItem::OnEvent (NET_Packet& P, u16 type)
{
	CExplosive::OnEvent (P, type);
	inherited::OnEvent (P, type);

}

void CExplosiveItem::UpdateCL()
{
	CExplosive::UpdateCL();
	inherited::UpdateCL();
}

void CExplosiveItem::ScheduledUpdate(u32 dt)
{
#ifdef MEASURE_UPDATES
	CTimer measure_sc_update; measure_sc_update.Start();
#endif


	inherited::ScheduledUpdate(dt);
	if(CDelayedActionFuse::isActive()&&CDelayedActionFuse::Update(GetCondition()))
	{
		Fvector normal;
		FindNormal(normal);

		CExplosive::GenExplodeEvent(Position(), normal);
		CParticlesPlayer::StopParticles(ID(), BI_NONE, true);
	}
	
	
#ifdef MEASURE_UPDATES
	Device.Statistic->scheduler_VariousItems_ += measure_sc_update.GetElapsed_sec();
#endif
}

bool CExplosiveItem::shedule_Needed()
{
//.	return true;

	return ( inherited::shedule_Needed() || CDelayedActionFuse::isActive() );
}

void CExplosiveItem::renderable_Render(IRenderBuffer& render_buffer)
{
	inherited::renderable_Render(render_buffer);
}
void CExplosiveItem::RemoveLinksToCLObj(CObject* O )
{
	CExplosive::RemoveLinksToCLObj(O);
	inherited::RemoveLinksToCLObj(O);
}

void	CExplosiveItem::ActivateExplosionBox	(const Fvector &size,Fvector &in_out_pos)
{
	//PKinematics(Visual())->CalculateBones();
}
void CExplosiveItem::GetRayExplosionSourcePos(Fvector &pos)
{
	random_point_in_object_box(pos,this);
}