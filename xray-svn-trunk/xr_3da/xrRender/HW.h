// HW.h: interface for the CHW class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HW_H__0E25CF4A_FFEC_11D3_B4E3_4854E82A090D__INCLUDED_)
#define AFX_HW_H__0E25CF4A_FFEC_11D3_B4E3_4854E82A090D__INCLUDED_
#pragma once

#include "hwcaps.h"
#ifndef _MAYA_EXPORT
#include "stats_manager.h"
#endif

enum ViewPort;

struct ViewPortHWResources
{
	ViewPortHWResources()
	{
		baseRT = nullptr;
		baseZB = nullptr;
	};

	ID3DRenderTargetView* baseRT;
	ID3DDepthStencilView* baseZB;
};

class CHW : public pureAppActivate, public pureAppDeactivate
{
//	Functions section
public:
	CHW();
	~CHW();

	void					CreateD3D				(HWND window_handle);
	void					DestroyD3D				();

	void					CreateDevice			(HWND window_handle, bool move_window);
	void					DestroyDevice			();

	void					CreateSwapChainDesc		(HWND window_handle, bool move_window);

	void					AdapterRoutine			();

	void					Reset					(HWND window_handle);
	void					ResetSwapChain			();

	void					LogHWInfo				(DXGI_ADAPTER_DESC& adapter_desc);

	void					selectResolution		(u32 &dwWidth, u32 &dwHeight, BOOL bWindowed);

	HRESULT					CreateDeviceDX11		();
	HRESULT					CreateSwapChain			(HWND window_handle);

	DXGI_FORMAT				selectDepthStencil		(DXGI_FORMAT);
	DXGI_FORMAT				selectFormat			(bool isForOutput);
	bool					isSupportingColorFormat	(DXGI_FORMAT format, D3D10_FORMAT_SUPPORT support);
	void					safeClearState			();

	void					updateWindowProps		(HWND window_handle);
	BOOL					support					(D3DFORMAT fmt, DWORD type, DWORD usage);

	void					SwitchViewPort			(ViewPort vp);

	void	Validate(void)	{};

//	Variables section
public:
	IDXGIDevice*			m_pDXGIDevice;
	IDXGIFactory*			m_pFactory;
	IDXGIAdapter*			m_pAdapter;	//	pD3D equivalent
	ID3D11Device*			pDevice;	
	ID3D11DeviceContext*    pContext;	
	IDXGISwapChain*         m_pSwapChain;
	ID3DRenderTargetView*	pBaseRT;	 // Keep in mind that in secondary viewports rendering pBaseRT does not point to swap chain buffer
	ID3DDepthStencilView*	pBaseZB;

	ViewPort				storedVP;
	xr_map<ViewPort, ViewPortHWResources> viewPortHWResList;

	CHWCaps					Caps;

	D3D_DRIVER_TYPE			m_DriverType;	//	DevT equivalent
	DXGI_SWAP_CHAIN_DESC	m_ChainDesc;	//	DevPP equivalent
	bool					m_bUsePerfhud;
	D3D_FEATURE_LEVEL		FeatureLevel;

	HRESULT					lastPresentStatus;

#ifndef _MAYA_EXPORT
	stats_manager			stats_manager;
#endif

	xr_map<IDXGIAdapter*, std::wstring> adaptersList_;

	void			UpdateViews();
	DXGI_RATIONAL	selectRefresh();

	virtual	void	OnAppActivate();
	virtual void	OnAppDeactivate();

private:
	bool					m_move_window;
};

extern ECORE_API CHW		HW;

#endif
