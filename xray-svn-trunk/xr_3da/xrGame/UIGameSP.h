#pragma once
#include "uigamecustom.h"
#include "ui/UIDialogWnd.h"
#include "game_graph_space.h"

class CUITradeWnd;			
class CInventory;

class game_cl_GameState;
class CUITalkWnd;
class CChangeLevelWnd;
class CUIMessageBox;
class CInventoryBox;
class CInventoryOwner;

class CUIGameSP : public CUIGameCustom
{
private:
	typedef CUIGameCustom inherited;
public:
	CUIGameSP									();
	virtual				~CUIGameSP				();

	virtual bool		IR_UIOnKeyboardPress	(int dik);
	virtual bool		IR_UIOnKeyboardRelease	(int dik);

	void				StartTalk				(bool disable_break);
	void				StartStashUI			(CInventoryOwner* pActorInv, CInventoryOwner* pOtherOwner);
	void				StartStashUI			(CInventoryOwner* pActorInv, CInventoryBox* pBox);
	void				OpenSafe				();
	void				StartUpgrade			(CInventoryOwner* pActorInv, CInventoryOwner* pMech);
	void				ChangeLevel				(GameGraph::_GRAPH_ID game_vert_id, u32 level_vert_id, Fvector pos, Fvector ang, Fvector pos2, Fvector ang2, bool b, const shared_str& message, bool b_allow_change_level);

	virtual void		HideShownDialogs		();
	virtual void		HideInputReceavingDialog();
	virtual void		ReInitShownUI			();
	virtual void		ReinitDialogs			();

	virtual void		Render					();

	void				EnableSkills		(bool val);
	void				EnableDownloads		(bool val);

	CUITalkWnd*			TalkMenu;
	CChangeLevelWnd*	UIChangeLevelWnd;
	CInventoryBox*		StoredInvBox;

	virtual	bool		EscapePressed();
};


class CChangeLevelWnd :public CUIDialogWnd
{
	CUIMessageBox*			m_messageBox;
	typedef CUIDialogWnd	inherited;
	void					OnCancel			();
	void					OnOk				();
public:
	GameGraph::_GRAPH_ID	m_game_vertex_id;
	u32						m_level_vertex_id;
	Fvector					m_position;
	Fvector					m_angles;
	Fvector					m_position_cancel;
	Fvector					m_angles_cancel;
	bool					m_b_position_cancel;
	bool					m_b_allow_change_level;
	shared_str				m_message_str;

						CChangeLevelWnd				();
	virtual				~CChangeLevelWnd			()									{};
	virtual void		SendMessage					(CUIWindow *pWnd, s16 msg, void *pData);
	virtual bool		WorkInPause					()const {return true;}
	virtual void		ShowDialog						(bool bDoHideIndicators);
	virtual void		HideDialog						();
	virtual bool		OnKeyboardAction					(int dik, EUIMessages keyboard_action);
};