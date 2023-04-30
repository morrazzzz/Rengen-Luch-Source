#include "pch_script.h"
#include "uigamesp.h"
#include "actor.h"
#include "level.h"
#include "../xr_input.h"

#include "game_cl_base.h"
#include "ui/UIPdaAux.h"
#include "xr_level_controller.h"
#include "object_broker.h"
#include "GameTaskManager.h"
#include "GameTask.h"
#include "inventory.h"
#include "ui/UITradeWnd.h"
#include "ui/UITalkWnd.h"
#include "ui/UIMessageBox.h"
#include "ui/UIInventoryWnd.h"
#include "ui/UIPdaWnd.h"
#include "ui/UIStashWnd.h"
#include "ui/UIMainIngameWnd.h"
#include "ui/UIWeaponSelectWnd.h"

CUIGameSP::CUIGameSP()
{
	StoredInvBox	= NULL;

	TalkMenu		= xr_new <CUITalkWnd>			();
	UIChangeLevelWnd= xr_new <CChangeLevelWnd>		();
}

CUIGameSP::~CUIGameSP() 
{	
	delete_data(TalkMenu);
	delete_data(UIChangeLevelWnd);
}

void CUIGameSP::HideShownDialogs()
{
	if (m_InventoryMenu && m_InventoryMenu->IsShown())
		m_InventoryMenu->HideDialog();

	if (m_PdaMenu && m_PdaMenu->IsShown())
		m_PdaMenu->HideDialog();

	if (TalkMenu && TalkMenu->IsShown())
		TalkMenu->HideDialog();

	if (m_UIStashWnd && m_UIStashWnd->IsShown())
		m_UIStashWnd->HideDialog();

	if (m_WeatherEditor && m_WeatherEditor->IsShown())
		m_WeatherEditor->HideDialog();

	if (m_WeaponSelect && m_WeaponSelect->IsShown())
		m_WeaponSelect->HideDialog();
}

void CUIGameSP::HideInputReceavingDialog()
{
	CUIDialogWnd* mir = TopInputReceiver();

	if (mir)
		mir->HideDialog();
}

#ifdef DEBUG
	void hud_adjust_mode_keyb(int dik);
	void hud_draw_adjust_mode();
#endif

void hud_draw_adjust_mode();

void CUIGameSP::ReInitShownUI() 
{ 
	if (m_InventoryMenu->IsShown()) 
		m_InventoryMenu->ReinitInventoryWnd();
	else if (m_UIStashWnd->IsShown())
		m_UIStashWnd->UpdateLists_delayed();
	
};

bool CUIGameSP::IR_UIOnKeyboardPress(int dik) 
{
	if(inherited::IR_UIOnKeyboardPress(dik)) return true;
	if( Device.Paused()		) return false;
	
#ifdef DEBUG
	hud_adjust_mode_keyb	(dik);
#endif

	CInventoryOwner* pInvOwner  = smart_cast<CInventoryOwner*>( Level().CurrentEntity() );
	if ( !pInvOwner )			return false;
	CEntityAlive* EA			= smart_cast<CEntityAlive*>(Level().CurrentEntity());
	if (!EA || !EA->g_Alive() )	return false;

	CActor *pActor = smart_cast<CActor*>(pInvOwner);
	if( !pActor ) 
		return false;

	if( !pActor->g_Alive() )	
		return false;

	if(UIMainIngameWnd->OnKeyboardPress(dik))
		return true;

	switch ( get_binded_action(dik) )
	{
	case kINVENTORY: 
		if ((!TopInputReceiver() || TopInputReceiver() == m_InventoryMenu) && !pActor->inventory().IsHandsOnly() && !pActor->inventory_disabled())
		{
			if (!m_InventoryMenu->IsShown())
				m_InventoryMenu->ShowDialog(true);
			else
				m_InventoryMenu->HideDialog();
			break;
		}

	case kWEAPONSELECT:
		if ((!TopInputReceiver() || TopInputReceiver() == m_WeaponSelect))
		{
			if (!m_WeaponSelect->IsShown())
				m_WeaponSelect->ShowDialog(false);
			else
				m_WeaponSelect->HideDialog();
			break;
		}

	case kACTIVE_JOBS:
		if( !TopInputReceiver() || TopInputReceiver()==m_PdaMenu)
		{
			if (!m_PdaMenu->IsShown())
				m_PdaMenu->ShowDialog		(true, eptQuests);
			else
				m_PdaMenu->HideDialog		();
		}break;

	case kMAP:
		if( !TopInputReceiver() || TopInputReceiver()==m_PdaMenu)
		{
			if (!m_PdaMenu->IsShown())
				m_PdaMenu->ShowDialog		(true, eptMap);
			else
				m_PdaMenu->HideDialog		();
		}break;

	case kCONTACTS:
		if( !TopInputReceiver() || TopInputReceiver()==m_PdaMenu)
		{
			if (!m_PdaMenu->IsShown())
				m_PdaMenu->ShowDialog		(true, eptContacts);
			else
				m_PdaMenu->HideDialog		();
			break;
		}break;

	case kTASK_INFO:
		{
			SDrawStaticStruct* ss	= AddCustomStatic("main_task", true);

			CGameTask* task = Level().GameTaskManager().ActiveTask();
			SGameTaskObjective* objective = Level().GameTaskManager().ActiveObjective();

			if (!task)
				ss->m_static->TextItemControl()->SetTextST	("st_no_active_task");
			else
				ss->m_static->TextItemControl()->SetTextST(task->m_Title.c_str());

			if (objective)
			{
				SDrawStaticStruct* sm2 = AddCustomStatic("secondary_task", true);
				sm2->m_static->TextItemControl()->SetTextST(objective->description.c_str());
			}

		}break;
	}

	return false;
}

