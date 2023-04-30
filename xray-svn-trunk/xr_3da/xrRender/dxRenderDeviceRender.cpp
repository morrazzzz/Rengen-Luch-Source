#include "stdafx.h"
#include "dxRenderDeviceRender.h"

#include "ResourceManager.h"

dxRenderDeviceRender::dxRenderDeviceRender()
	:	Resources(0)
{
	;
#if defined(USE_DX10) || defined(USE_DX11)
	HW.lastPresentStatus = S_OK;
#endif
}

void dxRenderDeviceRender::Copy(IRenderDeviceRender &_in)
{
	*this = *(dxRenderDeviceRender*)&_in;
}

void dxRenderDeviceRender::setGamma(float fGamma)
{
	m_Gamma.Gamma(fGamma);
	updateGamma();
}

void dxRenderDeviceRender::setBrightness(float fGamma)
{
	m_Gamma.Brightness(fGamma);
	updateGamma();
}

void dxRenderDeviceRender::setContrast(float fGamma)
{
	m_Gamma.Contrast(fGamma);
	updateGamma();
}

void dxRenderDeviceRender::updateGamma()
{
	m_Gamma.Update();
}

void dxRenderDeviceRender::OnDeviceDestroy( BOOL bKeepTextures)
{
	m_WireShader.destroy();
	m_SelectionShader.destroy();

	Resources->OnDeviceDestroy( bKeepTextures);
	RCache.OnDeviceDestroy();
}

void dxRenderDeviceRender::ValidateHW()
{
	HW.Validate();
}

void dxRenderDeviceRender::DestroyHW()
{
	xr_delete					(Resources);
	HW.DestroyDevice			();
}

void  dxRenderDeviceRender::Reset( HWND hWnd, u32 &dwWidth, u32 &dwHeight, float &fWidth_2, float &fHeight_2)
{
#ifdef DEBUG
	_SHOW_REF("*ref -CRenderDevice::ResetTotal: DeviceREF:",HW.pDevice);
#endif // DEBUG	

	Resources->reset_begin	();
	Memory.mem_compact		();
	HW.Reset				(hWnd);

#if defined(USE_DX10) || defined(USE_DX11)
	dwWidth					= HW.m_ChainDesc.BufferDesc.Width;
	dwHeight				= HW.m_ChainDesc.BufferDesc.Height;
#else	//	USE_DX10
	dwWidth					= HW.DevPP.BackBufferWidth;
	dwHeight				= HW.DevPP.BackBufferHeight;
#endif	//	USE_DX10

	fWidth_2				= float(dwWidth/2);
	fHeight_2				= float(dwHeight/2);
	Resources->reset_end	();

#ifdef DEBUG
	_SHOW_REF("*ref +CRenderDevice::ResetTotal: DeviceREF:",HW.pDevice);
#endif // DEBUG
}

void dxRenderDeviceRender::SetupStates()
{
	HW.Caps.Update			();

// In DX10 we have a shader pipeline instead. 
// Rasterizer and depth stencil states are managed
// in state cache.
}

void dxRenderDeviceRender::OnDeviceCreate(LPCSTR shName)
{
	// Signal everyone - device created
	RCache.OnDeviceCreate		();
	Resources->OnDeviceCreate	(shName);

	::Render->create			();
	Device.Statistic->OnDeviceCreate	();

	m_WireShader.create			("editor\\wire");
	m_SelectionShader.create	("editor\\selection");

	DUImpl.OnDeviceCreate();
	m_Gamma.Update();

}

void dxRenderDeviceRender::Create( HWND hWnd, u32 &dwWidth, u32 &dwHeight, float &fWidth_2, float &fHeight_2, bool move_window)
{
	HW.CreateDevice		(hWnd, move_window);

	dwWidth					= HW.m_ChainDesc.BufferDesc.Width;
	dwHeight				= HW.m_ChainDesc.BufferDesc.Height;

	fWidth_2			= float(dwWidth/2);
	fHeight_2			= float(dwHeight/2);

	Resources			= xr_new <CResourceManager>();

	//HW.OnAppDeactivate();
	//HW.OnAppActivate();

	}

void dxRenderDeviceRender::SetupGPU( BOOL bForceGPU_SW, BOOL bForceGPU_NonPure, BOOL bForceGPU_REF)
{
	HW.Caps.bForceGPU_SW		= bForceGPU_SW;
	HW.Caps.bForceGPU_NonPure	= bForceGPU_NonPure;
	HW.Caps.bForceGPU_REF		= bForceGPU_REF;
}

