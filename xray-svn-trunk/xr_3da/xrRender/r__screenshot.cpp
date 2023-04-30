#include "stdafx.h"

#include "xr_effgamma.h"
#include "dxRenderDeviceRender.h"
#include "../xrRender/tga.h"
#include "../../xr_3da/xrImage_Resampler.h"

#include "d3dx10tex.h"

#define	GAMESAVE_SIZE	128
#define SM_FOR_SEND_WIDTH 640
#define SM_FOR_SEND_HEIGHT 480

string_path screenName;
CRender::ScreenshotMode screenMode = CRender::SM_NORMAL;

void CRender::ScreenshotImpl(ScreenshotMode mode, LPCSTR name)
{
	Msg("1 %d", RImplementation.m_bMakeAsyncSS);

	HRESULT hr1;
	ID3D11Texture2D* pBuffer;
	hr1 = HW.m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBuffer);

	// Save
	switch (mode)
	{
	case IRender_interface::SM_FOR_GAMESAVE:
	{
		ID3DTexture2D* pSrcSmallTexture;

		D3D_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Width = GAMESAVE_SIZE;
		desc.Height = GAMESAVE_SIZE;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_BC1_UNORM;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D_USAGE_DEFAULT;
		desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
		CHK_DX(HW.pDevice->CreateTexture2D(&desc, NULL, &pSrcSmallTexture));

		//	D3DX10_TEXTURE_LOAD_INFO *pLoadInfo

		CHK_DX(D3DX11LoadTextureFromTexture(HW.pContext, pBuffer, NULL, pSrcSmallTexture));

		// save (logical & physical)
		ID3DBlob* saved = 0;

		hr1 = D3DX11SaveTextureToMemory(HW.pContext, pSrcSmallTexture, D3DX11_IFF_DDS, &saved, 0);

		if (hr1 == D3D_OK)
		{
			IWriter* fs = FS.w_open(name);
			if (fs)
			{
				fs->w(saved->GetBufferPointer(), (u32)saved->GetBufferSize());
				FS.w_close(fs);
			}
		}
		_RELEASE(saved);

		// cleanup
		_RELEASE(pSrcSmallTexture);
	}
	break;

	case IRender_interface::SM_NORMAL:
	{
		string64 t_stemp;
		string_path buf;

		xr_sprintf(buf, sizeof(buf), "ss_%s_%s_(%s)_%s.jpg", Core.UserName, timestamp(t_stemp),
			(g_pGameLevel) ? g_pGameLevel->name().c_str() : "mainmenu", name);

		ID3DBlob* saved = 0;

		CHK_DX(D3DX11SaveTextureToMemory(HW.pContext, pBuffer, D3DX11_IFF_JPG, &saved, 0));

		IWriter* fs = FS.w_open("$screenshots$", buf);
		R_ASSERT(fs);
		fs->w(saved->GetBufferPointer(), (u32)saved->GetBufferSize());
		FS.w_close(fs);
		_RELEASE(saved);

		// Additional formats
		if (strstr(Core.Params, "-ss_png"))
		{
			xr_sprintf(buf, sizeof(buf), "ssq_%s_%s_(%s)_%s.png", Core.UserName, timestamp(t_stemp),
				(g_pGameLevel) ? g_pGameLevel->name().c_str() : "mainmenu", name);

			saved = 0;

			CHK_DX(D3DX11SaveTextureToMemory(HW.pContext, pBuffer, D3DX11_IFF_PNG, &saved, 0));

			fs = FS.w_open("$screenshots$", buf);
			R_ASSERT(fs);
			fs->w(saved->GetBufferPointer(), (u32)saved->GetBufferSize());
			FS.w_close(fs);
			_RELEASE(saved);
		}

		if (strstr(Core.Params, "-ss_bmp"))
		{
			xr_sprintf(buf, sizeof(buf), "ssq_%s_%s_(%s)_%s.bmp", Core.UserName, timestamp(t_stemp),
				(g_pGameLevel) ? g_pGameLevel->name().c_str() : "mainmenu", name);

			saved = 0;

			CHK_DX(D3DX11SaveTextureToMemory(HW.pContext, pBuffer, D3DX11_IFF_BMP, &saved, 0));

			fs = FS.w_open("$screenshots$", buf);
			R_ASSERT(fs);
			fs->w(saved->GetBufferPointer(), (u32)saved->GetBufferSize());
			FS.w_close(fs);
			_RELEASE(saved);
		}

	}

	break;

	case IRender_interface::SM_FOR_LEVELMAP:
	{
		string64 t_stemp;
		string_path buf;

		xr_sprintf(buf, sizeof(buf), "ssq_%s_%s_(%s).bmp", Core.UserName, timestamp(t_stemp),
			(g_pGameLevel) ? g_pGameLevel->name().c_str() : "mainmenu");

		ID3DBlob* saved = 0;

		ID3DTexture2D* resulted;

		D3D_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(desc));

		desc.Width = SM_FOR_SEND_WIDTH;
		desc.Height = SM_FOR_SEND_HEIGHT;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_BC1_UNORM;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D_USAGE_DEFAULT;
		desc.BindFlags = D3D_BIND_SHADER_RESOURCE;

		CHK_DX(HW.pDevice->CreateTexture2D(&desc, NULL, &resulted));

		CHK_DX(D3DX11SaveTextureToMemory(HW.pContext, pBuffer, D3DX11_IFF_BMP, &saved, 0));

		IWriter* fs = FS.w_open("$screenshots$", buf);

		Msg("Map screenshot path %s", fs->fName.c_str());

		R_ASSERT(fs);

		fs->w(saved->GetBufferPointer(), (u32)saved->GetBufferSize());
		FS.w_close(fs);

		_RELEASE(saved);
	}
	break;

	case IRender_interface::SM_FOR_CUBEMAP:
	{
		R_ASSERT(!"CRender::Screenshot. This screenshot type is not supported for DX10/11.");
		/*
		string64			t_stemp;
		string_path			buf;
		VERIFY				(name);
		strconcat			(sizeof(buf),buf,"ss_",Core.UserName,"_",timestamp(t_stemp),"_#",name);
		xr_strcat				(buf,".tga");
		IWriter*		fs	= FS.w_open	("$screenshots$",buf); R_ASSERT(fs);
		TGAdesc				p;
		p.format			= IMG_24B;

		//	TODO: DX10: This is totally incorrect but mimics
		//	original behaviour. Fix later.
		hr					= pFB->LockRect(&D,0,D3DLOCK_NOSYSLOCK);
		if(hr!=D3D_OK)		return;
		hr					= pFB->UnlockRect();
		if(hr!=D3D_OK)		goto _end_;

		// save
		u32* data			= (u32*)xr_malloc(Device.dwHeight*Device.dwHeight*4);
		imf_Process
		(data,Device.dwHeight,Device.dwHeight,(u32*)D.pBits,Device.dwWidth,Device.dwHeight,imf_lanczos3);
		p.scanlenght		= Device.dwHeight*4;
		p.width				= Device.dwHeight;
		p.height			= Device.dwHeight;
		p.data				= data;
		p.maketga			(*fs);
		xr_free				(data);

		FS.w_close			(fs);
		*/
	}
	break;
	}

	_RELEASE(pBuffer);
}

void CRender::Screenshot(ScreenshotMode mode, LPCSTR name)
{
	if (name)
		xr_sprintf(screenName, "%s", name);

	screenMode = mode;

	ScreenshotAsyncBegin();
}

void CRender::ScreenshotAsyncBegin()
{
	VERIFY(!m_bMakeAsyncSS);

	m_bMakeAsyncSS = true;
}

void CRender::ScreenshotAsync()
{
	if (RImplementation.m_bMakeAsyncSS)
	{
		RImplementation.m_bMakeAsyncSS = false;

		ScreenshotImpl(screenMode, screenName);
	}
}

