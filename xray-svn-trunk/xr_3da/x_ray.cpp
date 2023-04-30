
// Root Developers:
// Oles - Oles Shishkovtsov
// AlexMX - Alexander Maksimchuk

#include "stdafx.h"
#include "x_ray.h"

#include "igame_level.h"
#include "igame_persistent.h"

#include "xr_input.h"
#include "xr_ioconsole.h"
#include "xr_ioc_cmd.h"

#include "BaseInstanceClasses.h"
#include "GameFont.h"
#include "resource.h"
#include "LightAnimLibrary.h"
#include "../xrcdb/ispatial.h"

ENGINE_API CInifile* pGameIni = NULL;
ENGINE_API CApplication* pApp = NULL;
extern BOOL g_bIntroFinished;
extern HWND logoWindow;

struct _SoundProcessor	: public pureFrame
{
	virtual void			OnFrame	( )
	{
#ifdef MEASURE_MT
		CTimer measure_mt; measure_mt.Start();
#endif


		::Sound->update(Device.vCameraPosition,Device.vCameraDirection,Device.vCameraTop);

		
#ifdef MEASURE_MT
		Device.Statistic->mtSoundTime_ += measure_mt.GetElapsed_sec();
#endif
	}
}SoundProcessor;


CApplication::CApplication()
{
	ll_dwReference = 0;

	max_load_stage = 0;

	// events
	eQuit = Engine.Event.Handler_Attach("KERNEL:quit", this);
	eStart = Engine.Event.Handler_Attach("KERNEL:start", this);
	eStartLoad = Engine.Event.Handler_Attach("KERNEL:load", this);
	eDisconnect = Engine.Event.Handler_Attach("KERNEL:disconnect", this);
	eConsole = Engine.Event.Handler_Attach("KERNEL:console", this);

	// levels
	Level_Current = u32(-1);

	Level_Scan();

	// Font
	pFontSystem = NULL;

	// Register us
	Device.seqFrameBegin.Add(this, REG_PRIORITY_HIGH);
	Device.seqFrame.Add(this, REG_PRIORITY_HIGH + 200);
	Device.seqRender.Add(this, REG_PRIORITY_HIGH);

	// Choose MT or ST sound handler
	if (psDeviceFlags.test(mtSound))
		Device.seqFrameMT.Add(&SoundProcessor);
	else
		Device.seqFrame.Add(&SoundProcessor);

	Console->Show();

	// App Title
//	app_title[ 0 ] = '\0';
	ls_header[0] = '\0';
	ls_tip_number[0] = '\0';
	ls_tip[0] = '\0';
}

CApplication::~CApplication()
{
	Console->Hide();

	// font
	xr_delete(pFontSystem);

	Device.seqFrameMT.Remove(&SoundProcessor);
	Device.seqFrame.Remove(&SoundProcessor);
	Device.seqFrame.Remove(this);
	Device.seqFrameBegin.Remove(this);
	Device.seqRender.Remove(this);


	// events
	Engine.Event.Handler_Detach(eConsole, this);
	Engine.Event.Handler_Detach(eDisconnect, this);
	Engine.Event.Handler_Detach(eStartLoad, this);
	Engine.Event.Handler_Detach(eStart, this);
	Engine.Event.Handler_Detach(eQuit, this);

}

void CApplication::OnEvent(EVENT E, u64 P1, u64 P2)
{
	if (E == eQuit)
	{
		LPCSTR reason = LPSTR(P1);
		Msg(LINE_SPACER);
		Msg("Exiting application. Reason: %s", reason);
		Msg(LINE_SPACER);

		FlushLog();
		PostQuitMessage(0);

		for (u32 i = 0; i < Levels.size(); i++)
		{
			xr_free(Levels[i].folder);
			xr_free(Levels[i].name);
		}
	}
	else if (E == eStart)
	{
		Msg(LINE_SPACER);
		Msg("----------- Loading or Creating Game -----------");

		LPSTR op_server = LPSTR(P1);
		Level_Current = u32(-1);

		R_ASSERT(!g_pGameLevel);
		R_ASSERT(g_pGamePersistent);

		{
			FlushLog();

			Console->Execute("main_menu off");
			Console->Hide();

			//!			this line is commented by Dima
			//!			because I don't see any reason to reset device here
			//!			Device.Reset(false);

			g_pGamePersistent->PreStart(op_server);

			g_pGameLevel = (IGame_Level*)NEW_INSTANCE(CLSID_GAME_LEVEL);

			g_pGamePersistent->Start(op_server);
			g_pGameLevel->Gameloading(op_server);
		}

		FlushLog();
		xr_free(op_server);
	}
	else if (E == eDisconnect)
	{
		ls_header[0] = '\0';
		ls_tip_number[0] = '\0';
		ls_tip[0] = '\0';

		LPCSTR reason = LPSTR(P1);
		Msg(LINE_SPACER);
		Msg("Disconnecting. Reason: %s", reason);
		Msg(LINE_SPACER);

		if (g_pGameLevel)
		{
			Console->Hide();

			g_pGameLevel->net_Stop();

			DEL_INSTANCE(g_pGameLevel);

			Console->Show();

			if ((FALSE == Engine.Event.Peek("KERNEL:quit")) && (FALSE == Engine.Event.Peek("KERNEL:start")))
			{
				Console->Execute("main_menu off");
				Console->Execute("main_menu on");
			}
		}

		FlushLog();

		if (g_pGamePersistent)
			g_pGamePersistent->Disconnect();
	}
	else if (E == eConsole)
	{
		LPSTR command = (LPSTR)P1;

		Console->ExecuteCommand(command, false);

		xr_free(command);
	}
}