void dxRenderDeviceRender::overdrawBegin()
{
#if defined(USE_DX10) || defined(USE_DX11)
	//	TODO: DX10: Implement overdrawBegin
	VERIFY(!"dxRenderDeviceRender::overdrawBegin not implemented.");
#else	//	USE_DX10
	// Turn stenciling
	CHK_DX(HW.pDevice->SetRenderState( D3DRS_STENCILENABLE,		TRUE			));
	CHK_DX(HW.pDevice->SetRenderState( D3DRS_STENCILFUNC,		D3DCMP_ALWAYS	));
	CHK_DX(HW.pDevice->SetRenderState( D3DRS_STENCILREF,		0				));
	CHK_DX(HW.pDevice->SetRenderState( D3DRS_STENCILMASK,		0x00000000		));
	CHK_DX(HW.pDevice->SetRenderState( D3DRS_STENCILWRITEMASK,	0xffffffff		));

	// Increment the stencil buffer for each pixel drawn
	CHK_DX(HW.pDevice->SetRenderState( D3DRS_STENCILFAIL,		D3DSTENCILOP_KEEP		));
	CHK_DX(HW.pDevice->SetRenderState( D3DRS_STENCILPASS,		D3DSTENCILOP_INCRSAT	));

	if (1==HW.Caps.SceneMode)		
	{ CHK_DX(HW.pDevice->SetRenderState( D3DRS_STENCILZFAIL,	D3DSTENCILOP_KEEP		)); }	// Overdraw
	else 
	{ CHK_DX(HW.pDevice->SetRenderState( D3DRS_STENCILZFAIL,	D3DSTENCILOP_INCRSAT	)); }	// ZB access
#endif	//	USE_DX10
}

void dxRenderDeviceRender::overdrawEnd()
{
#if defined(USE_DX10) || defined(USE_DX11)
	//	TODO: DX10: Implement overdrawEnd
	VERIFY(!"dxRenderDeviceRender::overdrawBegin not implemented.");
#else	//	USE_DX10
	// Set up the stencil states
	CHK_DX	(HW.pDevice->SetRenderState( D3DRS_STENCILZFAIL,		D3DSTENCILOP_KEEP	));
	CHK_DX	(HW.pDevice->SetRenderState( D3DRS_STENCILFAIL,		D3DSTENCILOP_KEEP	));
	CHK_DX	(HW.pDevice->SetRenderState( D3DRS_STENCILPASS,		D3DSTENCILOP_KEEP	));
	CHK_DX	(HW.pDevice->SetRenderState( D3DRS_STENCILFUNC,		D3DCMP_EQUAL		));
	CHK_DX	(HW.pDevice->SetRenderState( D3DRS_STENCILMASK,		0xff				));

	// Set the background to black
	CHK_DX	(HW.pDevice->Clear(0,0,D3DCLEAR_TARGET,D3DCOLOR_XRGB(255,0,0),0,0));

	// Draw a rectangle wherever the count equal I
	RCache.OnFrameEnd	();
	CHK_DX	(HW.pDevice->SetFVF( FVF::F_TL ));

	// Render gradients
	for (int I=0; I<12; I++ ) 
	{
		u32	_c	= I*256/13;
		u32	c	= D3DCOLOR_XRGB(_c,_c,_c);

		FVF::TL	pv[4];
		pv[0].set(float(0),			float(Device.dwHeight),	c,0,0);			
		pv[1].set(float(0),			float(0),			c,0,0);					
		pv[2].set(float(Device.dwWidth),	float(Device.dwHeight),	c,0,0);	
		pv[3].set(float(Device.dwWidth),	float(0),			c,0,0);

		CHK_DX(HW.pDevice->SetRenderState	( D3DRS_STENCILREF,		I	));
		CHK_DX(HW.pDevice->DrawPrimitiveUP	( D3DPT_TRIANGLESTRIP,	2,	pv, sizeof(FVF::TL) ));
	}
	CHK_DX(HW.pDevice->SetRenderState( D3DRS_STENCILENABLE,		FALSE ));
#endif	//	USE_DX10
}

void dxRenderDeviceRender::DeferredLoad(BOOL E)
{
	Resources->DeferredLoad(E);
}

void dxRenderDeviceRender::ResourcesDeferredUpload(BOOL multithreaded)
{
	Resources->DeferredUpload(multithreaded);
}

void dxRenderDeviceRender::SyncTexturesLoadingProcess()
{
	Resources->SyncTexturesLoading();
}

void dxRenderDeviceRender::ResourcesGetMemoryUsage(u32& m_base, u32& c_base, u32& m_lmaps, u32& c_lmaps)
{
	if (Resources)
		Resources->_GetMemoryUsage(m_base, c_base, m_lmaps, c_lmaps);
}

void dxRenderDeviceRender::ResourcesStoreNecessaryTextures()
{
	dxRenderDeviceRender::Instance().Resources->StoreNecessaryTextures();
}

