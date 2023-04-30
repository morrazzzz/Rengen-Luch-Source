#include "stdafx.h"
#include "x_ray.h"
#include "Main.h"
#include "xr_ioconsole.h"
#include "GameFont.h"
#include "resource.h"

#define TRIVIAL_ENCRYPTOR_DECODER
#include "trivial_encryptor.h"

#define STALKER_PRESENCE_MUTEX "RENTGEN-LUCH"

HWND logoWindow = NULL;
BOOL g_bIntroFinished = FALSE;

typedef void DUMMY_STUFF(const void*, const u32&, void*);
XRCORE_API DUMMY_STUFF* g_temporary_stuff;

bool CApplication::isDeveloperMode;

extern void setup_luabind_allocator();
int APIENTRY WinMain_impl(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* lpCmdLine, int nCmdShow);
void LogoWindow();
void SetHeap();
bool CheckAnotherAppInstance();


// A Windows entry point for our GUI application
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* lpCmdLine, int nCmdShow)
{
	__try
	{
		WinMain_impl(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
	}
	__except (stack_overflow_exception_filter(GetExceptionCode()))
	{
		_resetstkoflw();

		MessageBox(GetTopWindow(0), "WinMain_impl stack overflow", "Stack Overflow", MB_OK | MB_ICONERROR | MB_TASKMODAL);
	}

	return(0);
}

int APIENTRY WinMain_impl(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* lpCmdLine, int nCmdShow)
{
	Debug._initialize(false);

	SetHeap();

	// Check for another instance(No multi instances support?)
	if (!CheckAnotherAppInstance())
		return 1;

	LogoWindow();

	g_temporary_stuff = &trivial_encryptor::decode;

	compute_build_id();

	LPCSTR fsgame_ltx_name = "-fsltx ";
	string_path fsgame = "";

	if (strstr(lpCmdLine, fsgame_ltx_name))
	{
		int	sz = xr_strlen(fsgame_ltx_name);
		sscanf(strstr(lpCmdLine, fsgame_ltx_name) + sz, "%[^ ] ", fsgame);
	}

	Core._initialize("xr_3da", NULL, TRUE, fsgame[0] ? fsgame : NULL);

#ifdef DEBUG
	CApplication::isDeveloperMode = true;
#else
	CApplication::isDeveloperMode = strstr(Core.Params, "-developer") != nullptr;
#endif

	CApplication::InitSettings();

	CApplication::InitEngine();

	// Continue initing
	CApplication::Startup();

	// Do usefull stuff
	CApplication::Process();

	// Application is about to close
	CApplication::EndUp();
	Core._destroy();

	return 0;
}

void SetHeap()
{
	if (!IsDebuggerPresent())
	{
		HMODULE const kernel32 = LoadLibrary("kernel32.dll");
		R_ASSERT(kernel32);

		typedef BOOL(__stdcall * HeapSetInformation_type) (HANDLE, HEAP_INFORMATION_CLASS, PVOID, SIZE_T);

		HeapSetInformation_type const heap_set_information = (HeapSetInformation_type)GetProcAddress(kernel32, "HeapSetInformation");

		if (heap_set_information)
		{
			ULONG HeapFragValue = 2;

#ifdef DEBUG
			BOOL const result =
#endif
				heap_set_information(GetProcessHeap(), HeapCompatibilityInformation, &HeapFragValue, sizeof(HeapFragValue));

			VERIFY2(result, "can't set process heap low fragmentation");
		}
	}
}

bool CheckAnotherAppInstance()
{
	HANDLE hCheckPresenceMutex = INVALID_HANDLE_VALUE;

	hCheckPresenceMutex = OpenMutex(READ_CONTROL, FALSE, STALKER_PRESENCE_MUTEX);

	if (hCheckPresenceMutex == NULL)
	{
		// New mutex
		hCheckPresenceMutex = CreateMutex(NULL, FALSE, STALKER_PRESENCE_MUTEX);

		if (hCheckPresenceMutex == NULL)
			// Shit happens
			return false;
	}
	else
	{
		// Already running
		CloseHandle(hCheckPresenceMutex);

		return false;
	}

	return true;
}

void LogoWindow()
{
	// Title window
	logoWindow = CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_STARTUP), 0, logDlgProc);
	setup_luabind_allocator();
	HWND logoPicture = GetDlgItem(logoWindow, IDC_STATIC_LOGO);

	RECT logoRect;

	GetWindowRect(logoPicture, &logoRect);

	SetWindowPos(logoWindow,

#ifndef DEBUG
		HWND_TOPMOST,
#else
		HWND_NOTOPMOST,
#endif

		0, 0, logoRect.right - logoRect.left, logoRect.bottom - logoRect.top, SWP_NOMOVE | SWP_SHOWWINDOW// | SWP_NOSIZE
	);

	UpdateWindow(logoWindow);

	// AVI
	g_bIntroFinished = TRUE;

}