void CApplication::OnFrameBegin()
{
	Engine.Event.OnFrame();
}

void CApplication::OnFrame()
{
#ifdef MEASURE_ON_FRAME
	CTimer measure_on_frame; measure_on_frame.Start();
#endif


	g_SpatialSpace->update();
	g_SpatialSpacePhysic->update();

	if (g_pGameLevel)
		g_pGameLevel->SoundEvent_Dispatch();


#ifdef MEASURE_ON_FRAME
	Device.Statistic->onframe_App_ += measure_on_frame.GetElapsed_ms_f();
#endif
}

void CApplication::OnRender()
{
	if (!g_pGamePersistent)
		Device.auxThread_4_Allowed_.Set();
}

void CApplication::InitEngine()
{
	CTimer T; T.Start();

	Engine.Initialize();

	while (!g_bIntroFinished)
		Sleep(100);

	Msg("* Time %f ms \n", T.GetElapsed_sec() * 1000.f);
	T.Start();

	Device.Initialize(); // App window init

	Msg("* Time %f ms \n", T.GetElapsed_sec() * 1000.f);
	T.Start();

	InitInput();

	Msg("* Time %f ms \n", T.GetElapsed_sec() * 1000.f);
	T.Start();

	psDeviceFlags.set(rsR4, TRUE);
	InitConsole();

	Msg("* Time %f ms \n", T.GetElapsed_sec() * 1000.f);
	T.Start();

	Engine.External.Initialize(); // Open other ddls

	Msg("* Time %f ms \n", T.GetElapsed_sec() * 1000.f);

	Console->PostInitialize(); // after dlls are loaded
	Console->Execute("stat_memory");

	FlushLog(false);
}

void CApplication::InitSettings()
{
	Msg("# Initing Settings...");
	CTimer T; T.Start();

	string_path					fname; 
	FS.update_path				(fname,"$game_config$", "system.ltx");

#ifdef DEBUG
	Msg							("Updated path to system.ltx is %s", fname);
#endif

	pSettings				= xr_new <CInifile>(fname, TRUE);

	CHECK_OR_EXIT				(pSettings->section_count() != 0, make_string("Cannot find file %s.\nReinstalling application may fix this problem.", fname));

	FS.update_path(fname, "$game_config$", "game.ltx");

	pGameIni				= xr_new <CInifile>(fname,TRUE);

	CHECK_OR_EXIT				(pGameIni->section_count() != 0, make_string("Cannot find file %s.\nReinstalling application may fix this problem.", fname));
	
	Msg("* Time %f ms \n", T.GetElapsed_sec() * 1000.f);
	FlushLog(false);
}

void CApplication::InitConsole()
{
	Msg("# Initing Console...");

	Console	= xr_new <CConsole>();
	Console->Initialize();
}

void CApplication::InitInput()
{
	Msg("# Initing Imput...");

	BOOL bCaptureInput = !strstr(Core.Params, "-i");

	pInput = xr_new<CInput>(bCaptureInput);
}

void InitSound1()
{
	CSound_manager_interface::_create(0);
}

void InitSound2()
{
	CSound_manager_interface::_create(1);
}

void DestroyConsole()
{
	Console->Execute("cfg_save");
	Console->Destroy();

	xr_delete(Console);
}

void DestroySound()
{
	CSound_manager_interface::_destroy();
}

void DestroySettings()
{
	CInifile** s				= (CInifile**)(&pSettings);
	xr_delete					(*s);

	xr_delete					(pGameIni);
}

void DestroyEngine	()
{
	Device.Destroy();

	try
	{
		FlushLog(); // This should prevent empty log file in some cases
	}
	catch (...)
	{
		MessageBox(NULL, "Could not perform log saving after destroying Render Device", "Code incorrectness. Need Debug", MB_OK | MB_ICONWARNING | MB_TASKMODAL);
	}

	try
	{
		Engine.Destroy();
	}
	catch (...)
	{
		MessageBox(NULL, "Error while destroying Engine", "Error!", MB_OK | MB_ICONWARNING | MB_TASKMODAL);
	}
}

void DestroyInput()
{
	xr_delete(pInput);
}

void ExecUserScript()
{
	Console->AssignCFGFile();
	Console->Execute("unbindall");
	Console->ExecuteScript(Console->ConfigFile);
}

