#include "stdafx.h"
#include "AnomDetectorBase.h"
#include "customzone.h"
#include "inventory.h"
#include "level.h"
#include "map_manager.h"
#include "ActorEffector.h"
#include "actor.h"
#include "ai_sounds.h"

ZONE_INFO::ZONE_INFO	()
{
	pParticle=NULL;
}

ZONE_INFO::~ZONE_INFO	()
{
	if(pParticle)
		DestroyParticleInstance(pParticle);
}

CAnomDetectorBase::CAnomDetectorBase(void)
{
	m_bWorking					= false;
}

CAnomDetectorBase::~CAnomDetectorBase(void)
{
	ZONE_TYPE_MAP_IT it;
	for(it = m_ZoneTypeMap.begin(); m_ZoneTypeMap.end() != it; ++it)
		HUD_SOUND_ITEM::DestroySound(it->second.detect_snds);
//		it->second.detect_snd.destroy();

	m_ZoneInfoMap.clear();
}

BOOL CAnomDetectorBase::SpawnAndImportSOData(CSE_Abstract* data_containing_so)
{
	m_pCurrentActor		 = NULL;
	m_pCurrentInvOwner	 = NULL;

	return(inherited::SpawnAndImportSOData(data_containing_so));
}

void CAnomDetectorBase::LoadCfg(LPCSTR section)
{
	inherited::LoadCfg(section);

	m_fRadius				= pSettings->r_float(section,"radius");
	
	if( pSettings->line_exist(section,"night_vision_particle") )
		m_nightvision_particle	= pSettings->r_string(section,"night_vision_particle");

	u32 i = 1;
	string256 temp;

	//загрузить звуки для обозначения различных типов зон
	do 
	{
		xr_sprintf			(temp, "zone_class_%d", i);
		if(pSettings->line_exist(section,temp))
		{
			LPCSTR z_Class			= pSettings->r_string(section,temp);
			CLASS_ID zone_cls		= TEXT2CLSID(pSettings->r_string(z_Class,"class"));

			m_ZoneTypeMap.insert	(std::make_pair(zone_cls,ZONE_TYPE()));
			ZONE_TYPE& zone_type	= m_ZoneTypeMap[zone_cls];
			xr_sprintf					(temp, "zone_min_freq_%d", i);
			zone_type.min_freq		= pSettings->r_float(section,temp);
			xr_sprintf					(temp, "zone_max_freq_%d", i);
			zone_type.max_freq		= pSettings->r_float(section,temp);
			R_ASSERT				(zone_type.min_freq<zone_type.max_freq);
			xr_sprintf					(temp, "zone_sound_%d_", i);

			HUD_SOUND_ITEM::LoadSound(section, temp, zone_type.detect_snds, 0, SOUND_TYPE_ITEM);

			xr_sprintf					(temp, "zone_map_location_%d", i);
			
			if( pSettings->line_exist(section,temp) )
				zone_type.zone_map_location = pSettings->r_string(section,temp);

			++i;
		}
		else break;
	} while(true);

	m_ef_detector_type	= pSettings->r_u32(section,"ef_detector_type");
}


void CAnomDetectorBase::ScheduledUpdate(u32 dt)
{
#ifdef MEASURE_UPDATES
	CTimer measure_sc_update; measure_sc_update.Start();
#endif
	
	
	inherited::ScheduledUpdate(dt);
	
	if(!IsWorking())
		return;
	if(!H_Parent())
		return;

	Position().set(H_Parent()->Position());

	if (H_Parent() && H_Parent() == Level().CurrentViewEntity())
	{
		Fvector P; 

		P.set(H_Parent()->Position());
		feel_touch_update(P, m_fRadius);

		UpdateNightVisionMode();
	}
	
	
#ifdef MEASURE_UPDATES
	Device.Statistic->scheduler_VariousItems_ += measure_sc_update.GetElapsed_sec();
#endif
}

void CAnomDetectorBase::StopAllSounds()
{
	ZONE_TYPE_MAP_IT it;
	for(it = m_ZoneTypeMap.begin(); m_ZoneTypeMap.end() != it; ++it) 
	{
		ZONE_TYPE& zone_type = (*it).second;
		HUD_SOUND_ITEM::StopSound(zone_type.detect_snds);
//		zone_type.detect_snd.stop();
	}
}

void CAnomDetectorBase::UpdateCL()
{
#ifdef MEASURE_UPDATES
	CTimer measure_updatecl; measure_updatecl.Start();
#endif


	inherited::UpdateCL();

	if(!IsWorking())
		return;

	if(!H_Parent())
		return;

	if(!m_pCurrentActor)
		return;

	ZONE_INFO_MAP_IT it;

	for(it = m_ZoneInfoMap.begin(); m_ZoneInfoMap.end() != it; ++it) 
	{
		CCustomZone* pZone = it->first;
		ZONE_INFO& zone_info = it->second;

		//такой тип зон не обнаруживается
		if(m_ZoneTypeMap.find(pZone->CLS_ID) == m_ZoneTypeMap.end() || !pZone->VisibleByDetector())
			continue;

		ZONE_TYPE& zone_type = m_ZoneTypeMap[pZone->CLS_ID];

		float dist_to_zone = H_Parent()->Position().distance_to(pZone->Position()) - 0.8f * pZone->Radius();

		if(dist_to_zone < 0)
			dist_to_zone = 0;
		
		float fRelPow = 1.f - dist_to_zone / m_fRadius;

		clamp(fRelPow, 0.f, 1.f);

		//определить текущую частоту срабатывания сигнала
		zone_info.cur_freq = zone_type.min_freq + 
			(zone_type.max_freq - zone_type.min_freq) * fRelPow* fRelPow* fRelPow* fRelPow;

		float current_snd_time = 1000.f * 1.f / zone_info.cur_freq;
			
		if((float)zone_info.snd_time > current_snd_time)
		{
			zone_info.snd_time = 0;

			HUD_SOUND_ITEM::PlaySound(zone_type.detect_snds, Fvector().set(0,0,0), this, true, false);

		} 
		else 
			zone_info.snd_time += TimeDeltaU();
	}
	
	
#ifdef MEASURE_UPDATES
	Device.Statistic->updateCL_VariousItems_ += measure_updatecl.GetElapsed_sec();
#endif
}

