
#pragma once

#include "../IGame_Persistent.h"
class CMainMenu;
class CUICursor;
class CParticlesObject;
class CUISequencer;
class ui_core;

class CGamePersistent: 
	public IGame_Persistent, 
	public IEventReceiver
{
	typedef IGame_Persistent			inherited;

	bool				m_bPickableDOF;
	CUISequencer*		m_intro;
	EVENT				eQuickLoad;
	Fvector				m_dof		[4];	// 0-dest 1-current 2-from 3-original

	fastdelegate::FastDelegate0<> m_intro_event;

	void xr_stdcall		start_logo_intro		();
	void xr_stdcall		update_logo_intro		();

	void xr_stdcall		game_loaded				();
	void xr_stdcall		update_game_loaded		();

	void xr_stdcall		start_game_intro		();
	void xr_stdcall		update_game_intro		();

	void				WeathersUpdate			();
	void				UpdateDof				();

public:
	ui_core*			m_pUI_core;

						CGamePersistent			();
	virtual				~CGamePersistent		();

	virtual void		Start					(LPCSTR op);
	virtual void		Disconnect				();

	virtual	void		OnAppActivate			();
	virtual void		OnAppDeactivate			();

	virtual void		OnAppStart				();
	virtual void		OnAppEnd				();
	virtual	void		OnGameStart				();
	virtual void		OnGameEnd				();
	virtual void		OnFrame					();
	virtual void		OnEvent					(EVENT E, u64 P1, u64 P2);

	virtual void		RegisterModel			(IRenderVisual* V);
	virtual	float		MtlTransparent			(u32 mtl_idx);

	virtual bool		OnRenderPPUI_query		();
	virtual void		OnRenderPPUI_main		();
	virtual void		OnRenderPPUI_PP			();
	virtual	void		LoadTitle				(LPCSTR ui_msg, LPCSTR log_msg, bool change_tip = false, shared_str map_name = "");

	virtual bool		CanBePaused				();

			void		SetPickableEffectorDOF	(bool bSet);
			void		SetEffectorDOF			(const Fvector& needed_dof);
			void		RestoreEffectorDOF		();

	virtual void		GetCurrentDof			(Fvector3& dof);
	virtual void		SetBaseDof				(const Fvector3& dof);

	// Runs scripted weather cycle selection
			void		SelectWeatherCycle() override;
	// Checks if weather script has been loaded to prevent undefined behavior
			bool		IsWeatherScriptLoaded() override;

	//
			void		SetActiveCamera		();
			void		TutorialAndIntroHandling();

	xr_vector <fastdelegate::FastDelegate0<>> afterGameLoadedStuff_;
	virtual void		OnSectorChanged			(int sector);
	virtual void		OnAssetsChanged			();
};

IC CGamePersistent&		GamePersistent()		{return *((CGamePersistent*) g_pGamePersistent);}

extern	bool m_bCamReady;