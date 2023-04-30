#pragma once

class CWound;
class NET_Packet;
class CEntityAlive;
class CLevel;

#include "hit_immunity.h"
#include "Hit.h"

class CEntityConditionSimple
{
	float					m_fHealth;
	float					m_fHealthMax;
public:
							CEntityConditionSimple	();
	virtual					~CEntityConditionSimple	();

	IC		float				GetHealth				() const				{return m_fHealth;}
	IC		void				SetHealth				( const float value ) 	{ m_fHealth = value; }
	IC 		float 				GetMaxHealth			() const				{return m_fHealthMax;}
	IC 		float&				max_health				()						{return	m_fHealthMax;}
	IC const float&				health					() const				{return	m_fHealth;}
};

class CEntityCondition: public CEntityConditionSimple, public CHitImmunity
{
private:
	bool					m_use_limping_state;
	CEntityAlive			*m_object;

public:
							CEntityCondition		(CEntityAlive *object);
	virtual					~CEntityCondition		();

	virtual void			LoadCondition			(LPCSTR section);
	virtual void			LoadTwoHitsDeathParams	(LPCSTR section);
	virtual void			remove_links			(const CObject *object);

	virtual void			save					(NET_Packet &output_packet);
	virtual void			load					(IReader &input_packet);

	virtual float			GetSatiety				() const			{return m_fSatiety;}
	virtual float			GetThirsty				() const			{return m_fThirsty;}
	virtual float xr_stdcall GetAlcohol() const							{ return m_fAlcohol; }

	IC float				GetPower				() const			{return m_fPower;}	
	IC float				GetRadiation			() const			{return m_fRadiation;}
	IC float				GetPsyHealth			() const			{return m_fPsyHealth;}
	IC float				GetFireBurning			() const			{return m_fFireBurning;}
	IC float				GetChemBurning			() const			{return m_fChemBurning;}
	
	IC float 				GetEntityMorale			() const			{return m_fEntityMorale;}

	IC float 				GetHealthLost			() const			{return m_fHealthLost;}

	virtual bool 			IsLimping				() const;

	virtual void			ChangeSatiety			(const float value)		{};
	virtual void			ChangeThirsty			(const float value)		{};
	virtual void 			ChangeHealth			(const float value);
	virtual void 			ChangePower				(const float value);
	virtual void 			ChangeRadiation			(const float value);
	virtual void 			ChangePsyHealth			(const float value);
	virtual void 			ChangeFireBurning		(const float value);
	virtual void 			ChangeChemBurning		(const float value);
	virtual void 			ChangeAlcohol			(const float value){};

	IC void					MaxPower				()					{m_fPower = m_fPowerMax;};
	IC void					SetMaxPower				(const float val)	{m_fPowerMax = val; clamp(m_fPowerMax,0.1f,1.0f);};
	IC float				GetMaxPower				() const			{return m_fPowerMax;};

	void 					ChangeBleeding			(const float percent);

	void 					ChangeCircumspection	(const float value);
	void 					ChangeEntityMorale		(const float value);

	virtual CWound*			ConditionHit			(SHit* pHDS);
	//���������� ��������� � �������� �������
	virtual void			UpdateCondition			();
	void					UpdateWounds			();
	void					UpdateConditionTime		();
	IC void					SetConditionDeltaTime	(float DeltaTime) { m_fDeltaTime = DeltaTime; };

	
	//�������� ������ ����� �� ���� �������� ��� 
	float					BleedingSpeed			();

	CObject*				GetWhoHitLastTime		() {return m_pWho;}
	u16						GetWhoHitLastTimeID		() {return m_iWhoID;}

	CWound*					AddWound				(float hit_power, ALife::EHitType hit_type, u16 element);

	IC void 				SetCanBeHarmedState		(bool CanBeHarmed) 			{m_bCanBeHarmed = CanBeHarmed;}
	IC bool					CanBeHarmed				() const					{return m_bCanBeHarmed;};
	
