//////////////////////////////////////////////////////////////////////////
// character_info.cpp			игровая информация для персонажей в игре
// 
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "character_info.h"
#include "ui/xrUIXmlParser.h"

#ifdef XRGAME_EXPORTS

#include "PhraseDialog.h"
#include "xrServer_Objects_ALife_Monsters.h"

#endif

void _destroy_item_data_vector_cont(T_VECTOR* vec)
{
	T_VECTOR::iterator it = vec->begin();
	T_VECTOR::iterator it_e = vec->end();

	xr_vector<CUIXml*>			_tmp;
	for (; it != it_e; ++it)
	{
		xr_vector<CUIXml*>::iterator it_f = std::find(_tmp.begin(), _tmp.end(), (*it)._xml);
		if (it_f == _tmp.end())
			//.		{
			_tmp.push_back((*it)._xml);
		//.			Msg("%s is unique",(*it)._xml->m_xml_file_name);
		//.		}else
		//.			Msg("%s already in list",(*it)._xml->m_xml_file_name);

	}
	//.	Log("_tmp.size()",_tmp.size());
	delete_data(_tmp);
}

//////////////////////////////////////////////////////////////////////////
SCharacterProfile::SCharacterProfile()
{
	m_CharacterId			= NULL;
	characterProfileRank_	= NO_RANK;
	m_Reputation			= NO_REPUTATION;
}

SCharacterProfile::~SCharacterProfile()
{
}

//////////////////////////////////////////////////////////////////////////

CCharacterInfo::CCharacterInfo()
{
	m_ProfileId = NULL;
	m_SpecificCharacterId = NULL;

#ifdef XRGAME_EXPORTS
	characterCurrentRank_.set(NO_RANK);
	m_CurrentReputation.set(NO_REPUTATION);
	m_StartDialog = NULL;
	m_Sympathy = 0.0f;
#endif
}


CCharacterInfo::~CCharacterInfo()
{
}


void CCharacterInfo::Load(shared_str id)
{
	m_ProfileId = id;
	inherited_shared::load_shared(m_ProfileId, NULL);
}

#ifdef XRGAME_EXPORTS

void CCharacterInfo::InitSpecificCharacter (shared_str new_id)
{
	R_ASSERT(new_id.size());
	m_SpecificCharacterId = new_id;

	m_SpecificCharacter.LoadSCharacter(m_SpecificCharacterId);
	if (GetCharacterRank().value() == NO_RANK)
		SetCharacterRank(m_SpecificCharacter.GetSpecCRank());
	if(Reputation().value() == NO_REPUTATION)
		SetReputation(m_SpecificCharacter.Reputation());
	if(Community().index() == NO_COMMUNITY_INDEX)
		SetCommunity(m_SpecificCharacter.Community().index());
	if(!m_StartDialog || !m_StartDialog.size() )
		m_StartDialog = m_SpecificCharacter.data()->m_StartDialog;
}


#endif

void CCharacterInfo::load_shared(LPCSTR section)
{
	const ITEM_DATA& item_data = *id_to_index::GetById(m_ProfileId);

	R_ASSERT2(&item_data, make_string("Error reading character info profile data [%s]. It is missing in xmls [%s]", m_ProfileId.c_str(), section ? section : "no sexion"));

	CUIXml*		pXML		= item_data._xml;
	pXML->SetLocalRoot		(pXML->GetRoot());

	XML_NODE* item_node = pXML->NavigateToNode(id_to_index::tag_name, item_data.pos_in_file);
	R_ASSERT3(item_node, "profile id=", *item_data.id);

	pXML->SetLocalRoot(item_node);



	LPCSTR spec_char = pXML->Read("specific_character", 0, NULL);
	if(!spec_char)
	{
		data()->m_CharacterId	= NULL;
		
		LPCSTR char_class			= pXML->Read	("class",		0,	NULL);

		if(char_class)
		{
			char* buf_str = xr_strdup(char_class);
			xr_strlwr(buf_str);
			data()->m_Class				= buf_str;
			xr_free(buf_str);
		}
		else
			data()->m_Class				= NO_CHARACTER_CLASS;
			
		data()->characterProfileRank_	= pXML->ReadInt("rank", 0, NO_RANK);
		data()->m_Reputation			= pXML->ReadInt("reputation", 0, NO_REPUTATION);
	}
	else
		data()->m_CharacterId = spec_char;
}

#ifdef XRGAME_EXPORTS
void CCharacterInfo::InitCInfo(CSE_ALifeTraderAbstract* trader)
{
	SetCommunity				(trader->m_community_index);
	SetCharacterRank			(trader->rankServer_);
	SetReputation				(trader->m_reputation);
	Load						(trader->character_profile());
	InitSpecificCharacter		(trader->specific_character());
}


shared_str CCharacterInfo::Profile()			const
{
	return m_ProfileId;
}

LPCSTR CCharacterInfo::Name() const
{
	R_ASSERT(m_SpecificCharacterId.size());

	if (m_SpecificCharacter.data() && m_SpecificCharacter.Name())
		return	m_SpecificCharacter.Name();
	else
		return "null";
}

shared_str CCharacterInfo::Bio() const
{
	return 	m_SpecificCharacter.Bio();
}

void CCharacterInfo::SetCharacterRank(CHARACTER_RANK_VALUE rank)
{
	characterCurrentRank_.set(rank);
}

void CCharacterInfo::SetReputation (CHARACTER_REPUTATION_VALUE reputation)
{
	m_CurrentReputation.set(reputation);
}

const shared_str& CCharacterInfo::IconName() const
{
	R_ASSERT(m_SpecificCharacterId.size());
	return m_SpecificCharacter.IconName();
}

shared_str	CCharacterInfo::StartDialog	()	const
{
	return m_StartDialog;
}

const DIALOG_ID_VECTOR&	CCharacterInfo::ActorDialogs	()	const
{
	R_ASSERT(m_SpecificCharacterId.size());
	return m_SpecificCharacter.data()->m_ActorDialogs;
}

void CCharacterInfo::load	(IReader& stream)
{
	stream.r_stringZ	(m_StartDialog);
}

void CCharacterInfo::save	(NET_Packet& stream)
{
	stream.w_stringZ	(m_StartDialog);
}

#endif



void CCharacterInfo::InitXmlIdToIndex()
{
	if(!id_to_index::tag_name)
		id_to_index::tag_name = "character";
	if(!id_to_index::file_str)
		id_to_index::file_str = pSettings->r_string("profiles", "files");
}
