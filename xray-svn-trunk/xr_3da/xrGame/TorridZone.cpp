#include "stdafx.h"
#include "torridZone.h"
#include "../objectanimator.h"
#include "xrServer_Objects_ALife_Monsters.h"

CTorridZone::CTorridZone()
{
	m_animator			= xr_new <CObjectAnimator>();
}

CTorridZone::~CTorridZone()
{
	xr_delete			(m_animator);
}

BOOL CTorridZone::SpawnAndImportSOData(CSE_Abstract* data_containing_so)
{
	if (!inherited::SpawnAndImportSOData(data_containing_so))
		return			(FALSE);

	CSE_Abstract		*abstract=(CSE_Abstract*)(data_containing_so);
	CSE_ALifeTorridZone	*zone	= smart_cast<CSE_ALifeTorridZone*>(abstract);
	VERIFY				(zone);

	m_animator->Load	(zone->get_motion());
	m_animator->Play	(true);

	return				(TRUE);
}

void CTorridZone::UpdateWorkload(u32 dt)
{
	inherited::UpdateWorkload	(dt);
	m_animator->Update			(float(dt)/1000.f);
	XFORM().set					(m_animator->XFORM());
	OnMove						();
}

void CTorridZone::ScheduledUpdate(u32 dt)
{
#ifdef MEASURE_UPDATES
	CTimer measure_sc_update; measure_sc_update.Start();
#endif


	inherited::ScheduledUpdate(dt);

	if(m_idle_sound._feedback())		m_idle_sound.set_position		(XFORM().c);
	if(m_blowout_sound._feedback())		m_blowout_sound.set_position	(XFORM().c);
	if(m_hit_sound._feedback())			m_hit_sound.set_position		(XFORM().c);
	if(m_entrance_sound._feedback())	m_entrance_sound.set_position	(XFORM().c);


#ifdef MEASURE_UPDATES
	Device.Statistic->scheduler_VariousAnomalies_ += measure_sc_update.GetElapsed_sec();
#endif
}

bool CTorridZone::Enable()
{
	bool res = inherited::Enable();
	if (res)
	{
		m_animator->Stop();
		m_animator->Play(true);
	}
	return res;
}

bool CTorridZone::Disable()
{
	bool res = inherited::Disable();
	if (res)
		m_animator->Stop();

	return res;
}

// Lain: added
bool   CTorridZone::light_in_slow_mode()
{
	return false;
}

BOOL   CTorridZone::AlwaysInUpdateList()
{
	return true;
}