	void					ClearWounds();
protected:
	void					UpdateHealth			();
	void					UpdatePower				();
	void					UpdateSatiety			(float k=1.0f);
	void					UpdateThirsty			(float k=1.0f);
	void					UpdateRadiation			(float k=1.0f);
	void					UpdatePsyHealth			(float k=1.0f);
	void					UpdateEntityMorale		();
	void					UpdateFireBurning		(float k=1.0f);
	void					UpdateChemBurning		(float k=1.0f);


	//��������� ���� ���� � ����������� �� �������� �������
	//(������ ��� InventoryOwner)
	float					HitOutfitEffect(float hit_power, ALife::EHitType hit_type, s16 element, float AP, bool& add_wound);
	//��������� ������ ��� � ����������� �� �������� �������
	float					HitPowerEffect			(float power_loss);
	
	//��� �������� ��������� �������� ���,
	//������������ ����� ���� ��� ������� ���
	//� �������� ������ ����� �� ����
	DEFINE_VECTOR(CWound*, WOUND_VECTOR, WOUND_VECTOR_IT);
	WOUND_VECTOR			m_WoundVector;
	//������� ������� ���
	

	//��� �������� �� 0 �� 1			
	float m_fPower;					//����
	float m_fRadiation;				//���� ������������� ���������
	float m_fPsyHealth;				//��������
	float m_fEntityMorale;			//������
	float m_fFireBurning;			//adds some hit after "burn" 
	float m_fChemBurning;			//adds some hit after "chemical_burn" hit

	float m_fThirsty;
	float m_fSatiety;
	float m_fAlcohol;

	//������������ ��������
	float m_fPowerMax;
	float m_fRadiationMax;
	float m_fPsyHealthMax;
	float m_fEntityMoraleMax;
	float m_fFireBurningMax;
	float m_fChemBurningMax;

	//�������� ��������� ���������� �� ������ ����������
	float m_fDeltaHealth;
	float m_fDeltaPower;
	float m_fDeltaRadiation;
	float m_fDeltaPsyHealth;
	float m_fDeltaEntityMorale;
	float m_fDeltaFireBurning;
	float m_fDeltaChemBurning;

	float m_fDeltaCircumspection;

	struct SConditionChangeV
	{
		float			m_fV_Radiation;
		float			m_fV_PsyHealth;
		float			m_fV_Circumspection;
		float			m_fV_EntityMorale;
		float			m_fV_FireBurning;
		float			m_fV_ChemBurning;
		float			m_fV_RadiationHealth;
		float			m_fV_BurningHealth;
		float			m_fV_Bleeding;
		float			m_fV_WoundIncarnation;
		float			m_fV_HealthRestore;
		void			load(LPCSTR sect, LPCSTR prefix);
	};
	
	SConditionChangeV m_change_v;

	float				m_fMinWoundSize;
	bool				m_bIsBleeding;

	//����� ����, ������������� �� ���������� �������� � ����
	float				m_fHealthHitPart;
	float				m_fPowerHitPart;



	//������ �������� �� ���������� ����
	float				m_fHealthLost;

	float				m_fKillHitTreshold;
	float				m_fLastChanceHealth;
	float				m_fInvulnerableTime;
	float				m_fInvulnerableTimeDelta;

	//��� ������������ ������� 
	u64					m_iLastTimeCalled;
	float				m_fDeltaTime;
	//��� ����� ��������� ���
	CObject*			m_pWho;
	u16					m_iWhoID;

	//��� �������� ���������� �� DamageManager
	float				m_fHitBoneScale;
	float				m_fWoundBoneScale;

	float				m_limping_threshold;

	bool				m_bTimeValid;
	bool				m_bCanBeHarmed;

public:
	virtual void					reinit				();
	
	IC const	float				fdelta_time			() const 	{return		(m_fDeltaTime);			}
	IC const	WOUND_VECTOR&		wounds				() const	{return		(m_WoundVector);		}
	IC float&						radiation			()			{return		(m_fRadiation);			}
	IC float&						fire_burning		()			{return		(m_fFireBurning);		}
	IC float&						chem_burning		()			{return		(m_fChemBurning);		}
	IC float&						hit_bone_scale		()			{return		(m_fHitBoneScale);		}
	IC float&						wound_bone_scale	()			{return		(m_fWoundBoneScale);	}
	IC SConditionChangeV&			change_v			()			{return		(m_change_v);			}
};
