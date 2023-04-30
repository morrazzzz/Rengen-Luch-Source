// EngineAPI.cpp: implementation of the CEngineAPI class.

#include "stdafx.h"
#include "EngineAPI.h"

extern xr_token* vid_quality_token;

CEngineAPI::CEngineAPI	()
{
	hGame			= 0;
	hRender			= 0;

	pCreate			= 0;
	pDestroy		= 0;

	xrGameModuleExist = false;
	xrRenderModuleExist = false;
}

CEngineAPI::~CEngineAPI()
{
	// destroy quality token here
	if (vid_quality_token)
	{
		for( int i=0; vid_quality_token[i].name; i++ )
		{
			xr_free					(vid_quality_token[i].name);
		}

		xr_free						(vid_quality_token);
		vid_quality_token			= NULL;
	}
}

ENGINE_API int g_current_renderer = 0;

void CEngineAPI::Initialize()
{
	Msg("# Initializing Other DDLs...");

	// Render
	CreateRendererList();

	LPCSTR r4_name = "xrRender_R4.dll";

	if (psDeviceFlags.test(rsR4))
	{
		// try to initialize R4
		Log("* Loading DLL:", r4_name);
		hRender = LoadLibrary(r4_name);

		if (!hRender)
		{
			Msg("No render dll found or DX 11 is not supported. xrRender_R4.dll");
		}
		else
		{
			xrRenderModuleExist = true;

			g_current_renderer = 4;
		}
	}

	R_ASSERT2(hRender, "No xrRender dll");

	if(hRender)
		Device.ConnectToRender();

	// Game	
	{
		LPCSTR g_name = "xrGame.dll";
		Log("* Loading DLL:", g_name);
		hGame = LoadLibrary(g_name);

		if (hGame)
		{
			xrGameModuleExist = true;

			pCreate = (Factory_Create*)GetProcAddress(hGame, "xrFactory_Create");

			R_ASSERT(pCreate);

			pDestroy = (Factory_Destroy*)GetProcAddress(hGame, "xrFactory_Destroy");

			R_ASSERT(pDestroy);
		}
		else
			R_ASSERT2(false, "No xrGame dll");
	}

	FlushLog(false);
}

void CEngineAPI::Destroy()
{
	if (hGame)
	{
		FreeLibrary(hGame);
		hGame	= 0;
	}

	if (hRender)
	{
		FreeLibrary(hRender);
		hRender = 0;
	}

	pCreate					= 0;
	pDestroy				= 0;

	Engine.Event._destroy	();
	XRC.r_clear_compact		();
}

extern "C" {
	typedef bool _declspec(dllexport) SupportsDX11Rendering();
};

void CEngineAPI::CreateRendererList()
{
	if(vid_quality_token != NULL)
		return;

	bool bSupports_r4 = false;

	LPCSTR r4_name = "xrRender_R4.dll";

	xr_vector<LPCSTR>_tmp;

	{
		// try to initialize R4
		Log("* Loading DLL:", r4_name);

		// Hide "d3d10.dll not found" message box for XP
		SetErrorMode(SEM_FAILCRITICALERRORS);
		hRender = LoadLibrary(r4_name);
		// Restore error handling
		SetErrorMode(0);

		if (hRender)
		{
			SupportsDX11Rendering* test_dx11_rendering = (SupportsDX11Rendering*)GetProcAddress(hRender, "SupportsDX11Rendering");

			R_ASSERT(test_dx11_rendering);

			bSupports_r4 = test_dx11_rendering();

			FreeLibrary(hRender);

			if (bSupports_r4)
			{
				LPCSTR val = NULL;

				_tmp.push_back(val);
				val = "renderer_r4";
				_tmp.back() = xr_strdup(val);
			}
		}
		else
			Msg("* %s Not loaded", r4_name);
	}

	hRender = 0;

	if(_tmp.size() == 0)
		Msg("Not a single renderer could be initialized");

	u32 _cnt								= _tmp.size() + 1;
	vid_quality_token						= xr_alloc<xr_token>(_cnt);

	vid_quality_token[_cnt-1].id			= -1;
	vid_quality_token[_cnt-1].name			= NULL;

	Msg("^ Available render modes count [%d]:", _tmp.size());

	for(u32 i = 0; i < _tmp.size();++i)
	{
		vid_quality_token[i].id				= i;
		vid_quality_token[i].name			= _tmp[i];

		Msg	("^ [%s]", _tmp[i]);
	}
}