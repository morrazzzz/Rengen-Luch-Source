#include "pch_script.h"
#include "actor.h"
#include "UIGameSP.h"
#include "PDA.h"
#include "level.h"
#include "character_info.h"
#include "relation_registry.h"
#include "alife_registry_container.h"
#include "script_game_object.h"
#include "xrServer.h"
#include "alife_registry_wrappers.h"
#include "map_manager.h"
#include "ui/UIMainIngameWnd.h"
#include "ui/UIPdaWnd.h"
#include "ui/UIDiaryWnd.h"
#include "ui/UITalkWnd.h"
#include "game_object_space.h"
#include "script_callback_ex.h"
#include "encyclopedia_article.h"
#include "GameTaskManager.h"
#include "GameTaskdefs.h"
#include "infoportion.h"
#include "ai/monsters/basemonster/base_monster.h"
#include "ai/trader/ai_trader.h"
#include "ai/stalker/ai_stalker.h"
#include "map_location.h"
#include "patrol_path_params.h"
#include "patrol_path_manager.h"
#include "Inventory.h"
#include "ArtDetectorBase.h"

void CActor::AddEncyclopediaArticle(const CInfoPortion* info_portion) const
{
	VERIFY(info_portion);

	ARTICLE_VECTOR& article_vector = encyclopedia_registry->registry().objects();

	ARTICLE_VECTOR::iterator last_end = article_vector.end();
	ARTICLE_VECTOR::iterator B = article_vector.begin();
	ARTICLE_VECTOR::iterator E = last_end;

	for(ARTICLE_ID_VECTOR::const_iterator it = info_portion->ArticlesDisable().begin(); it != info_portion->ArticlesDisable().end(); it++)
	{
		FindArticleByIDPred pred(*it);
		last_end = std::remove_if(B, last_end, pred);
	}

	article_vector.erase(last_end, E);

	for(ARTICLE_ID_VECTOR::const_iterator it = info_portion->Articles().begin(); it != info_portion->Articles().end(); it++)
	{
		FindArticleByIDPred pred(*it);

		if(std::find_if(article_vector.begin(), article_vector.end(), pred) != article_vector.end()) 
			continue;

		CEncyclopediaArticle article;

		article.Load(*it);

		article_vector.push_back(ARTICLE_DATA(*it, GetGameTime(), article.data()->articleType));
		LPCSTR g,n;
		int _atype = article.data()->articleType;
		g = *(article.data()->group);
		n = *(article.data()->name);

		callback(GameObject::eArticleInfo)(lua_game_object(), g, n, _atype);

		if(CurrentGameUI())
		{
			CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(CurrentGameUI());
			pda_section::part p = pda_section::encyclopedia;

			switch (article.data()->articleType){
				case ARTICLE_DATA::eEncyclopediaArticle:	p = pda_section::encyclopedia;	break;
				case ARTICLE_DATA::eJournalArticle:			p = pda_section::journal;		break;
				case ARTICLE_DATA::eInfoArticle:			p = pda_section::info;			break;
				case ARTICLE_DATA::eTaskArticle:			p = pda_section::quests;		break;
				default: NODEFAULT;
			};

			pGameSP->m_PdaMenu->PdaContentsChanged(p);
		}

	}

}


void CActor::AddGameTask(const CInfoPortion* info_portion) const
{
	VERIFY(info_portion);

	if(info_portion->GameTasks().empty())
		return;

	for(TASK_ID_VECTOR::const_iterator it = info_portion->GameTasks().begin(); it != info_portion->GameTasks().end(); it++)
	{
		Level().GameTaskManager().GiveGameTaskToActor(*it, 0);
	}
}

extern u16 map_add_position_spot_ser(Fvector position, LPCSTR levelName, LPCSTR spot_type, LPCSTR text);

void CActor::AddMapLocation(const CInfoPortion* info_portion) const
{
	VERIFY(info_portion);

	if (info_portion->MapLocations().empty())
		return;

	for (TASK_ID_VECTOR::const_iterator it = info_portion->MapLocations().begin(); it != info_portion->MapLocations().end(); it++)
	{
		LPCSTR section = **it;

		R_ASSERT2(pSettings->section_exist(section), section);

		if (pSettings->section_exist(section))
		{
			LPCSTR way_point_name = pSettings->r_string(section, "way_point_name");
			LPCSTR level_name = pSettings->r_string(section, "level_name");
			LPCSTR spot_type = pSettings->r_string(section, "spot_type");
			LPCSTR hint = pSettings->r_string(section, "hint");

			CPatrolPathParams way_point = CPatrolPathParams(way_point_name);

			if (way_point.m_path)
			{
				const Fvector& position = way_point.point(u32(0));

				map_add_position_spot_ser(position, level_name, spot_type, hint);

				callback(GameObject::eMapSpecialLocationDiscovered)(level_name, hint);

				Msg("- Map Location Added: %s", section);
			}
		}
	}
}



