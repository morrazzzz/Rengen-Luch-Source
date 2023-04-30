// xrAI.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "process.h"
#include "global_options.h"

//#pragma comment(linker,"/STACK:0x800000,0x400000")

#pragma comment(lib,"comctl32.lib")
//#pragma comment(lib,"d3dx9.lib")
//#pragma comment(lib,"IMAGEHLP.LIB")
#pragma comment(lib,"winmm.LIB")
#pragma comment(lib,"xrCDB.lib")
#pragma comment(lib,"xrCore.lib")
#pragma comment(lib,"FreeImage.lib")

#define FSLTX "fs.ltx"

extern void	xrCompiler(LPCSTR name);
extern void logThread(void *dummy);
extern volatile BOOL bClose;

static const char* h_str =
"The following keys are supported / required:\n"
"-? or -h	== this is help\n\n"
"-f<NAME>	== compile level in gamedata\\levels\\<NAME>\\\n\n"
"-norgb	== disable common lightmap calculating\n\n"
"-nosun	== disable sun-lighting\n\n"
"-threads <NUM>	== numbers of threads. Ex. -threads 16 \n\n"
"-p	== high priority \n\n"
"-no_ok_message	== don't show completion 'OK' message\n\n"
"-pure_alloc == required for lc and do lights compiler\n\n"
"-allow_replace_thm_tga == show Yes/No message box, when thm or tga is missing, to replace it with dummy"
"\n"
"NOTE: The -f<NAME> is required for any functionality\n";

void Help()
{
	MessageBox(0, h_str, "Command line options", MB_OK | MB_ICONINFORMATION);

	u32 NUM_THREADS = 0;

	if (strstr(Core.Params, "-threads "))
		sscanf(strstr(Core.Params, "-threads ") + xr_strlen("-threads"), "%d", &NUM_THREADS);

	Msg("%d", NUM_THREADS);
}

void Startup(LPSTR     lpCmdLine)
{
	char cmd[512], name[256];

	xr_strcpy(cmd, lpCmdLine);
	strlwr(cmd);
	if (strstr(cmd, "-?") || strstr(cmd, "-h"))			{ Help(); return; }
	if (strstr(cmd, "-f") == 0)							{ Help(); return; }
	// KD: new options
	if (strstr(cmd, "-norgb"))							b_norgb = TRUE;
	if (strstr(cmd, "-nosun"))							b_nosun = TRUE;

	// Give a LOG-thread a chance to startup
	InitCommonControls();
	thread_spawn(logThread, "log-update", 1024 * 1024, 0);
	Sleep(150);

	Msg("To get list of possible params - run xrDO_Light directly from exe file or prompt '-?' or '-h'");

	// Load project
	name[0] = 0; sscanf(strstr(cmd, "-f") + 2, "%s", name);

	extern  HWND logWindow;
	string256			temp;
	xr_sprintf(temp, "%s - Detail Compiler", name);
	SetWindowText(logWindow, temp);

	//FS.update_path	(name,"$game_levels$",name);
	FS.get_path("$level$")->_set(name);

	CTimer				dwStartupTime; dwStartupTime.Start();
	xrCompiler(0);

	// Show statistic
	char	stats[256];
	extern	std::string make_time(u32 sec);

	xr_sprintf(stats, "Time elapsed: %s", make_time((dwStartupTime.GetElapsed_ms()) / 1000).c_str());

	clMsg("Build succesful!\n%s", stats);

	if (!strstr(Core.Params, "-no_ok_message"))
		MessageBox(logWindow, stats, "Congratulation!", MB_OK | MB_ICONINFORMATION);

	bClose = TRUE;
	Sleep(500);
}

int APIENTRY WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nCmdShow)
{

	// check custom fsgame.ltx
	string_path	fsgame = "";
	LPCSTR		fsgame_ltx_name = "-fsltx ";
	if (strstr(lpCmdLine, fsgame_ltx_name))
	{
		u32 sz = xr_strlen(fsgame_ltx_name);
		sscanf(strstr(lpCmdLine, fsgame_ltx_name) + sz, "%[^ ] ", fsgame);
	}

	// Initialize debugging
	Debug._initialize(false);
	Core._initialize("xrdo_la", 0, 1, fsgame[0] ? fsgame : FSLTX);
	Startup(lpCmdLine);

	return 0;
}
