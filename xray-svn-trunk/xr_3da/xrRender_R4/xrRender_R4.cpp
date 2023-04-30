// Defines the entry point for the DLL application.

#include "stdafx.h"
#include "../xrRender/dxRenderFactory.h"
#include "../xrRender/dxUIRender.h"
#include "../xrRender/dxDebugRender.h"

#pragma comment(lib,"xr_3da.lib")

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH	:
		//	Can't call CreateDXGIFactory from DllMain

		::Render					= &RImplementation;
		::RenderFactory				= &RenderFactoryImpl;
		::DU						= &DUImpl;

		UIRender					= &UIRenderImpl;

#ifdef DRENDER
		DRender						= &DebugRenderImpl;
#endif
		xrRender_initconsole();

		break;
	case DLL_THREAD_ATTACH	:
	case DLL_THREAD_DETACH	:
	case DLL_PROCESS_DETACH	:
		break;
	}

	return TRUE;
}


extern "C"
{
	bool _declspec(dllexport) SupportsDX11Rendering();
};

bool _declspec(dllexport) SupportsDX11Rendering()
{
	return xrRender_test_hw() ? true : false;
}