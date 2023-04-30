#pragma once

class ENGINE_API CGameFont;

#include "../Include/xrRender/FactoryPtr.h"
#include "../Include/xrRender/ApplicationRender.h"

#define QUICK_LOAD_PHASES 4
#define LEVEL_LOAD_PHASES 17
#define LEVEL_LOAD_AND_PREFETCH_PHASES 18

class ENGINE_API CApplication	:
	public pureFrameBegin,
	public pureFrame,
	public pureRender,
	public IEventReceiver
{
	friend class dxApplicationRender;

	// levels
	struct					sLevelInfo
	{
		char*				folder;
		char*				name;
	};
public:
	string2048				ls_header;
	string2048				ls_tip_number;
	string2048				ls_tip;
private:
	FactoryPtr<IApplicationRender>	m_pRender;

	int						max_load_stage;

	int						load_stage;

	u32						ll_dwReference;
private:
	EVENT					eQuit;
	EVENT					eStart;
	EVENT					eStartLoad;
	EVENT					eDisconnect;
	EVENT					eConsole;

	void					Level_Append		(LPCSTR lname);
public:
	CGameFont*				pFontSystem;

	static bool				isDeveloperMode;

	// Levels
	xr_vector<sLevelInfo>	Levels;
	u32						Level_Current;
	void					Level_Scan			();
	int						Level_ID			(LPCSTR name);
	void					Level_Set			(u32 ID);

	// Initialization
	static void				InitEngine			();
	static void				InitSettings		();
	static void				InitConsole			();
	static void				InitInput			();
	static void				Startup				();
	static void				Process				();
	static void				EndUp				();

	// Loading
	void					LoadPhaseBegin		(u8 load_phases);
	void					LoadPhaseEnd		();
	void					LoadTitleInt		(LPCSTR str1, LPCSTR str2, LPCSTR str3, LPCSTR log_msg);
	void					ClearTitle			();
	void					LoadStage			(LPCSTR log_msg);
	void					LoadDraw			();

	virtual	void			OnEvent				(EVENT E, u64 P1, u64 P2);

	// Other
							CApplication		();
	virtual					~CApplication		();

	virtual void			OnFrameBegin		();
	virtual void			OnFrame				();
	virtual void			OnRender			();

			void			load_draw_internal	();
			void			destroy_loading_shaders();
};

extern ENGINE_API CApplication* pApp;