void dxRenderDeviceRender::ResourcesDumpMemoryUsage()
{
	dxRenderDeviceRender::Instance().Resources->_DumpMemoryUsage();
}

void dxRenderDeviceRender::RenderPrefetchUITextures()
{
	Resources->RMPrefetchUITextures();
}


dxRenderDeviceRender::DeviceState dxRenderDeviceRender::GetDeviceState()
{
	dxRenderDeviceRender::DeviceState state = dsOK;
	HW.Validate		();
#if defined(USE_DX10) || defined(USE_DX11)
#pragma todo("utak3r: mapping presentation results to device states should be carefully checked and corrected if needed.")
	switch (HW.lastPresentStatus)
	{
	case S_OK:
		state = dsOK;
		break;
	case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE:
	case DXGI_ERROR_FRAME_STATISTICS_DISJOINT:
	case DXGI_ERROR_WAS_STILL_DRAWING:
	case DXGI_ERROR_UNSUPPORTED:
		state = dsLost;
		break;
	case DXGI_ERROR_DEVICE_RESET:
	case DXGI_ERROR_DEVICE_REMOVED:
	case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
		state = dsNeedReset;
		break;
	default:
		state = dsOK;
	}
#else	//	USE_DX10
	HRESULT	_hr		= HW.pDevice->TestCooperativeLevel();
	if (FAILED(_hr))
	{
		// If the device was lost, do not render until we get it back
		if		(D3DERR_DEVICELOST==_hr)
			state = dsLost;

		// Check if the device is ready to be reset
		if		(D3DERR_DEVICENOTRESET==_hr)
			state = dsNeedReset;
	}
#endif	//	USE_DX10

	return state;
}

BOOL dxRenderDeviceRender::GetForceGPU_REF()
{
	return HW.Caps.bForceGPU_REF;
}

u32 dxRenderDeviceRender::GetCacheStatPolys()
{
	return RCache.stat.polys;
}

void dxRenderDeviceRender::RenderBegin(ViewPort vp)
{
	RCache.OnRenderBegin(vp);

	RCache.set_CullMode(CULL_CW);
	RCache.set_CullMode(CULL_CCW);

	if (HW.Caps.SceneMode)
		overdrawBegin();
}

void dxRenderDeviceRender::Clear()
{
#if defined(USE_DX10) || defined(USE_DX11)

	if (psDeviceFlags.test(rsClearBB))
	{
		FLOAT ColorRGBA[4] = {0.0f,0.0f,0.0f,1.0f};
		HW.pContext->ClearRenderTargetView(RCache.get_RT(), ColorRGBA);
	}
	HW.pContext->ClearDepthStencilView(RCache.get_ZB(), D3D10_CLEAR_DEPTH | D3D10_CLEAR_STENCIL, 1.0f, 0);
#else	//	USE_DX10
	CHK_DX(HW.pDevice->Clear(0,0,
		D3DCLEAR_ZBUFFER|
		(psDeviceFlags.test(rsClearBB)?D3DCLEAR_TARGET:0)|
		(HW.Caps.bStencil?D3DCLEAR_STENCIL:0),
		D3DCOLOR_XRGB(0,0,0),1,0
		));
#endif	//	USE_DX10
}

void dxRenderDeviceRender::ResourcesDestroyNecessaryTextures()
{
	Resources->DestroyNecessaryTextures();
}

void dxRenderDeviceRender::ClearTarget()
{
#if defined(USE_DX10) || defined(USE_DX11)
	FLOAT ColorRGBA[4] = {0.0f,0.0f,0.0f,1.0f};
	HW.pContext->ClearRenderTargetView(RCache.get_RT(), ColorRGBA);
#else	//	USE_DX10
	CHK_DX(HW.pDevice->Clear(0,0,D3DCLEAR_TARGET,D3DCOLOR_XRGB(0,0,0),1,0));
#endif	//	USE_DX10
}

void dxRenderDeviceRender::SwitchViewPortRTZB(ViewPort vp)
{

}

void dxRenderDeviceRender::SetCacheXform(Fmatrix &mView, Fmatrix &mProject)
{
	RCache.set_xform_view(mView);
	RCache.set_xform_project(mProject);
}

bool dxRenderDeviceRender::HWSupportsShaderYUV2RGB()
{
	u32		v_dev	= CAP_VERSION(HW.Caps.raster_major, HW.Caps.raster_minor);
	u32		v_need	= CAP_VERSION(2,0);
	return (v_dev>=v_need);
}

void  dxRenderDeviceRender::OnAssetsChanged()
{
	Resources->m_textures_description.UnLoad();
	Resources->m_textures_description.Load();
}
