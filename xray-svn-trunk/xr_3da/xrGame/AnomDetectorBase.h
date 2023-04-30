#pragma once
#include "inventory_item_object.h"
#include "../feel_touch.h"
#include "hudsound.h"

class CCustomZone;
//описание типа зоны
struct ZONE_TYPE
{
	//интервал частот отыгрывани€ звука
	float		min_freq;
	float		max_freq;
	//звук реакции детектора на конкретную зону
//	ref_sound	detect_snd;
	HUD_SOUND_ITEM	detect_snds;

	shared_str	zone_map_location;
};

//описание зоны, обнаруженной детектором
struct ZONE_INFO
{
	u32								snd_time;
	//текуща€ частота работы датчика
	float							cur_freq;
	//particle for night-vision mode
	CParticlesObject*				pParticle;

	ZONE_INFO	();
	~ZONE_INFO	();
};

class CInventoryOwner;

class CAnomDetectorBase :
	public CInventoryItemObject,
	public Feel::Touch
{
	typedef	CInventoryItemObject	inherited;
public:
	CAnomDetectorBase(void);
	virtual ~CAnomDetectorBase(void);

	virtual void LoadCfg			(LPCSTR section);
	
	virtual BOOL SpawnAndImportSOData(CSE_Abstract* data_containing_so);

	virtual void AfterAttachToParent	();
	virtual void BeforeDetachFromParent	(bool just_before_destroy);

	virtual void ScheduledUpdate		(u32 dt);
	virtual void UpdateCL			();

	virtual void feel_touch_new		(CObject* O);
	virtual void feel_touch_delete	(CObject* O);
	virtual BOOL feel_touch_contact	(CObject* O);

			void TurnOn				();
			void TurnOff			();
			bool IsWorking			() {return m_bWorking;}

	virtual void OnMoveToSlot		();
	virtual void OnMoveToRuck		();
	virtual void OnMoveToBelt		();

protected:
	void StopAllSounds				();
	void UpdateMapLocations			();
	void AddRemoveMapSpot			(CCustomZone* pZone, bool bAdd);
	void UpdateNightVisionMode		();

	bool m_bWorking;

	float m_fRadius;

	//если хоз€ин текущий актер
	CActor*				m_pCurrentActor;
	CInventoryOwner*	m_pCurrentInvOwner;

	//информаци€ об онаруживаемых зонах
	DEFINE_MAP(CLASS_ID, ZONE_TYPE, ZONE_TYPE_MAP, ZONE_TYPE_MAP_IT);
	ZONE_TYPE_MAP m_ZoneTypeMap;
	
	//список обнаруженных зон и информаци€ о них
	DEFINE_MAP(CCustomZone*, ZONE_INFO, ZONE_INFO_MAP, ZONE_INFO_MAP_IT);
	ZONE_INFO_MAP m_ZoneInfoMap;
	
	shared_str						m_nightvision_particle;

protected:
	u32					m_ef_detector_type;

public:
	virtual u32			ef_detector_type	() const;
};
