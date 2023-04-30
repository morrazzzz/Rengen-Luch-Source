#pragma once

#include "OutfitBase.h"
#include "hudsound.h"

struct SBoneProtections;

class CCustomOutfit: public COutfitBase {
private:
    typedef	COutfitBase inherited;
public:
									CCustomOutfit		(void);
	virtual							~CCustomOutfit		(void);

	virtual void					LoadCfg				(LPCSTR section);

	virtual void					ExportDataToServer	(NET_Packet& P);
	virtual void					DestroyClientObj	();
	virtual BOOL					SpawnAndImportSOData(CSE_Abstract* data_containing_so);

	virtual void					BeforeDetachFromParent	(bool just_before_destroy);
	
	virtual void					OnMoveToSlot		();
	virtual void					OnMoveToRuck		();

protected:
	shared_str						m_ActorVisual;
	shared_str						m_ActorVisual_legs;
	shared_str						m_full_icon_name;

protected:
	u32								m_ef_equipment_type;

	bool								m_bNightVisionEnabled;
	bool								m_bNightVisionOn;

public:
	float							m_additional_jump_speed;
	float							m_additional_run_coef;
	float							m_additional_sprint_koef;

	u32								maxArtefactCount_; //Кол-во разрешенных артифактов на поясе
	u32								maxAmmoCount_; //Кол-во разрешенных пачек патронов на поясе

	BOOL							block_helmet_slot;
	float							m_additional_weight;
	float							m_additional_weight2;

	virtual u32						ef_equipment_type		() const;
	const shared_str&				GetFullIconName			() const	{return m_full_icon_name;};

protected:
	virtual bool			install_upgrade_impl	( LPCSTR section, bool test );
};
