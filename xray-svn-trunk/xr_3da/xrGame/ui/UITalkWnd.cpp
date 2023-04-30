#include "stdafx.h"
#include "UITalkWnd.h"

#include "UITalkDialogWnd.h"
#include "UITradeWnd.h"

#include "../actor.h"
#include "../UIGameSP.h"
#include "../level.h"

#include "../PhraseDialog.h"
#include "../PhraseDialogManager.h"

#include "../string_table.h"
#include "../xr_level_controller.h"
#include "../../cameraBase.h"
#include "UIXmlInit.h"
#include "UIUpgradeWnd.h"
#include "../GameConstants.h"

#define INIT_SUBWINDOW(cls, wnd)	wnd = xr_new <cls>(); \
									wnd->SetAutoDelete(true); \
									AttachChild(wnd); \
									wnd->Hide();

CUITalkWnd::CUITalkWnd()
{
	m_pActor				= NULL;

	m_pOurInvOwner			= NULL;
	m_pOthersInvOwner		= NULL;

	m_pOurDialogManager		= NULL;
	m_pOthersDialogManager	= NULL;

	ToTopicMode				();

	InitTalkWnd				();
	m_bNeedToUpdateQuestions = false;
	b_disable_break			= false;
}

//////////////////////////////////////////////////////////////////////////

CUITalkWnd::~CUITalkWnd()
{
}

void CUITalkWnd::InitTalkWnd()
{
	inherited::SetWndRect(Frect().set(0, 0, UI_BASE_WIDTH, UI_BASE_HEIGHT));

	UITalkDialogWnd			= xr_new <CUITalkDialogWnd>();
	UITalkDialogWnd->SetAutoDelete(true);
	AttachChild				(UITalkDialogWnd);
	UITalkDialogWnd->Init(0,0, UI_BASE_WIDTH, UI_BASE_HEIGHT);

	/////////////////////////
	//���� ��������
	//UITradeWnd = xr_new<CUITradeWnd>();UITradeWnd->SetAutoDelete(true);
	//AttachChild(UITradeWnd);

	//UITradeWnd->Hide();

	INIT_SUBWINDOW(CUITradeWnd, UITradeWnd);
	INIT_SUBWINDOW(CUIUpgradeWnd, UIUpgradeWnd);

}

//////////////////////////////////////////////////////////////////////////

void CUITalkWnd::InitTalkDialog()
{
	m_pActor = Actor();
	if (m_pActor && !m_pActor->IsTalking()) return;

	m_pOurInvOwner = smart_cast<CInventoryOwner*>(m_pActor);
	m_pOthersInvOwner = m_pActor->GetTalkPartner();

	m_pOurDialogManager = smart_cast<CPhraseDialogManager*>(m_pOurInvOwner);
	m_pOthersDialogManager = smart_cast<CPhraseDialogManager*>(m_pOthersInvOwner);

	//����� ������������
	UITalkDialogWnd->UICharacterInfoLeft.InitCharacter		(m_pOurInvOwner->object_id());
	UITalkDialogWnd->UICharacterInfoRight.InitCharacter		(m_pOthersInvOwner->object_id());

	UITalkDialogWnd->UIDialogFrame.UITitleText.SetText		(m_pOthersInvOwner->Name());
	UITalkDialogWnd->UIOurPhrasesFrame.UITitleText.SetText	(m_pOurInvOwner->Name());
	
	//�������� ��� ���������
	UITalkDialogWnd->ClearAll();

	InitOthersStartDialog					();
	NeedUpdateQuestions						();
	Update									();

	UITalkDialogWnd->SetOsoznanieMode		(m_pOthersInvOwner->NeedOsoznanieMode());
	UITalkDialogWnd->ShowTradeButton		(m_pOthersInvOwner->IsTradeEnabled(), m_pOthersInvOwner->IsUpgradeEnabled(m_pOurInvOwner));
	UITalkDialogWnd->Show					();

	UITalkDialogWnd->UIOurIcon.Show(GameConstants::GetUseCharInfoInWnds());
	UITalkDialogWnd->UIOthersIcon.Show(GameConstants::GetUseCharInfoInWnds());

	UITradeWnd->Hide						();
	UIUpgradeWnd->Hide						();
}