bool CUIGameSP::EscapePressed()
{
	if (CurrentGameUI()->m_InventoryMenu->IsShown())
	{
		CurrentGameUI()->m_InventoryMenu->HideDialog();

		return true;
	}

	if (CurrentGameUI()->m_WeaponSelect->IsShown())
	{
		CurrentGameUI()->m_WeaponSelect->HideDialog();

		return true;
	}

	CurrentGameUI()->TopInputReceiver()->HideDialog();

	return false;
}

void CUIGameSP::Render()
{
	inherited::Render();
	hud_draw_adjust_mode();
}

bool CUIGameSP::IR_UIOnKeyboardRelease(int dik) 
{
	if(inherited::IR_UIOnKeyboardRelease(dik)) return true;

	if (is_binded(kTASK_INFO, dik))
	{
		RemoveCustomStatic("main_task");
		RemoveCustomStatic("secondary_task");
	}

	if (is_binded(kWEAPONSELECT, dik))
	{
		if ((!TopInputReceiver() || TopInputReceiver() == m_WeaponSelect))
		{
			if (m_WeaponSelect->IsShown())
				m_WeaponSelect->HideDialog();
		}
	}

	return false;
}

void CUIGameSP::StartTalk(bool disable_break)
{
	RemoveCustomStatic		("main_task");

	TalkMenu->b_disable_break = disable_break;
	TalkMenu->ShowDialog		(true);
}

void CUIGameSP::StartStashUI(CInventoryOwner* pActorInv, CInventoryOwner* pOtherOwner) //“руп, богажник ...
{
	if( TopInputReceiver() )		return;

	m_UIStashWnd->InitCustomInventory(pActorInv, pOtherOwner);
	m_UIStashWnd->ShowDialog(true);
}

void CUIGameSP::StartStashUI(CInventoryOwner* pActorInv, CInventoryBox* pBox) //ящик, —эйф
{
	if( TopInputReceiver() )		return;
	
	m_UIStashWnd->InitInventoryBox(pActorInv, pBox);
	m_UIStashWnd->ShowDialog(true);
}
//Ќужно проверить что будет, если гг убьют
void CUIGameSP::OpenSafe()
{
	if (TopInputReceiver())		return;

	if (StoredInvBox){
		m_UIStashWnd->InitInventoryBox(Actor(), StoredInvBox);
		m_UIStashWnd->ShowDialog(true);
	}

}

void  CUIGameSP::StartUpgrade(CInventoryOwner* pActorInv, CInventoryOwner* pMech)
{
	TalkMenu->SwitchToUpgrade(pActorInv, pMech);
}