void CApplication::Startup()
{
	CTimer T; T.Start();

	FlushLog(false);

	Msg("# Initing Sound...");

	InitSound1();
	ExecUserScript();
	InitSound2();

	Msg("* Time %f ms \n", T.GetElapsed_sec() * 1000.f);

	if (Engine.External.XrGameModuleExist())
	{
		// ...command line for auto start
		{
			LPCSTR pStartup = strstr(Core.Params, "-start ");
			if (pStartup)
				Console->Execute(pStartup + 1);
		}
		{
			LPCSTR pStartup = strstr(Core.Params, "-load ");
			if (pStartup)
				Console->Execute(pStartup + 1);
		}
	}

	T.Start();
	Msg("# Creating Device Instance...");

	// Initialize APP
	ShowWindow(Device.m_hWnd , SW_SHOWNORMAL);
	Device.Create();
	Msg("* Time %f ms \n", T.GetElapsed_sec() * 1000.f);
	
	T.Start();
	Msg("# Creating Light Anim Lib...");

	LALib.OnCreate();

	Msg("* Time %f ms \n", T.GetElapsed_sec() * 1000.f);

	T.Start();
	Msg("# Creating Application Class Instance...");

	pApp						= xr_new <CApplication>();

	Msg("* Time %f ms \n", T.GetElapsed_sec() * 1000.f);

	if (Engine.External.XrGameModuleExist())
	{
		T.Start();
		Msg("# Creating Game Persistant Instance...");

		g_pGamePersistent = (IGame_Persistent*)NEW_INSTANCE(CLSID_GAME_PERSISTANT);

		Msg("* Time %f ms \n", T.GetElapsed_sec() * 1000.f);

		T.Start();
		Msg("# Creating Spacial Space Instances...");

		g_SpatialSpace = xr_new <ISpatial_DB>();
		g_SpatialSpacePhysic = xr_new <ISpatial_DB>();

		Msg("* Time %f ms \n", T.GetElapsed_sec() * 1000.f);
	}

	// Destroy LOGO
	DestroyWindow				(logoWindow);
	logoWindow					= NULL;

	Memory.mem_usage();
}

void CApplication::Process()
{
	// Main cycle
	Device.Run();	// Do the Game!
}

void CApplication::EndUp()
{
	// Destroy APP
	xr_delete(g_SpatialSpacePhysic);
	xr_delete(g_SpatialSpace);

	if (g_pGamePersistent)
		DEL_INSTANCE(g_pGamePersistent);

	xr_delete(pApp);

	Engine.Event.Dump();

	DestroyInput();

	DestroySettings();

	LALib.OnDestroy();

	DestroyConsole();

	DestroySound();
	DestroyEngine();
}

void CApplication::Level_Append(LPCSTR folder)
{
	string_path	N1,N2,N3,N4;

	strconcat(sizeof(N1), N1, folder, "level");
	strconcat(sizeof(N2), N2, folder, "level.ltx");
	strconcat(sizeof(N3), N3, folder, "level.geom");
	strconcat(sizeof(N4), N4, folder, "level.cform");

	if (
		FS.exist("$game_levels$", N1) &&
		FS.exist("$game_levels$", N2) &&
		FS.exist("$game_levels$", N3) &&
		FS.exist("$game_levels$", N4)
		)
	{
		sLevelInfo LI;

		LI.folder = xr_strdup(folder);
		LI.name = 0;

		Levels.push_back(LI);
	}
}

void CApplication::Level_Scan()
{
	xr_vector<char*>* folder = FS.file_list_open("$game_levels$", FS_ListFolders | FS_RootOnly);

	R_ASSERT(folder && folder->size());

	for (u32 i = 0; i < folder->size(); i++)
		Level_Append((*folder)[i]);

	FS.file_list_close(folder);

#ifdef DEBUG
	folder = FS.file_list_open("$game_levels$", "$debug$\\", FS_ListFolders | FS_RootOnly);
	if (folder)
	{
		string_path	tmp_path;

		for (u32 i = 0; i < folder->size(); i++)
		{
			strconcat(sizeof(tmp_path), tmp_path, "$debug$\\", (*folder)[i]);
			Level_Append(tmp_path);
		}

		FS.file_list_close(folder);
	}
#endif
}

void CApplication::Level_Set(u32 L)
{
	if (L>=Levels.size())
		return;

	Level_Current = L;

	FS.get_path	("$level$")->_set(Levels[L].folder);

	string_path temp;
	string_path temp2;

	strconcat(sizeof(temp),temp,"intro\\intro_", Levels[L].folder);
	temp[xr_strlen(temp)-1] = 0;

	if (FS.exist(temp2, "$game_textures$", temp, ".dds"))
		m_pRender->setLevelLogo(temp);
	else
		m_pRender->setLevelLogo("intro\\intro_no_start_picture");
}

int CApplication::Level_ID(LPCSTR name)
{
	int result = -1;

	char buffer	[256];

	strconcat(sizeof(buffer), buffer, name, "\\");

	for (u32 I=0; I<Levels.size(); I++)
	{
		if (0 == stricmp(buffer, Levels[I].folder))	
			result = int(I);
	}

	return result;
}