void CAnomDetectorBase::feel_touch_new(CObject* O)
{
	CCustomZone *pZone = smart_cast<CCustomZone*>(O);
	if(pZone && pZone->IsEnabled()) 
	{
		m_ZoneInfoMap[pZone].snd_time = 0;
		
		AddRemoveMapSpot(pZone,true);
	}
}

void CAnomDetectorBase::feel_touch_delete(CObject* O)
{
	CCustomZone *pZone = smart_cast<CCustomZone*>(O);
	if(pZone)
	{
		m_ZoneInfoMap.erase(pZone);
		AddRemoveMapSpot(pZone,false);
	}
}

BOOL CAnomDetectorBase::feel_touch_contact(CObject* O)
{
	return (NULL != smart_cast<CCustomZone*>(O));
}

void CAnomDetectorBase::AfterAttachToParent()
{
	m_pCurrentActor				= smart_cast<CActor*>(H_Parent());
	m_pCurrentInvOwner			= smart_cast<CInventoryOwner*>(H_Parent());
	
	inherited::AfterAttachToParent();
}

void CAnomDetectorBase::BeforeDetachFromParent(bool just_before_destroy)
{
	inherited::BeforeDetachFromParent(just_before_destroy);
	
	m_pCurrentActor				= NULL;
	m_pCurrentInvOwner			= NULL;

	StopAllSounds				();

	m_ZoneInfoMap.clear			();
	Feel::Touch::feel_touch.clear();
}


u32	CAnomDetectorBase::ef_detector_type() const
{
	return	(m_ef_detector_type);
}

void CAnomDetectorBase::OnMoveToRuck()
{
	inherited::OnMoveToRuck();
	TurnOff();
}

void CAnomDetectorBase::OnMoveToSlot()
{
	inherited::OnMoveToSlot	();
	TurnOn					();
}

void CAnomDetectorBase::OnMoveToBelt()
{
	inherited::OnMoveToBelt	();
	TurnOn					();
}

void CAnomDetectorBase::TurnOn()
{
	m_bWorking				= true;
	UpdateMapLocations		();
	UpdateNightVisionMode	();
}

void CAnomDetectorBase::TurnOff()
{
	m_bWorking				= false;
	UpdateMapLocations		();
	UpdateNightVisionMode	();
}

void CAnomDetectorBase::AddRemoveMapSpot(CCustomZone* pZone, bool bAdd)
{
	if(m_ZoneTypeMap.find(pZone->CLS_ID) == m_ZoneTypeMap.end() )return;
	
	if ( bAdd && !pZone->VisibleByDetector() ) return;
		

	ZONE_TYPE& zone_type = m_ZoneTypeMap[pZone->CLS_ID];
	if( xr_strlen(zone_type.zone_map_location) ){
		if( bAdd )
			Level().MapManager().AddMapLocation(*zone_type.zone_map_location,pZone->ID());
		else
			Level().MapManager().RemoveMapLocation(*zone_type.zone_map_location,pZone->ID());
	}
}

void CAnomDetectorBase::UpdateMapLocations() // called on turn on/off only
{
	ZONE_INFO_MAP_IT it;
	for(it = m_ZoneInfoMap.begin(); it != m_ZoneInfoMap.end(); ++it)
		AddRemoveMapSpot(it->first,IsWorking());
}

void CAnomDetectorBase::UpdateNightVisionMode()
{
//	CObject* tmp = Level().CurrentViewEntity();	
	bool bNightVision = false;

	bNightVision = Actor()->Cameras().GetPPEffector(EEffectorPPType(effNightvision))!=NULL;
	
	bool bOn =	bNightVision && 
				m_pCurrentActor &&
				m_pCurrentActor==Level().CurrentViewEntity()&& 
				IsWorking() && 
				m_nightvision_particle.size();

	ZONE_INFO_MAP_IT it;
	for(it = m_ZoneInfoMap.begin(); m_ZoneInfoMap.end() != it; ++it) 
	{
		CCustomZone *pZone = it->first;
		ZONE_INFO& zone_info = it->second;

		if(bOn){
			Fvector zero_vector;
			zero_vector.set(0.f,0.f,0.f);

			if(!zone_info.pParticle)
				zone_info.pParticle = CParticlesObject::Create(*m_nightvision_particle,FALSE);
			
			zone_info.pParticle->UpdateParent(pZone->XFORM(),zero_vector);
			if(!zone_info.pParticle->IsPlaying())
				zone_info.pParticle->Play(false);
		}else{
			if(zone_info.pParticle){
				zone_info.pParticle->Stop			();
				DestroyParticleInstance(zone_info.pParticle);
			}
		}
	}
}
