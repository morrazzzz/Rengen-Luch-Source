// xrLC.cpp : Defines the entry point for the application.

#include "stdafx.h"
#include "math.h"
#include "build.h"

//#pragma comment(linker,"/STACK:0x800000,0x400000")
//#pragma comment(linker,"/HEAP:0x70000000,0x10000000")

#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"d3dx9.lib")
#pragma comment(lib,"IMAGEHLP.LIB")
#pragma comment(lib,"winmm.LIB")
#pragma comment(lib,"xrCDB.lib")
#pragma comment(lib,"FreeImage.lib")
#pragma comment(lib,"xrCore.lib")

#define PROTECTED_BUILD

#ifdef PROTECTED_BUILD
#	define TRIVIAL_ENCRYPTOR_ENCODER
#	define TRIVIAL_ENCRYPTOR_DECODER
#	include "../xrCore/trivial_encryptor.h"
#	undef TRIVIAL_ENCRYPTOR_ENCODER
#	undef TRIVIAL_ENCRYPTOR_DECODER
#endif // PROTECTED_BUILD

CBuild*	pBuild = NULL;
u32		version = 0;

#define FSLTX "fs.ltx"

extern void logThread(void *dummy);
extern volatile BOOL bClose;
extern bool strangeFaceLogging;
extern bool lighLogging;

static const char* h_str =
"The following keys are supported / required:\n"
"-? or -h	== this is help\n\n"
"-nosun	== disable sun-lighting\n\n"
"-norgb	== disable common lightmap calculating\n\n"
"-f<NAME>	== compile level in GameData\\Levels\\<NAME>\\\n\n"
"-noinvalidfaces	== invalid faces detection\n\n"
"-skipinvalid	== skip crash if invalid faces exists\n\n"
"-notessellation	== compile level without tessellation\n\n"
"-lmap_quality	== lightmap quality\n\n"
"-threads <NUM>	== numbers of threads. Ex. -threads 16 \n\n"
"-p	== high priority \n\n"
"-r	== restore LMaps compilation \n\n"
"-no_ok_message	== don't show completion 'OK' message\n\n"
"-shutdown == Makes compiler to turn off PC when its done compiling\n\n"
"-pure_alloc == required for lc and do lights compiler\n\n"
"-no_strange_face_log == disables some log messages\n\n"
"-light_logging == disables some log messages\n\n"
"-allow_replace_thm_tga == show Yes/No message box, when thm or tga is missing, to replace it with dummy"
"\n"
"NOTE: The -f<NAME> is required for any functionality\n";

void Help()
{
	MessageBox(0, h_str, "Command line options", MB_OK | MB_ICONINFORMATION);
}

typedef int __cdecl xrOptions(b_params* params, u32 version, bool bRunBuild);

void get_console_param(const char *cmd, const char *param_name, const char *expr, void* param)
{
	if (strstr(cmd, param_name))
	{
		int						sz = xr_strlen(param_name);
		sscanf(strstr(cmd, param_name) + sz, expr, param);
	}
}

void get_console_float(const char *cmd, const char *param_name, float* param)
{
	get_console_param(cmd, param_name, "%f", param);
};

void get_console_u32(const char *cmd, const char *param_name, u32* param)
{
	get_console_param(cmd, param_name, "%u", param);
};