void CActor::AddGameNews(GAME_NEWS_DATA& news_data)
{

	GAME_NEWS_VECTOR& news_vector	= game_news_registry->registry().objects();
	news_data.receive_time			= GetGameTime();
	news_vector.push_back			(news_data);

	if(CurrentGameUI())
	{
		CurrentGameUI()->UIMainIngameWnd->ReceiveNews(&news_data);
		CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(CurrentGameUI());

		if(pGameSP) 
			pGameSP->m_PdaMenu->PdaContentsChanged	(pda_section::news);
	}
}


bool CActor::OnReceiveInfo(shared_str info_id) const
{
	if(!CInventoryOwner::OnReceiveInfo(info_id))
		return false;

	if (CInfoPortion::IsComplexInfoPortion(*info_id))
	{
		CInfoPortion info_portion;
		info_portion.Load(info_id);

		AddEncyclopediaArticle	(&info_portion);
		AddGameTask				(&info_portion);
		AddMapLocation			(&info_portion);
	}

	callback(GameObject::eInventoryInfo)(lua_game_object(), *info_id);

	if(!CurrentGameUI())
		return false;

	//только если находимся в режиме single
	CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(CurrentGameUI());

	if(!pGameSP)
		return false;

	if(pGameSP->TalkMenu && pGameSP->TalkMenu->IsShown())
	{
		pGameSP->TalkMenu->NeedUpdateQuestions();
	}


	return true;
}

void CActor::OnDisableInfo(shared_str info_id) const
{
	CInventoryOwner::OnDisableInfo(info_id);

	if(!CurrentGameUI())
		return;

	//только если находимся в режиме single
	CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(CurrentGameUI());

	if(!pGameSP) return;

	if(pGameSP->TalkMenu && pGameSP->TalkMenu->IsShown())
		pGameSP->TalkMenu->NeedUpdateQuestions();
}

void CActor::ReceivePhrase(DIALOG_SHARED_PTR& phrase_dialog)
{
	//только если находимся в режиме single
	CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(CurrentGameUI());

	if(!pGameSP)
		return;

	if(pGameSP->TalkMenu && pGameSP->TalkMenu->IsShown())
		pGameSP->TalkMenu->NeedUpdateQuestions();

	CPhraseDialogManager::ReceivePhrase(phrase_dialog);
}

void CActor::UpdateAvailableDialogs(CPhraseDialogManager* partner)
{
	m_AvailableDialogs.clear();
	m_CheckedDialogs.clear();

	if(CInventoryOwner::m_known_info_registry->registry().objects_ptr())
	{
		for(KNOWN_INFO_VECTOR::const_iterator it = CInventoryOwner::m_known_info_registry->registry().objects_ptr()->begin();
			CInventoryOwner::m_known_info_registry->registry().objects_ptr()->end() != it; ++it)
		{
			CInfoPortion info_portion;

			if (!CInfoPortion::IsComplexInfoPortion(*((*it).info_id)))
				continue;
			
			info_portion.Load((*it).info_id);

			for(u32 i = 0; i<info_portion.DialogNames().size(); i++)
				AddAvailableDialog(*info_portion.DialogNames()[i], partner);
		}
	}

	//добавить актерский диалог собеседника
	CInventoryOwner* pInvOwnerPartner = smart_cast<CInventoryOwner*>(partner); VERIFY(pInvOwnerPartner);
	
	for(u32 i = 0; i<pInvOwnerPartner->CharacterInfo().ActorDialogs().size(); i++)
		AddAvailableDialog(pInvOwnerPartner->CharacterInfo().ActorDialogs()[i], partner);

	CPhraseDialogManager::UpdateAvailableDialogs(partner);
}

void CActor::TryToTalk()
{
	VERIFY(m_pPersonWeLookingAt);

	if(!IsTalking())
	{
		RunTalkDialog(m_pPersonWeLookingAt);
	}
}

