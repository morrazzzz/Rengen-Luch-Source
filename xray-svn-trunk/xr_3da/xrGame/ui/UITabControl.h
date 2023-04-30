#pragma once

#include "uiwindow.h"
#include "../script_export_space.h"
#include "UIOptionsItem.h"

class CUITabButton;
class CUIButton;

DEF_VECTOR (TABS_VECTOR, CUITabButton*)

class CUITabControl: public CUIWindow, public CUIOptionsItem
{
	typedef				CUIWindow inherited;
public:
						CUITabControl				();
	virtual				~CUITabControl				();

	// options item
	virtual void		SetCurrentOptValue				();
	virtual void		SaveOptValue					();
	virtual bool		IsChangedOptValue					() const;
	virtual void		SaveBackUpOptValue			();
	virtual void		UndoOptValue				();

	virtual bool		OnKeyboardAction					(int dik, EUIMessages keyboard_action);
	virtual void		OnTabChange					(int iCur, int iPrev);
	virtual void		OnStaticFocusReceive		(CUIWindow* pWnd);
	virtual void		OnStaticFocusLost			(CUIWindow* pWnd);

	// ���������� ������-�������� � ������ �������� ��������
	bool				AddItem						(LPCSTR pItemName, LPCSTR pTexName, float x, float y, float width, float height);
	bool				AddItem						(CUITabButton *pButton);

	void				RemoveItem					(const u32 Index);
	void				RemoveAll					();

	virtual void		SendMessage					(CUIWindow *pWnd, s16 msg, void *pData);

			int			GetActiveIndex				()	const								{ return m_iPushedIndex; }
			int			GetPrevActiveIndex			()	const								{ return m_iPrevPushedIndex; }
			void		SetNewActiveTab				(const int iNewTab);	
	const	int			GetTabsCount				() const						{ return m_TabsArr.size(); }
	
	// ����� ������������� ������������� (���/����)
	IC bool				GetAcceleratorsMode			() const						{ return m_bAcceleratorsEnable; }
	void				SetAcceleratorsMode			(bool bEnable)					{ m_bAcceleratorsEnable = bEnable; }


	TABS_VECTOR *		GetButtonsVector			()								{ return &m_TabsArr; }
	CUIButton*			GetButtonByIndex			(int i);
	const shared_str	GetCommandName				(int i);
	CUIButton*			GetButtonByCommand			(const shared_str& n);
			void		ResetTab					();
protected:
	// ������ ������ - �������������� ��������
	TABS_VECTOR			m_TabsArr;

	// ������� ������� ������. -1 - �� ����, 0 - ������, 1 - ������, � �.�.
	int					m_iPushedIndex;
	int					m_iPrevPushedIndex;

	// ���� ���������� ���������
	u32					m_cGlobalTextColor;
	u32					m_cGlobalButtonColor;

	// ���� ������� �� �������� ��������
	u32					m_cActiveTextColor;
	u32					m_cActiveButtonColor;

	// ���������/��������� ������������ ������������
	bool				m_bAcceleratorsEnable;
	int					m_opt_backup_value;
	DECLARE_SCRIPT_REGISTER_FUNCTION
};