void Startup(LPSTR     lpCmdLine)
{
	char cmd[512], name[256];

	xr_strcpy(cmd, lpCmdLine);
	strlwr(cmd);
	if (strstr(cmd, "-?") || strstr(cmd, "-h"))			{ Help(); return; }
	if (strstr(cmd, "-f") == 0)							{ Help(); return; }
	if (strstr(cmd, "-gi"))								b_radiosity = TRUE;
	if (strstr(cmd, "-noise"))							b_noise = TRUE;
	if (strstr(cmd, "-nosun"))							b_nosun = TRUE;
	if (strstr(cmd, "-norgb"))							b_norgb = TRUE;
	if (strstr(cmd, "-noinvalidfaces"))					b_noinvalidfaces = TRUE;
	if (strstr(cmd, "-skipinvalid"))					b_skipinvalid = TRUE;
	if (strstr(cmd, "-notessellation"))                 b_notessellation = TRUE;
	if (strstr(cmd, "-no_strange_face_log"))			strangeFaceLogging = false;
	if (strstr(cmd, "-light_logging"))					lighLogging = true;

	get_console_float(lpCmdLine, "-lmap_quality ", &f_lmap_quality);
	get_console_u32(lpCmdLine, "-lmap_j_quality ", &f_lmap_j_quality);

	// Give a LOG-thread a chance to startup
	//_set_sbh_threshold(1920);
	InitCommonControls();
	thread_spawn(logThread, "log-update", 1024 * 1024, 0);
	Sleep(150);

	Msg("To get list of possible params - run xrLC directly from exe file or prompt '-?' or '-h'");

	// Faster FPU 
	SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

	/*
	u32	dwMin			= 1800*(1024*1024);
	u32	dwMax			= 1900*(1024*1024);
	if (0==SetProcessWorkingSetSize(GetCurrentProcess(),dwMin,dwMax))
	{
	clMsg("*** Failed to expand working set");
	};
	*/

	// Load project
	name[0] = 0;				sscanf(strstr(cmd, "-f") + 2, "%s", name);

	extern  HWND logWindow;
	string256				temp;
	xr_sprintf(temp, "%s - Levels Compiler", name);
	SetWindowText(logWindow, temp);

	string_path				prjName;
	FS.update_path(prjName, "$game_levels$", strconcat(sizeof(prjName), prjName, name, "\\build.prj"));
	string256				phaseName;
	Phase(strconcat(sizeof(phaseName), phaseName, "Reading project [", name, "]..."));

	string256 inf;
	IReader*	F = FS.r_open(prjName);
	if (NULL == F)
	{
		xr_sprintf(inf, "Build failed!\nCan't find level: '%s'", prjName);
		clMsg(inf);
		MessageBox(logWindow, inf, "Error!", MB_OK | MB_ICONERROR);
		return;
	}

	// Version
	F->r_chunk(EB_Version, &version);
	clMsg("version: %d", version);
	R_ASSERT(XRCL_CURRENT_VERSION == version);

	// Header
	b_params				Params;
	F->r_chunk(EB_Parameters, &Params);

	if (f_lmap_quality > 0.f)
		Params.m_lm_pixels_per_meter = f_lmap_quality;

	if (f_lmap_j_quality > 0.f)
		Params.m_lm_jitter_samples = f_lmap_j_quality;

	Msg(LINE_SPACER);
	Msg("- LEVEL QUALITY PARAMS:");
	Msg("---Quality flag            = %s", (Params.m_quality == ebqDraft) ? "Draft" : (Params.m_quality == ebqHigh) ? "High" : (Params.m_quality == ebqCustom) ? "Custom" : "Unknown");
	Msg("---LM Jitter Samples       = %u", Params.m_lm_jitter_samples);
	Msg("---LM Pixels per meter     = %f", Params.m_lm_pixels_per_meter);
	Msg("---LM collapsing           = %u", Params.m_lm_rms);
	Msg("---LM zero                 = %u", Params.m_lm_rms_zero);
	Msg("---Normal smooth angle     = %f", Params.m_sm_angle);
	Msg("---Weld distance           = %f", Params.m_weld_distance);

	// Show options if needed
	if (0) // no xrLC_options project?!
	{
		Phase("Project options...");
		HMODULE		L = LoadLibrary("xrLC_Options.dll");
		void*		P = GetProcAddress(L, "_frmScenePropertiesRun");
		R_ASSERT(P);
		xrOptions*	O = (xrOptions*)P;
		int			R = O(&Params, version, false);
		FreeLibrary(L);
		if (R == 2)
		{
			ExitProcess(0);
		}
	}

	// Conversion
	Phase("Converting data structures...");
	pBuild = xr_new<CBuild>();
	pBuild->Load(Params, *F);
	FS.r_close(F);

	// Call for builder
	string_path				lfn;
	CTimer	dwStartupTime;	dwStartupTime.Start();
	FS.update_path(lfn, _game_levels_, name);
	pBuild->Run(lfn);
	xr_delete(pBuild);

	// Show statistic
	extern	std::string make_time(u32 sec);
	u32	dwEndTime = dwStartupTime.GetElapsed_ms();

	xr_sprintf(inf, "Time elapsed: %s", make_time(dwEndTime / 1000).c_str());

	clMsg("Build succesful!\n%s", inf);

	if (!strstr(lpCmdLine, "-shutdown") && !strstr(Core.Params, "-no_ok_message"))
		MessageBox(logWindow, inf, "Congratulation!", MB_OK | MB_ICONINFORMATION);

	// Close log
	bClose = TRUE;
	Sleep(500);
}

typedef void DUMMY_STUFF(const void*, const u32&, void*);
XRCORE_API DUMMY_STUFF	*g_temporary_stuff;
XRCORE_API DUMMY_STUFF	*g_dummy_stuff;

int APIENTRY WinMain(HINSTANCE hInst,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nCmdShow)
{
	g_temporary_stuff = &trivial_encryptor::decode;
	g_dummy_stuff = &trivial_encryptor::encode;

	// check custom fsgame.ltx
	LPCSTR		fsgame_ltx_name = "-fsltx ";
	string_path	fsgame = "";
	if (strstr(lpCmdLine, fsgame_ltx_name))
	{
		int		sz = xr_strlen(fsgame_ltx_name);
		sscanf(strstr(lpCmdLine, fsgame_ltx_name) + sz, "%[^ ] ", fsgame);
	}
	// Initialize debugging
	Debug._initialize(false);
	Core._initialize("xrlc_la", NULL, TRUE, fsgame[0] ? fsgame : FSLTX);
	Startup(lpCmdLine);
	Core._destroy();
	if (strstr(lpCmdLine, "-shutdown"))
	{
		HANDLE				token;
		TOKEN_PRIVILEGES	token_privileges;

		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
			return FALSE;

		LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &token_privileges.Privileges[0].Luid);

		token_privileges.PrivilegeCount = 1;
		token_privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		AdjustTokenPrivileges(token, FALSE, &token_privileges, 0, 0, 0);

		if (GetLastError() != ERROR_SUCCESS)
			return FALSE;

		ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, SHTDN_REASON_MAJOR_OPERATINGSYSTEM | SHTDN_REASON_MINOR_UPGRADE | SHTDN_REASON_FLAG_PLANNED);
	}
	return 0;
}