void CUITalkWnd::InitOthersStartDialog()
{
	m_pOthersDialogManager->UpdateAvailableDialogs(m_pOurDialogManager);
	if(!m_pOthersDialogManager->AvailableDialogs().empty())
	{
		m_pCurrentDialog = m_pOthersDialogManager->AvailableDialogs().front();
		m_pOthersDialogManager->InitDialog(m_pOurDialogManager, m_pCurrentDialog);
		
		//������� �����
		CStringTable stbl;
		AddAnswer(m_pCurrentDialog->GetPhraseText("0"), m_pOthersInvOwner->Name());
		m_pOthersDialogManager->SayPhrase(m_pCurrentDialog, "0");

		//���� ������ ����������, ������� � ����� ������ ����
		if(!m_pCurrentDialog || m_pCurrentDialog->IsFinished()) ToTopicMode();
	}
}

void CUITalkWnd::NeedUpdateQuestions()
{
	m_bNeedToUpdateQuestions = true;
}

void CUITalkWnd::UpdateQuestions()
{
	UITalkDialogWnd->ClearQuestions();

	//���� ��� ��������� �������, ��
	//������ ������ ����
	if(!m_pCurrentDialog)
	{
		m_pOurDialogManager->UpdateAvailableDialogs(m_pOthersDialogManager);
		int number = 0;
		for(u32 i=0; i< m_pOurDialogManager->AvailableDialogs().size(); ++i, ++number)
		{
			const DIALOG_SHARED_PTR& phrase_dialog = m_pOurDialogManager->AvailableDialogs()[i];

			shared_str caption = phrase_dialog->DialogCaption();

			if (!caption.size())
				--number;

			AddQuestion(phrase_dialog->DialogCaption(), phrase_dialog->GetDialogID(), number);
			//Msg("AddQuestion: caption=[%s] phrase_id=[%s] num=[%d]", phrase_dialog->DialogCaption(), *phrase_dialog->GetDialogID(), i);
		}
	} else {
		if(m_pCurrentDialog->IsWeSpeaking(m_pOurDialogManager))
		{
			//���� � ������ ���������� ���� ������ ���� ����� ��������, �� ������
			//������� (����� ��� �� ���������� ������� ��������)
			if( !m_pCurrentDialog->PhraseList().empty() && m_pCurrentDialog->allIsDummy() ){
				CPhrase* phrase = m_pCurrentDialog->PhraseList()[Random.randI(m_pCurrentDialog->PhraseList().size())];
				SayPhrase(phrase->GetID());
			};

			//����� ��������� ���� �� ��������� �������
			if( m_pCurrentDialog && !m_pCurrentDialog->allIsDummy() )
			{
				int number = 0;
				for(PHRASE_VECTOR::const_iterator   it = m_pCurrentDialog->PhraseList().begin();
					it != m_pCurrentDialog->PhraseList().end();
					++it, ++number)
				{
					CPhrase* phrase = *it;
					AddQuestion(phrase->GetText(), phrase->GetID(), number);
				}
			}
			else
				UpdateQuestions();
		}
	}
	m_bNeedToUpdateQuestions = false;

}

//////////////////////////////////////////////////////////////////////////

void CUITalkWnd::SendMessage(CUIWindow* pWnd, s16 msg, void* pData)
{
	if(pWnd == UITalkDialogWnd && msg == TALK_DIALOG_TRADE_BUTTON_CLICKED)
	{
		SwitchToTrade();
	}
	else if(pWnd == UITalkDialogWnd && msg == TALK_DIALOG_QUESTION_CLICKED)
	{
		AskQuestion();
	}
	else if(pWnd == UITradeWnd && msg == TRADE_WND_CLOSED)
	{
		UITalkDialogWnd->Show();
		UITradeWnd->Hide();
	}
	else if (pWnd == UIUpgradeWnd && msg == TRADE_WND_CLOSED)
	{
		UITalkDialogWnd->Show();
		UIUpgradeWnd->Hide();
	}
	inherited::SendMessage(pWnd, msg, pData);
}

//////////////////////////////////////////////////////////////////////////
void UpdateCameraDirection(CGameObject* pTo)
{
	CCameraBase* cam = Actor()->cam_Active();

	Fvector des_dir; 
	Fvector des_pt;
	pTo->Center(des_pt);
	des_pt.y+=pTo->Radius()*0.5f;

	des_dir.sub(des_pt,cam->vPosition);

	float p,h;
	des_dir.getHP(h,p);


	if(angle_difference(cam->yaw,-h)>0.2)
		cam->yaw		= angle_inertion_var(cam->yaw,		-h,	0.15f,	0.2f,	PI_DIV_6,	TimeDelta());

	if(angle_difference(cam->pitch,-p)>0.2)
		cam->pitch		= angle_inertion_var(cam->pitch,	-p,	0.15f,	0.2f,	PI_DIV_6,	TimeDelta());

}

