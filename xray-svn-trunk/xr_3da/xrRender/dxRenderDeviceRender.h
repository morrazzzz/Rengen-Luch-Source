
#pragma once

#define DEV dxRenderDeviceRender::Instance().Resources

#include "..\..\Include\xrRender\RenderDeviceRender.h"
#include "xr_effgamma.h"

class CResourceManager;
enum ViewPort;

class dxRenderDeviceRender : public IRenderDeviceRender
{
public:
	static dxRenderDeviceRender& Instance() {  return *((dxRenderDeviceRender*)(&*Device.m_pRender));}

	dxRenderDeviceRender();

	virtual void	Copy(IRenderDeviceRender &_in);

	//	Gamma correction functions
	virtual void	setGamma(float fGamma);
	virtual void	setBrightness(float fGamma);
	virtual void	setContrast(float fGamma);
	virtual void	updateGamma();

	//void	gammaGenLUT(D3DGAMMARAMP &G) {m_Gamma.GenLUT(G);}

	//	Destroy
	virtual void	OnDeviceDestroy( BOOL bKeepTextures);
	virtual void	ValidateHW();
	virtual void	DestroyHW();
	virtual void	Reset( HWND hWnd, u32 &dwWidth, u32 &dwHeight, float &fWidth_2, float &fHeight_2);
	//	Init
	virtual void	SetupStates();
	virtual void	OnDeviceCreate(LPCSTR shName);
	virtual void	Create( HWND hWnd, u32 &dwWidth, u32 &dwHeight, float &fWidth_2, float &fHeight_2, bool);
	virtual void	SetupGPU( BOOL bForceGPU_SW, BOOL bForceGPU_NonPure, BOOL bForceGPU_REF);
	//	Overdraw
	virtual void	overdrawBegin();
	virtual void	overdrawEnd();

	//	Resources control
	virtual void	DeferredLoad(BOOL E);
	virtual void	ResourcesDeferredUpload(BOOL multithreaded);
	virtual void	SyncTexturesLoadingProcess();
	virtual void	ResourcesGetMemoryUsage(u32& m_base, u32& c_base, u32& m_lmaps, u32& c_lmaps);
	virtual void	ResourcesDestroyNecessaryTextures();
	virtual void	ResourcesStoreNecessaryTextures();
	virtual void	ResourcesDumpMemoryUsage();
	virtual void	RenderPrefetchUITextures();

	//	HWSupport
	virtual bool	HWSupportsShaderYUV2RGB();

	//	Device state
	virtual DeviceState GetDeviceState();
	virtual BOOL	GetForceGPU_REF();
	virtual u32		GetCacheStatPolys();
	virtual void	RenderBegin(ViewPort vp);
	virtual void	Clear();
	virtual void	RenderFinish(ViewPort vp);
	virtual void	ClearTarget();
	virtual void	SwitchViewPortRTZB(ViewPort vp);
	virtual void	SetCacheXform(Fmatrix &mView, Fmatrix &mProject);
	virtual void	OnAssetsChanged();
	virtual void	OnNewFrame();

	virtual BOOL	Render_test_hw()	{return xrRender_test_hw();}

public:
	CResourceManager*	Resources;
	ref_shader			m_WireShader;
	ref_shader			m_SelectionShader;

private:

	CGammaControl		m_Gamma;
};