extern ENGINE_API BOOL bShowPauseString;
void CUIGameSP::ChangeLevel(	GameGraph::_GRAPH_ID game_vert_id,
								u32 level_vert_id, 
								Fvector pos, 
								Fvector ang, 
								Fvector pos2, 
								Fvector ang2, 
								bool b_use_position_cancel,
								const shared_str& message_str,
								bool b_allow_change_level)
{
	if(TopInputReceiver()!=UIChangeLevelWnd)
	{
		UIChangeLevelWnd->m_game_vertex_id		= game_vert_id;
		UIChangeLevelWnd->m_level_vertex_id		= level_vert_id;
		UIChangeLevelWnd->m_position			= pos;
		UIChangeLevelWnd->m_angles				= ang;
		UIChangeLevelWnd->m_position_cancel		= pos2;
		UIChangeLevelWnd->m_angles_cancel		= ang2;
		UIChangeLevelWnd->m_b_position_cancel	= b_use_position_cancel;
		UIChangeLevelWnd->m_b_allow_change_level= b_allow_change_level;
		UIChangeLevelWnd->m_message_str			= message_str;

		UIChangeLevelWnd->ShowDialog		(true);
	}
}

void CUIGameSP::EnableSkills(bool val)
{
	m_PdaMenu->EnableSkills(val);
}

void CUIGameSP::EnableDownloads(bool val)
{
	m_PdaMenu->EnableDownloads(val);
}

void CUIGameSP::ReinitDialogs()
{
	delete_data(m_InventoryMenu);
	m_InventoryMenu		= xr_new <CUIInventoryWnd>();
	
	delete_data(TalkMenu);
	TalkMenu		= xr_new <CUITalkWnd>();
}

CChangeLevelWnd::CChangeLevelWnd		()
{
	m_messageBox			= xr_new <CUIMessageBox>();	
	m_messageBox->SetAutoDelete(true);
	AttachChild				(m_messageBox);
}

void CChangeLevelWnd::SendMessage(CUIWindow *pWnd, s16 msg, void *pData)
{
	if(pWnd==m_messageBox){
		if(msg==MESSAGE_BOX_YES_CLICKED){
			OnOk									();
		}else
		if(msg==MESSAGE_BOX_NO_CLICKED || msg==MESSAGE_BOX_OK_CLICKED)
		{
			OnCancel								();
		}
	}else
		inherited::SendMessage(pWnd, msg, pData);
}

void CChangeLevelWnd::OnOk()
{
	HideDialog								();
	NET_Packet								p;
	p.w_begin								(M_CHANGE_LEVEL);
	p.w										(&m_game_vertex_id,sizeof(m_game_vertex_id));
	p.w										(&m_level_vertex_id,sizeof(m_level_vertex_id));
	p.w_vec3								(m_position);
	p.w_vec3								(m_angles);

	Level().Send							(p,net_flags(TRUE));
}

void CChangeLevelWnd::OnCancel()
{
	HideDialog();
	if(m_b_position_cancel)
		Actor()->MoveActor(m_position_cancel, m_angles_cancel);
}

bool CChangeLevelWnd::OnKeyboardAction(int dik, EUIMessages keyboard_action)
{
	if(keyboard_action==WINDOW_KEY_PRESSED)
	{
		if(is_binded(kQUIT, dik) )
			OnCancel		();
		return true;
	}
	return inherited::OnKeyboardAction(dik, keyboard_action);
}

#include "ai_space.h"
#include "script_engine.h"

bool g_block_pause	= false;
void CChangeLevelWnd::ShowDialog(bool bDoHideIndicators)
{
	luabind::functor<bool> lua_bool;
	bool call_result = false;

	R_ASSERT2(ai().script_engine().functor("level_weathers.is_blowout_active", lua_bool), "Can't find level_weathers.set_weather_manualy");

	if (lua_bool.is_valid()){
		call_result = lua_bool();
	}

	if (call_result)
	{ 
		m_messageBox->InitMessageBox("message_box_change_level_blowout");	//if blowout - show rejection message 
	}
	else
	{
		m_messageBox->InitMessageBox(m_b_allow_change_level ? "message_box_change_level" : "message_box_change_level_disabled");	//else ask if player wants to move to other level
	}

	SetWndPos				(m_messageBox->GetWndPos());
	m_messageBox->SetWndPos	(Fvector2().set(0.0f,0.0f));
	SetWndSize				(m_messageBox->GetWndSize());

	m_messageBox->SetText	(m_message_str.c_str());
	

	g_block_pause							= true;
	Device.Pause							(TRUE, TRUE, TRUE, "CChangeLevelWnd_show");
	bShowPauseString						= FALSE;

	inherited::ShowDialog(bDoHideIndicators);
}

void CChangeLevelWnd::HideDialog()
{
	g_block_pause							= false;
	Device.Pause							(FALSE, TRUE, TRUE, "CChangeLevelWnd_hide");

	inherited::HideDialog();
}