void CUITalkWnd::Update()
{
	//���������� ��������, ���� �����
	if (g_actor && m_pActor && !m_pActor->IsTalking() )
	{
		StopTalk();
	}else{
		CGameObject* pOurGO = smart_cast<CGameObject*>(m_pOurInvOwner);
		CGameObject* pOtherGO = smart_cast<CGameObject*>(m_pOthersInvOwner);
	
		if(	NULL==pOurGO || NULL==pOtherGO )
			HideDialog();
	}

	if(m_bNeedToUpdateQuestions)
	{
		UpdateQuestions			();
	}
	inherited::Update			();
	UpdateCameraDirection		(smart_cast<CGameObject*>(m_pOthersInvOwner));

	if(playing_sound())
	{
		CGameObject* pOtherGO	= smart_cast<CGameObject*>(m_pOthersInvOwner);
		Fvector P				= pOtherGO->Position();
		P.y						+= 1.8f;
		m_sound.set_position	(P);
	}
}

void CUITalkWnd::Draw()
{
	inherited::Draw				();
}

void CUITalkWnd::Show(bool status)
{
	inherited::Show					(status);
	if (status)
	{
		InitTalkDialog				();
	} else {
		StopSnd						();
		UITradeWnd->Hide				();
		UIUpgradeWnd->Hide();
		if(m_pActor)
		{
			ToTopicMode					();

			if (m_pActor->IsTalking()) 
				m_pActor->StopTalk();

			m_pActor = NULL;
		}
	}
}

bool  CUITalkWnd::TopicMode			() 
{
	return NULL == m_pCurrentDialog.get();
}

//////////////////////////////////////////////////////////////////////////

void  CUITalkWnd::ToTopicMode		() 
{
	m_pCurrentDialog = DIALOG_SHARED_PTR((CPhraseDialog*)NULL);
}

//////////////////////////////////////////////////////////////////////////

void CUITalkWnd::AskQuestion()
{
	if(m_bNeedToUpdateQuestions) return;//quick dblclick:(
	shared_str					phrase_id;

	//����� ������ ���� ���������
	if(TopicMode())
	{
		if ( (UITalkDialogWnd->m_ClickedQuestionID =="") ||
			(!m_pOurDialogManager->HaveAvailableDialog(UITalkDialogWnd->m_ClickedQuestionID)) ) 
		{

			string128	s;
			xr_sprintf		(s,"ID = [%s] of selected question is out of range of available dialogs ",UITalkDialogWnd->m_ClickedQuestionID);
			VERIFY2(FALSE, s);
		}

		m_pCurrentDialog = m_pOurDialogManager->GetDialogByID( UITalkDialogWnd->m_ClickedQuestionID);
		
		m_pOurDialogManager->InitDialog(m_pOthersDialogManager, m_pCurrentDialog);
		phrase_id = "0";
	}
	else
	{
		phrase_id = UITalkDialogWnd->m_ClickedQuestionID;
	}

	SayPhrase				(phrase_id);
	NeedUpdateQuestions		();
}

//////////////////////////////////////////////////////////////////////////

void CUITalkWnd::SayPhrase(const shared_str& phrase_id)
{

	AddAnswer(m_pCurrentDialog->GetPhraseText(phrase_id), m_pOurInvOwner->Name());
	m_pOurDialogManager->SayPhrase(m_pCurrentDialog, phrase_id);
/*
	//�������� ����� ����������� � ������, ���� �� ���-�� ������
	if(m_pCurrentDialog->GetLastPhraseID() !=  phrase_id)
		AddAnswer(m_pCurrentDialog->GetLastPhraseText(), m_pOthersInvOwner->Name());
*/
	//���� ������ ����������, ������� � ����� ������ ����
	if(m_pCurrentDialog->IsFinished()) ToTopicMode();
}

//////////////////////////////////////////////////////////////////////////

