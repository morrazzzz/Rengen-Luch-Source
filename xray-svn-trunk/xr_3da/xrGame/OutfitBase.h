#pragma once

#include "inventory_item_object.h"
#include "hudsound.h"
#include "NightVisionUsable.h"


struct SBoneProtections;
class CBinocularsVision;

class COutfitBase
	: public CInventoryItemObject
	, public CNightVisionUsable
{
private:
    typedef	CInventoryItemObject inherited;
public:
									COutfitBase			(void);
	virtual							~COutfitBase		(void);

	void							LoadCfg				(LPCSTR section) override;
	
	BOOL							SpawnAndImportSOData(CSE_Abstract* data_containing_so) override;
	void							RemoveLinksToCLObj	(CObject* object) override;

	virtual void					save				(NET_Packet &output_packet);
	virtual void					load				(IReader &input_packet);
	
	void							UpdateCL			() override;
	
	bool							render_item_ui_query() override;
	void							render_item_ui		() override;

	//����������� ������ ����, ��� ������, ����� ������ ����� �� ���������
	virtual void					Hit					(float P, ALife::EHitType hit_type);

	//������������ �� ������� ����������� ���
	//��� ��������������� ���� �����������
	//���� �� ��������� ����� ������
	float							GetHitTypeProtection(ALife::EHitType hit_type, s16 element);
	float							GetDefHitTypeProtection(ALife::EHitType hit_type);

	////tatarinrafa: ������ �� �� ������� ����
	float							GetBoneArmor(s16 element);
	float							HitThroughArmor(float hit_power, s16 element, float ap, bool& add_wound, ALife::EHitType hit_type);
	
	float							GetArmorBody			();
	float							GetArmorHead			();

	//����������� �� ������� ����������� ������ ����
	//���� �� ��������� ����� ������
	float							GetPowerLoss		() const;

	float							GetHealthRestoreSpeed() const { return m_fHealthRestoreSpeed; }
	float							GetRadiationRestoreSpeed() const { return m_fRadiationRestoreSpeed; }
	float							GetSatietyRestoreSpeed() const { return m_fSatietyRestoreSpeed; }
	float							GetPowerRestoreSpeed() const { return m_fPowerRestoreSpeed; }
	float							GetBleedingRestoreSpeed() const { return m_fBleedingRestoreSpeed; }

//	Alive detector methods
	bool							GetAliveDetectorState() const;
	virtual void					SwitchAliveDetector();
	virtual void					SwitchAliveDetector(bool state);

	virtual void					OnMoveToRuck		();

protected:
	HitImmunity::HitTypeSVec		m_HitTypeProtection;
	float							m_fPowerLoss;
	float							m_fHealthRestoreSpeed;
	float 							m_fRadiationRestoreSpeed;
	float 							m_fSatietyRestoreSpeed;
	float							m_fPowerRestoreSpeed;
	float							m_fBleedingRestoreSpeed;

	SBoneProtections*				m_boneProtection;
	
	// ��� ������ ��� ���� � ������, �� ������� �������� ������� ����� ��� ������ � UI
	shared_str						m_armorTestBoneBody;
	shared_str						m_armorTestBoneHead;

	float							GetArmorByBoneName		(const shared_str& boneName);

//	Alive detector fields
	shared_str						m_alive_detector_upgrade_sect;
	CBinocularsVision*				m_adetector;
	bool							m_alive_detector_state;
public:
	shared_str						m_BonesProtectionSect;

	virtual	BOOL					BonePassBullet			(int boneID);
	
			void			ReloadBonesProtection	();
			void			AddBonesProtection		(LPCSTR bones_section);

	// ������ ����������� ���������� ������ �� ������
	float							visorWetness_1_;

	// ��� ������� ������� ��������� ������ ��� ��� ����� �� ����� ����� ������ ��� �� ����
	float							visorWetnessSpreadValue_1_;

	// �������� �� ������� ������, ����� �� ������, ����� ������
	bool							hasVisorEffects_;

	BOOL							block_pnv_slot;

protected:
	virtual bool			install_upgrade_impl( LPCSTR section, bool test );
};
