#pragma once

#include "UIProgressBar.h"
#include "UIGameLog.h"

#include "../alife_space.h"

#include "UICarPanel.h"
#include "../UIActorStateIcons.h"
#include "UIMotionIcon.h"
#include "../hudsound.h"
#include "UICellItem.h"
#include "../inventory_space.h"

class					CUIPdaMsgListItem;
class					CLAItem;
class					CUIZoneMap;
class					CUIArtefactPanel;
class					CUIScrollView;
struct					GAME_NEWS_DATA;
class					CActor;
class					CWeapon;
class					CMissile;
class					CInventoryItem;

class CUIMainIngameWnd: public CUIWindow  
{
public:
	CUIMainIngameWnd();
	virtual ~CUIMainIngameWnd();

	virtual void Init();
	virtual void Draw();
	virtual void Update();

	bool OnKeyboardPress(int dik);

protected:
	
	CUIStatic			UIStaticHealth;
	CUITextWnd			UIStaticQuickHelp;
	CUIProgressBar		UIHealthBar;
	CUIProgressBar		UIArmorBar;
	CUICarPanel			UICarPanel;
	CUIMotionIcon		UIMotionIcon;	
	CUIZoneMap*			UIZoneMap;

	EDifficultyShowCondition showMiniMapCond_;

	CUIStatic			UIStaticTime;
	CUIStatic			UIStaticDate;
	CUIStatic			UIStaticTorch;
	CUIProgressBar		UIFlashlightBar;
	CUIStatic			UIStaticTurret;
	CUIProgressBar		UITurretBar;
	CUIActorStateIcons	UIActorStateIcons;//������ ������, ��������, ���������� ������ � ��

	//������, ������������ ���������� �������� PDA
	CUIStatic			UIPdaOnline;
	
	//����������� ������
	CUIStatic			UIWeaponBack;
	CUIStatic			UIWeaponSignAmmo;
	CUIStatic			UIWeaponIcon;
	Frect				UIWeaponIcon_rect;
	CUIStatic			UIWeaponFiremode;

	CUIStatic			UIInvincibleIcon;

	CUIScrollView*		m_UIIcons;

	// use progress circle
	float				useCircleRectX;
	float				useCircleRectY;
	int					useCircleFrames;

	u8					useCircleTextureFrame;
	CUIStatic			useCircle;

public:
	CUIStatic*			GetPDAOnline() { return &UIPdaOnline; };
	
	// ����� �������������� ��������������� ������� 
	enum EWarningIcons
	{
		ewiAll				= 0,
		ewiWeaponJammed,
		ewiRadiation,
		ewiWound,
		ewiStarvation,
		ewiThirst,
		ewiPsyHealth,
		ewiInvincible,
		ewiArtefact,
	};

	// ������ ���� ��������������� ������
	void				SetWarningIconColor				(EWarningIcons icon, const u32 cl);
	void				TurnOffWarningIcon				(EWarningIcons icon);

	// ������ ��������� ����� �����������, ����������� �� system.ltx
	typedef				xr_map<EWarningIcons, xr_vector<float> >	Thresholds;
	typedef				Thresholds::iterator						Thresholds_it;
	Thresholds			m_Thresholds;

	// ���� ������������ ��������� �������� ������
	enum EFlashingIcons
	{
		efiPdaTask	= 0,
		efiMail,
		efiEncyclopedia
	};
	
	void				SetFlashIconState_				(EFlashingIcons type, bool enable);

	void				AnimateContacts					(bool b_snd);
	HUD_SOUND_ITEM			m_contactSnd;

	void				ReceiveNews						(GAME_NEWS_DATA* news);
	
protected:
	void				SetWarningIconColor				(CUIStatic* s, const u32 cl);
	void				InitFlashingIcons				(CUIXml* node);
	void				DestroyFlashingIcons			();
	void				UpdateFlashingIcons				();
	void				UpdateActiveItemInfo			();
	void				HandleBolt						();

	void				SetAmmoIcon						(const shared_str& se�t_name);

	// first - ������, second - ��������
	DEF_MAP				(FlashingIcons, EFlashingIcons, CUIStatic*);
	FlashingIcons		m_FlashingIcons;

	//��� �������� ��������� ������ � ������
	CActor*				m_pActor;	
	CWeapon*			m_pWeapon;
	CMissile*			m_pGrenade;
	CInventoryItem*		m_pItem;

	// ����������� ��������� ��� ��������� ������� �� ������
	void				RenderQuickInfos();

public:
	CUICarPanel&		CarPanel							(){return UICarPanel;};
	CUIMotionIcon&		MotionIcon							(){return UIMotionIcon;}
	void				OnConnected							();
	void				reset_ui							();
protected:
	float				fuzzyShowInfo_;
	CInventoryItem*		m_pPickUpItem;

	II_BriefInfo		m_item_info;

	CUICellItem*		uiPickUpItemIconNew_;
public:
	void				SetPickUpItem	(CInventoryItem* PickUpItem);
#ifdef DRENDER
	void				draw_adjust_mode					();
#endif //DRENDER
};