void CUITalkWnd::AddQuestion(const shared_str& text, const shared_str& value, int number)
{
	if (text.size() == 0)
		return;
	UITalkDialogWnd->AddQuestion(*CStringTable().translate(text),value.c_str(), number);
}

//////////////////////////////////////////////////////////////////////////

void CUITalkWnd::AddAnswer(const shared_str& text, LPCSTR SpeakerName)
{
	//��� ������ ����� ������ ������ �� �������
	if(text.size() == 0) return;
	PlaySnd			(text.c_str());

	bool i_am = (0 == xr_strcmp(SpeakerName, m_pOurInvOwner->Name()));
	UITalkDialogWnd->AddAnswer(SpeakerName,*CStringTable().translate(text),i_am);
}

//////////////////////////////////////////////////////////////////////////

void CUITalkWnd::SwitchToTrade()
{
	if (m_pOthersInvOwner->IsUpgradeEnabled(m_pOurInvOwner))
	{
		SwitchToUpgrade();
	}
	else if(m_pOurInvOwner->IsTradeEnabled() && m_pOthersInvOwner->IsTradeEnabled())
	{

		UITalkDialogWnd->Hide		();

		UITradeWnd->InitTrade		(m_pOurInvOwner, m_pOthersInvOwner);
		UITradeWnd->Show				();
		UITradeWnd->StartTrade		();
		UITradeWnd->BringAllToTop	();
		StopSnd						();
	}
}


void CUITalkWnd::SwitchToUpgrade(CInventoryOwner* our, CInventoryOwner* other)
{
	UITalkDialogWnd->Hide();
	UIUpgradeWnd->InitUpgrade(our ? our : m_pOurInvOwner, other ? other : m_pOthersInvOwner);
	UIUpgradeWnd->Show();
	UIUpgradeWnd->BringAllToTop();
	UITradeWnd->StartTrade();
	UITradeWnd->BringAllToTop();
	StopSnd();
}

bool CUITalkWnd::OnKeyboardAction(int dik, EUIMessages keyboard_action)
{

	if (keyboard_action==WINDOW_KEY_PRESSED)
	{
		if(is_binded(kUSE, dik) || is_binded(kQUIT, dik))
		{
			if(!b_disable_break)
			{
				HideDialog();
				return true;
			}
		}
		else if(is_binded(kSPRINT_TOGGLE, dik))
		{
			SwitchToTrade();
		}
	}

	return inherited::OnKeyboardAction(dik,keyboard_action);
}

void CUITalkWnd::PlaySnd(LPCSTR text)
{
	u32 text_len = xr_strlen(text);
	if ( text_len == 0 )
	{
		return;
	}
	
	string_path	fn;
	
	LPCSTR path = "characters_voice\\dialogs\\";
	LPCSTR ext  = ".ogg";
	u32 tsize   = sizeof(fn) - xr_strlen(path) - xr_strlen(ext) - 1;
	if ( text_len > tsize )
	{
		text_len = tsize;
	}

	strncpy_s( fn, sizeof(fn), path, xr_strlen(path) );
	strncat_s( fn, sizeof(fn), text, text_len );
	strncat_s( fn, sizeof(fn), ext,  xr_strlen(ext) );

	//	strconcat( sizeof(fn), fn, "characters_voice\\dialogs\\", text2, ".ogg" );

	StopSnd();
	if ( FS.exist( "$game_sounds$", fn ) )
	{
		VERIFY( m_pActor );
		if ( !m_pActor->OnDialogSoundHandlerStart(m_pOthersInvOwner, fn) )
		{
			CGameObject* pOtherGO = smart_cast<CGameObject*>(m_pOthersInvOwner);
			Fvector P = pOtherGO->Position();
			P.y			+= 1.8f;
			m_sound.create( fn, st_Effect, sg_SourceType );
			m_sound.play_at_pos( 0, P );
		}
	}
}

void CUITalkWnd::StopSnd()
{
	if (m_pActor && m_pActor->OnDialogSoundHandlerStop(m_pOthersInvOwner)) return;

	if(m_sound._feedback()) 
		m_sound.stop	();
}

void CUITalkWnd::AddIconedMessage(LPCSTR text, LPCSTR texture_name, Frect texture_rect, LPCSTR templ_name)
{
	UITalkDialogWnd->AddIconedAnswer(text, texture_name, texture_rect, templ_name);
}

void CUITalkWnd::StopTalk()
{
	HideDialog();
}

void CUITalkWnd::Stop()
{
}