void CActor::RunTalkDialog(CInventoryOwner* talk_partner, bool* custom_break_bool)
{
	//предложить поговорить с нами
	if(talk_partner->OfferTalk(this))
	{	
		StartTalk(talk_partner);
		//только если находимся в режиме single
		CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(CurrentGameUI());

		if(pGameSP)
		{
			if(pGameSP->TopInputReceiver())
				pGameSP->TopInputReceiver()->HideDialog();

			pGameSP->StartTalk(custom_break_bool ? *custom_break_bool : talk_partner->bDisableBreakDialog);
		}
	}
}

void CActor::StartTalk (CInventoryOwner* talk_partner)
{
	PIItem det_active = inventory().ItemFromSlot(DETECTOR_SLOT);
	if (det_active)
	{
		CArtDetectorBase* det = smart_cast<CArtDetectorBase*>(det_active);
		det->HideDetector(true);
	}

	CGameObject* GO = smart_cast<CGameObject*>(talk_partner); VERIFY(GO);

	CInventoryOwner::StartTalk(talk_partner);
}


void CActor::NewPdaContact(CInventoryOwner* pInvOwner)
{
	if (pInvOwner && pInvOwner->special_map_spot && pInvOwner->special_map_spot->IsShown()) //if we assigned map spot in character_desc then dont add regular map spot
	{
		return;
	}

	bool b_alive = !!(smart_cast<CEntityAlive*>(pInvOwner))->g_Alive();

	CAI_Stalker* pStalker = smart_cast<CAI_Stalker*>(pInvOwner);

	if (pStalker && pStalker->IsGhost())
		return;

	CurrentGameUI()->UIMainIngameWnd->AnimateContacts(b_alive);

	Level().MapManager().AddRelationLocation(pInvOwner);

	if(CurrentGameUI())
	{
		CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(CurrentGameUI());

		if(pGameSP)
			pGameSP->m_PdaMenu->PdaContentsChanged(pda_section::contacts);
	}
}

void CActor::LostPdaContact(CInventoryOwner* pInvOwner)
{
	CGameObject* GO = smart_cast<CGameObject*>(pInvOwner);

	if (GO)
	{

		for(int t = ALife::eRelationTypeFriend; t<ALife::eRelationTypeLast; ++t)
		{
			ALife::ERelationType tt = (ALife::ERelationType)t;
			Level().MapManager().RemoveMapLocation(RELATION_REGISTRY().GetSpotName(tt),	GO->ID());
		}

		Level().MapManager().RemoveMapLocation("deadbody_location",	GO->ID());
	};

	if(CurrentGameUI())
	{
		CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(CurrentGameUI());

		if(pGameSP)
		{
			pGameSP->m_PdaMenu->PdaContentsChanged	(pda_section::contacts);
		}
	}

}

void CActor::AddGameNews_deffered(GAME_NEWS_DATA& news_data, u32 delay)
{
	GAME_NEWS_DATA* d = xr_new <GAME_NEWS_DATA>(news_data);

	m_defferedMessages.push_back( SDefNewsMsg() );
	m_defferedMessages.back().news_data = d;
	m_defferedMessages.back().time = EngineTimeU() + delay;

	std::sort(m_defferedMessages.begin(), m_defferedMessages.end());
}

void CActor::UpdateDefferedMessages()
{
	while( m_defferedMessages.size())
	{
		SDefNewsMsg& M = m_defferedMessages.back();

		if (M.time <= EngineTimeU())
		{
			AddGameNews(*M.news_data);		
			xr_delete(M.news_data);

			m_defferedMessages.pop_back();
		}
		else
			break;
	}
}

bool CActor::OnDialogSoundHandlerStart(CInventoryOwner *inv_owner, LPCSTR phrase)
{
	CAI_Trader* trader = smart_cast<CAI_Trader*>(inv_owner);

	if (!trader)
		return false;

	trader->dialog_sound_start(phrase);

	return true;
}

bool CActor::OnDialogSoundHandlerStop(CInventoryOwner *inv_owner)
{
	CAI_Trader* trader = smart_cast<CAI_Trader*>(inv_owner);

	if (!trader)
		return false;

	trader->dialog_sound_stop();

	return true;
}

void CActor::DumpTasks()
{
	Level().GameTaskManager().DumpTasks();
}
