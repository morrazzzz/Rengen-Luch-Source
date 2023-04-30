//////////////////////////////////////////////////////////////////////////
// specific_character.h:	������� ���������� ��� � ���������� 
//							���������� � ����
//////////////////////////////////////////////////////////////////////////

#pragma		once

#include "character_info_defs.h"
#include "character_supplies.h"
#include "shared_data.h"
#include "xml_str_id_loader.h"


#ifdef XRGAME_EXPORTS

#include "PhraseDialogDefs.h"
#include "character_community.h"

#endif


//////////////////////////////////////////////////////////////////////////
// SSpecificCharacterData: ������ � ���������� ���������
//////////////////////////////////////////////////////////////////////////
struct SSpecificCharacterData : CSharedResource
{
#ifdef  XRGAME_EXPORTS

	SSpecificCharacterData ();
	virtual ~SSpecificCharacterData ();

	//������� ��� ���������
	xr_string	m_sGameName;
	//����� � ���������� ��������� (���� �� string table)
	shared_str	m_sBioText;
	//������ ���������� ��������, ������� ����� ����������� 
	xr_string	m_sSupplySpawn;
	SSpecificCharacterSupplies m_supplies;

	//��� ������ ������������ �������� NPC ��� ���������
	xr_string	m_sNpcConfigSect;
	//��� ������ ������������ ����� ��� NPC ���������
	xr_string	m_sound_voice_prefix;

	float		m_fPanic_threshold;
	float		m_fHitProbabilityFactor;
	int			m_crouch_type;

	shared_str	m_can_upgrade;

	xr_string	m_critical_wound_weights;
#endif
	shared_str	m_terrain_sect;

	//��� ������
	xr_string m_sVisual;

	ALife::_STORY_ID speificStoryID_;

#ifdef  XRGAME_EXPORTS
	
	//��������� ������
	shared_str					m_StartDialog;
	//������� ������, ������� ����� �������� ������ ��� ������� � ������ ����������
	DIALOG_ID_VECTOR			m_ActorDialogs;

	shared_str					m_icon_name;
	//������� 
	CHARACTER_COMMUNITY			m_Community;

	bool						isTrader_;
#endif
	
	//����
	CHARACTER_RANK_VALUE		sCharRank_;
	//���������
	CHARACTER_REPUTATION_VALUE	m_Reputation;

	//������ ��������� (�������-��������, ������ � �.�.)
	//� ������� �� �����������
	xr_vector<CHARACTER_CLASS>	m_Classes;

	//����������
	bool						m_invulnerable;

	//�������� �� �� ��� �������� �� ������������ ��� ���������� ������
	//� �������� ������ ����� ����� �������� ID
	bool m_bNoRandom;
	//���� �������� �������� ������� �� ��������� ��� ����� �������
	bool m_bDefaultForCommunity;
#ifdef  XRGAME_EXPORTS
	struct SMoneyDef{
		u32				min_money;
		u32				max_money;
		bool			inf_money;
	};
	SMoneyDef			money_def;
#endif

	LPCSTR				s_map_spot;
};

class CInventoryOwner;
class CCharacterInfo;
class CSE_ALifeTraderAbstract;


class CSpecificCharacter: public CSharedClass<SSpecificCharacterData, shared_str, false>,
						  public CXML_IdToIndex<CSpecificCharacter>
{
private:
	typedef CSharedClass	<SSpecificCharacterData, shared_str, false>				inherited_shared;
	typedef CXML_IdToIndex	<CSpecificCharacter>									id_to_index;

	friend id_to_index;
	friend CInventoryOwner;
	friend CCharacterInfo;
	friend CSE_ALifeTraderAbstract;
public:

								CSpecificCharacter		();
								~CSpecificCharacter		();

	virtual void				LoadSCharacter					(shared_str		id);

protected:
	const SSpecificCharacterData* data					() const	{ VERIFY(inherited_shared::get_sd()); return inherited_shared::get_sd();}
	SSpecificCharacterData*		  data					()			{ VERIFY(inherited_shared::get_sd()); return inherited_shared::get_sd();}

	//�������� �� XML �����
	virtual void				load_shared				(LPCSTR section);
	static void					InitXmlIdToIndex		();

	shared_str		m_OwnId;
public:

	LPCSTR						Visual() const;
	ALife::_STORY_ID			SpecificStoryID()			{ return data()->speificStoryID_; }; // returns id specified in xml or u32(-1)

#ifdef  XRGAME_EXPORTS
	bool						IsTrader() const			{ return data()->isTrader_; };

	LPCSTR						Name					() const ;
	shared_str					Bio						() const ;
	const CHARACTER_COMMUNITY&	Community				() const ;
	SSpecificCharacterData::SMoneyDef& MoneyDef			() 	{return data()->money_def;}

	CHARACTER_RANK_VALUE		GetSpecCRank			() const ;
	CHARACTER_REPUTATION_VALUE	Reputation				() const ;

	LPCSTR						SupplySpawn				() const ;
	const SSpecificCharacterSupplies&	Supplies		() const ;
	LPCSTR						NpcConfigSect			() const ;
	LPCSTR						sound_voice_prefix		() const ;
	float						panic_threshold			() const ;
	float						hit_probability_factor	() const ;
	int							crouch_type				() const ;
	LPCSTR						critical_wound_weights	() const ;
	const shared_str&			CanUpgrade				() const ;

	const LPCSTR				MapSpotString			() const { return data()->s_map_spot; };

	const shared_str&			IconName				() const	{return data()->m_icon_name;};
#endif
	shared_str					terrain_sect			() const;

	// Character invulnerability on spawn, can by changed by script
	bool						Invulnerable			() const;
};


//////////////////////////////////////////////////////////////////////////

