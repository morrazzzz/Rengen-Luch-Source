// dx10HW.cpp: implementation of the DX10 specialisation of CHW.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#pragma warning(disable:4995)
#include <d3dx9.h>
#pragma warning(default:4995)

#include <d3dx11.h>

#include "../xrRender/HW.h"
#include "../XR_IOConsole.h"
#include "../../Include/xrAPI/xrAPI.h"

#include "StateManager\dx10SamplerStateCache.h"
#include "StateManager\dx10StateCache.h"

void	fill_vid_mode_list			(CHW* _hw);
void	free_vid_mode_list			();

void	fill_render_mode_list		();
void	free_render_mode_list		();

extern ENGINE_API float psSVPImageSizeK;

#pragma comment (lib, "d3dx11.lib")

CHW HW;

CHW::CHW() : m_pAdapter(0), pDevice(NULL), m_move_window(true)
{
	Device.seqAppActivate.Add(this);
	Device.seqAppDeactivate.Add(this);

	storedVP = (ViewPort)0;
}

CHW::~CHW()
{
	Device.seqAppActivate.Remove(this);
	Device.seqAppDeactivate.Remove(this);
}

void CHW::CreateD3D(HWND window_handle)
{
	//DirectX Graphics Infrastructure (DXGI) device, adapter and factory access
	R_CHK(pDevice->QueryInterface(__uuidof(IDXGIDevice), (void **)&m_pDXGIDevice));
	R_CHK(m_pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void **)&m_pAdapter));
	R_CHK(m_pAdapter->GetParent(__uuidof(IDXGIFactory), (void **)&m_pFactory));

	AdapterRoutine();
}

void CHW::DestroyD3D()
{
	_SHOW_REF("refCount:m_pAdapter", m_pAdapter);

	_RELEASE(m_pFactory);
	_RELEASE(m_pAdapter);
	_RELEASE(m_pDXGIDevice);
}

void CHW::AdapterRoutine()
{
	// make list of adapters for possible usage in future
	{
		IDXGIAdapter* adapter = nullptr;
		m_bUsePerfhud = false;

		UINT i = 0;
		while (m_pFactory->EnumAdapters(i++, &adapter) != DXGI_ERROR_NOT_FOUND)
		{
			DXGI_ADAPTER_DESC desc;
			adapter->GetDesc(&desc);

			adaptersList_.insert(mk_pair(adapter, desc.Description));

			Msg("* Found Adapter: %S", desc.Description);
		}
	}

	if (strstr(Core.Params, "-nvidia_perf_hud")) // does it exist ????
	{
		// Look for 'NVIDIA NVPerfHUD' adapter. If it is present, override default settings

		for each (auto adapter in adaptersList_)
		{
			if (adapter.second == L"NVIDIA PerfHUD")
			{
				m_bUsePerfhud = true;
				m_pAdapter = adapter.first;

				break;
			}
		}
	}

	DXGI_ADAPTER_DESC desc;
	m_pAdapter->GetDesc(&desc);

	Msg("* Selected Adapter: %S", desc.Description);

	if (!m_pAdapter) // Should not happen, without errors in previous stages
	{
		HRESULT R = m_pFactory->EnumAdapters(0, &m_pAdapter);

		if (FAILED(R))
		{
			// Fatal error! Cannot get the primary graphics adapter AT STARTUP !!!
			Msg("Failed to initialize graphics hardware.\n"
				"Please try to restart the game.\n"
				"EnumAdapters returned 0x%08x", R
				);

			FlushLog();
			MessageBox(NULL, "Failed to initialize graphics hardware.\nPlease try to restart the game.", "Error!", MB_OK | MB_ICONERROR | MB_TOPMOST);

			TerminateProcess(GetCurrentProcess(), 0);
		}
	}
}

HRESULT CHW::CreateDeviceDX11()
{
	pDevice = NULL;
	pContext = NULL;

	m_DriverType = Caps.bForceGPU_REF ? D3D_DRIVER_TYPE_REFERENCE : D3D_DRIVER_TYPE_HARDWARE;

	HRESULT R;
	UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL pFeatureLevels[] = {
		D3D_FEATURE_LEVEL_11_0,
	};

	R = D3D11CreateDevice(
		NULL,
		m_DriverType,
		NULL,
		createDeviceFlags,
		pFeatureLevels,
		sizeof(pFeatureLevels) / sizeof(pFeatureLevels[0]),
		D3D11_SDK_VERSION,
		&pDevice,
		&FeatureLevel,
		&pContext
	);

#ifdef DEBUG
	if (R != S_OK) // try to disable debug, may be then we can start. If yes, than user probably needs to install Win SDK
	{
		Msg("! Failed to create device and swap chain. Trying to disable debug flag and create it again");

		createDeviceFlags &= ~D3D11_CREATE_DEVICE_DEBUG;

		R = D3D11CreateDevice(
			NULL,
			m_DriverType,
			NULL,
			createDeviceFlags,
			pFeatureLevels,
			sizeof(pFeatureLevels) / sizeof(pFeatureLevels[0]),
			D3D11_SDK_VERSION,
			&pDevice,
			&FeatureLevel,
			&pContext
			);

		if (R == S_OK)
		{
			string1024 str;

			xr_sprintf(str, "- The first attempt to create device and swap chain was not successfull, but an attempt to create it without debug flag is successfull.\nThis means you should try to install Windows SDK 8.1 and DirectX 11 SDK to run DXGI in debug mode");

			Msg("%s", str);

			MessageBox(NULL, str, "Error!", MB_OK | MB_ICONERROR | MB_TOPMOST);
		}

	}
#endif

	if (R == S_OK)
	{
		Msg("- Device and swap chain are created succesfully. DXGI Debug is %s ", createDeviceFlags&D3D11_CREATE_DEVICE_DEBUG ? "enabled" : "disabled");
	}
	else if FAILED(R)
	{
		Msg("!Failed to initialize graphics hardware.\nPlease try to restart the game.\nCreateDevice returned 0x%08x", R);

		FlushLog();

		MessageBox(NULL, "Failed to initialize graphics hardware.\nPlease try to restart the game.", "Error!", MB_OK | MB_ICONERROR | MB_TOPMOST);

		TerminateProcess(GetCurrentProcess(), 0);
	}

	return R;
}

HRESULT CHW::CreateSwapChain(HWND window_handle)
{
	HRESULT R;

	R = m_pFactory->CreateSwapChain(pDevice, &m_ChainDesc, &m_pSwapChain);

	if (!strstr(Core.Params, "-allow_alt_enter")) // should not allow alt enter, since we should process swap chain reset in this case
		m_pFactory->MakeWindowAssociation(window_handle, DXGI_MWA_NO_ALT_ENTER); // works only if placed after swap chain creation

	return R;
}

void CHW::CreateDevice(HWND window_handle, bool move_window)
{
	// Create DX11 Device
	CreateDeviceDX11();

	// Select dxgi adapter, device and factory using DX11Device instance
	CreateD3D(window_handle);

	// Set up the presentation parameters (SWAP CHAIN)
	CreateSwapChainDesc(window_handle, move_window);

	// Create swap chain
	CreateSwapChain(window_handle);

	// Create backbuffer and depth-stencil views here
	UpdateViews();

	// Apply specific to windowed mode flags
	updateWindowProps(window_handle);

	DXGI_ADAPTER_DESC adapter_desc;

	R_CHK(m_pAdapter->GetDesc(&adapter_desc));

	Caps.id_vendor = adapter_desc.VendorId;
	Caps.id_device = adapter_desc.DeviceId;

	// Display the name of video board
	LogHWInfo(adapter_desc);
}

void CHW::DestroyDevice()
{
	//	Destroy state managers
	StateManager.Reset();
	RSManager.ClearStateArray();
	DSSManager.ClearStateArray();
	BSManager.ClearStateArray();
	SSManager.ClearStateArray();

	for (auto it = viewPortHWResList.begin(); it != viewPortHWResList.end(); ++it)
	{
		_SHOW_REF("refCount:pBaseZB", it->second.baseZB);
		_SHOW_REF("refCount:pBaseRT", it->second.baseRT);

		_RELEASE(it->second.baseZB);
		_RELEASE(it->second.baseRT);
	}

	//	Must switch to windowed mode to release swap chain
	if (!m_ChainDesc.Windowed)
		m_pSwapChain->SetFullscreenState(FALSE, NULL);

	_SHOW_REF				("refCount:m_pSwapChain", m_pSwapChain);
	_RELEASE				(m_pSwapChain);
	_RELEASE				(pContext);
	_SHOW_REF				("DeviceREF:", HW.pDevice);
	_RELEASE				(HW.pDevice);


	DestroyD3D();

	free_vid_mode_list();
}

void CHW::CreateSwapChainDesc(HWND window_handle, bool move_window)
{
	m_move_window = move_window;
	BOOL windowed = !psDeviceFlags.is(rsFullscreen);

	ZeroMemory(&m_ChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));

	selectResolution(m_ChainDesc.BufferDesc.Width, m_ChainDesc.BufferDesc.Height, windowed);

	UINT buffers_count = windowed ? 1 : (UINT)_max(2, ps_r3_backbuffers_count);

	m_ChainDesc.BufferCount = buffers_count;
	m_ChainDesc.BufferDesc.Format = selectFormat(true);
	m_ChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	m_ChainDesc.OutputWindow = window_handle;
	m_ChainDesc.SampleDesc.Count = 1;
	m_ChainDesc.SampleDesc.Quality = 0;
	m_ChainDesc.Windowed = windowed;
	m_ChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	if (buffers_count > 1)
		m_ChainDesc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;
	else
		m_ChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	m_ChainDesc.BufferDesc.RefreshRate = selectRefresh();
}

void CHW::Reset(HWND window_handle)
{
	ResetSwapChain();
	UpdateViews();
	updateWindowProps(window_handle);
}

void CHW::ResetSwapChain()
{
	BOOL windowed = !psDeviceFlags.is(rsFullscreen);
	m_ChainDesc.Windowed = windowed;

	DXGI_MODE_DESC& buf_desc = m_ChainDesc.BufferDesc;

	selectResolution(buf_desc.Width, buf_desc.Height, windowed);
	buf_desc.RefreshRate = selectRefresh();

	for (auto it = viewPortHWResList.begin(); it != viewPortHWResList.end(); ++it)
	{
		_SHOW_REF("refCount:pBaseZB", it->second.baseZB);
		_SHOW_REF("refCount:pBaseRT", it->second.baseRT);

		_RELEASE(it->second.baseZB);
		_RELEASE(it->second.baseRT);
	}

	m_pSwapChain->SetFullscreenState(!windowed, NULL);

	CHK_DX(m_pSwapChain->ResizeTarget(&buf_desc));
	CHK_DX(m_pSwapChain->ResizeBuffers(m_ChainDesc.BufferCount, buf_desc.Width, buf_desc.Height, buf_desc.Format, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
}

void CHW::LogHWInfo(DXGI_ADAPTER_DESC& adapter_desc)
{
	Msg(LINE_SPACER);
	Msg("* GPU [vendor:%X]-[device:%X]: %S", adapter_desc.VendorId, adapter_desc.DeviceId, adapter_desc.Description);
	Msg("* GPU driver type: %s", (m_DriverType == D3D_DRIVER_TYPE_HARDWARE) ? "hardware" : "software reference");

	Caps.id_vendor = adapter_desc.VendorId;
	Caps.id_device = adapter_desc.DeviceId;

	_SHOW_REF("* CREATE: DeviceREF:", HW.pDevice);

	size_t memory = adapter_desc.DedicatedVideoMemory;

	Msg("* Available GPU memory: %d M", memory / (1024 * 1024));

	//Msg("* DDI-level: %2.1f", float(D3DXGetDriverLevel(pDevice)) / 100.f);

	switch (FeatureLevel)
	{
	case D3D_FEATURE_LEVEL_10_0:
		Msg("* Direct3D feature level used: 10.0");
		break;
	case D3D_FEATURE_LEVEL_10_1:
		Msg("* Direct3D feature level used: 10.1");
		break;
	case D3D_FEATURE_LEVEL_11_0:
		Msg("* Direct3D feature level used: 11.0");
		break;
	}
	Msg(LINE_SPACER);
}

void CHW::safeClearState()
{
	if (S_OK == lastPresentStatus)
		pContext->ClearState();
}

bool CHW::isSupportingColorFormat(DXGI_FORMAT format, D3D10_FORMAT_SUPPORT support)
{
	UINT values = 0;

	if (!pDevice)
	  return false;

	if (FAILED(pDevice->CheckFormatSupport(format, &values)))
		return false;

	if (values && support)
		return true;

	return false;
}

DXGI_FORMAT CHW::selectFormat(bool isForOutput)
{
	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;

	if (isForOutput)
		format = DXGI_FORMAT_R8G8B8A8_UNORM;
	else
		format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	return format;
}

DXGI_FORMAT CHW::selectDepthStencil(DXGI_FORMAT fTarget)
{
	// this is UNUSED!
	return DXGI_FORMAT_D24_UNORM_S8_UINT;  // D3DFMT_D24S8
}

void CHW::selectResolution(u32 &dwWidth, u32 &dwHeight, BOOL bWindowed)
{
	fill_vid_mode_list(this);

	if(bWindowed)
	{
		dwWidth		= psCurrentVidMode[0];
		dwHeight	= psCurrentVidMode[1];
	}
	else //check
	{
		string64 buff;
		xr_sprintf(buff, sizeof(buff), "%dx%d", psCurrentVidMode[0], psCurrentVidMode[1]);

		if (_ParseItem(buff, vid_mode_token) == u32(-1)) //not found
		{
			//select safe
			xr_sprintf(buff, sizeof(buff), "vid_mode %s", vid_mode_token[0].name);
			Console->Execute(buff);
		}

		dwWidth = psCurrentVidMode[0];
		dwHeight = psCurrentVidMode[1];
	}
}

DXGI_RATIONAL CHW::selectRefresh()
{
	// utak3r: when resizing target, let DXGI calculate the refresh rate for itself.
	// This is very important for performance, that this value is correct.
	DXGI_RATIONAL refresh;
	refresh.Numerator = 0;
	refresh.Denominator = 1;

	return refresh;
}

void CHW::OnAppActivate()
{
	if (m_pSwapChain && !m_ChainDesc.Windowed)
	{
		ShowWindow( m_ChainDesc.OutputWindow, SW_RESTORE);
		m_pSwapChain->SetFullscreenState(TRUE, NULL);

		Device.m_pRender->updateGamma();
	}
}

void CHW::OnAppDeactivate()
{
	if (m_pSwapChain && !m_ChainDesc.Windowed)
	{
		m_pSwapChain->SetFullscreenState(FALSE, NULL);
		ShowWindow(m_ChainDesc.OutputWindow, SW_MINIMIZE);
	}
}

BOOL CHW::support(D3DFORMAT fmt, DWORD type, DWORD usage)
{
	//	TODO: DX10: implement stub for this code.

	VERIFY(!"Implement CHW::support");

	/*
	HRESULT hr		= pD3D->CheckDeviceFormat(DevAdapter,DevT,Caps.fTarget,usage,(D3DRESOURCETYPE)type,fmt);
	if (FAILED(hr))	return FALSE;
	else			return TRUE;
	*/

	return TRUE;
}

void CHW::SwitchViewPort(ViewPort vp)
{
	if (storedVP == vp && pBaseRT)
		return;

	storedVP = vp;

	auto it = viewPortHWResList.find(vp);

	if (it == viewPortHWResList.end())
		it = viewPortHWResList.find(MAIN_VIEWPORT);

	pBaseRT = it->second.baseRT;
	pBaseZB = it->second.baseZB;
}

void CHW::updateWindowProps(HWND window_handle)
{
	// utak3r: with DX10 we no longer want to play with window's styles and flags,
	// DXGI will do this for its own for us, if we're in a fullscreen mode.
	// Thus, we will check if the current mode is a windowed mode
	// and only then perform our actions.

	BOOL bWindowed = !psDeviceFlags.is	(rsFullscreen);

	if (bWindowed)
	{
		if (m_move_window)
		{
			LONG dwWindowStyle = 0;

			if (strstr(Core.Params, "-no_dialog_header"))
				SetWindowLong(window_handle, GWL_STYLE, dwWindowStyle = (WS_BORDER | WS_VISIBLE));
			else
				SetWindowLong(window_handle, GWL_STYLE, dwWindowStyle = (WS_BORDER | WS_DLGFRAME | WS_VISIBLE | WS_SYSMENU | WS_MINIMIZEBOX));

			RECT m_rcWindowBounds;
			BOOL bCenter = TRUE;

			if (strstr(Core.Params, "-no_center_screen"))
				bCenter = FALSE;

			if (bCenter)
			{
				RECT DesktopRect;

				GetClientRect(GetDesktopWindow(), &DesktopRect);

				SetRect(&m_rcWindowBounds,
					(DesktopRect.right - m_ChainDesc.BufferDesc.Width) / 2,
					(DesktopRect.bottom - m_ChainDesc.BufferDesc.Height) / 2,
					(DesktopRect.right + m_ChainDesc.BufferDesc.Width) / 2,
					(DesktopRect.bottom + m_ChainDesc.BufferDesc.Height) / 2);
			}
			else
			{
				SetRect(&m_rcWindowBounds,
					0,
					0,
					m_ChainDesc.BufferDesc.Width,
					m_ChainDesc.BufferDesc.Height);
			};

			AdjustWindowRect(&m_rcWindowBounds, dwWindowStyle, FALSE);

			SetWindowPos(window_handle,
				HWND_NOTOPMOST,
				m_rcWindowBounds.left,
				m_rcWindowBounds.top,
				(m_rcWindowBounds.right - m_rcWindowBounds.left),
				(m_rcWindowBounds.bottom - m_rcWindowBounds.top),
				SWP_SHOWWINDOW | SWP_NOCOPYBITS | SWP_DRAWFRAME);
		}
	}

	ShowCursor(FALSE);

	SetForegroundWindow(window_handle);
}

struct _uniq_mode
{
	_uniq_mode(LPCSTR v):_val(v){}
	LPCSTR _val;

	bool operator() (LPCSTR _other)
	{
		return !stricmp(_val,_other);
	}
};

void free_vid_mode_list()
{
	for(int i = 0; vid_mode_token[i].name; i++)
	{
		xr_free(vid_mode_token[i].name);
	}

	xr_free(vid_mode_token);

	vid_mode_token = NULL;
}

void fill_vid_mode_list(CHW* _hw)
{
	if(vid_mode_token != NULL)
		return;

	xr_vector<LPCSTR> _tmp;
	xr_vector<DXGI_MODE_DESC> modes;

	IDXGIOutput* pOutput;
	//_hw->m_pSwapChain->GetContainingOutput(&pOutput);
	_hw->m_pAdapter->EnumOutputs(0, &pOutput);

	VERIFY(pOutput);

	UINT num = 0;
	DXGI_FORMAT format = _hw->selectFormat(true);
	UINT flags = 0;

	// Get the number of display modes available
	pOutput->GetDisplayModeList(format, flags, &num, 0);

	// Get the list of display modes
	modes.resize(num);

	pOutput->GetDisplayModeList(format, flags, &num, &modes.front());

	_RELEASE(pOutput);

	for (u32 i = 0; i < num; ++i)
	{
		DXGI_MODE_DESC &desc = modes[i];
		string32 str;

		if(desc.Width < 800)
			continue;

		xr_sprintf(str, sizeof(str), "%dx%d", desc.Width, desc.Height);

		if(_tmp.end() != std::find_if(_tmp.begin(), _tmp.end(), _uniq_mode(str)))
			continue;

		_tmp.push_back(NULL);
		_tmp.back() = xr_strdup(str);
	}
	
//	_tmp.push_back				(NULL);
//	_tmp.back()					= xr_strdup("1024x768");

	u32 _cnt = _tmp.size()+1;

	vid_mode_token = xr_alloc<xr_token>(_cnt);

	vid_mode_token[_cnt-1].id = -1;
	vid_mode_token[_cnt-1].name = NULL;

#ifdef DEBUG
	Msg("Available video modes[%d]:",_tmp.size());
#endif
	for(u32 i = 0; i < _tmp.size(); ++i)
	{
		vid_mode_token[i].id = i;
		vid_mode_token[i].name = _tmp[i];

#ifdef DEBUG
		Msg("[%s]", _tmp[i]);
#endif
	}
}

void CHW::UpdateViews()
{
	/*
	BOOL currentFullscreen;
	m_pSwapChain->GetFullscreenState(&currentFullscreen, NULL);
	DXGI_MODE_DESC &desc = m_ChainDesc.BufferDesc;
	if (currentFullscreen == psDeviceFlags.is(rsFullscreen))
		{
		m_pSwapChain->SetFullscreenState(!psDeviceFlags.is(rsFullscreen), NULL);
		m_pSwapChain->ResizeTarget(&desc);
		}
	*/

	DXGI_SWAP_CHAIN_DESC& sd = m_ChainDesc;
	HRESULT R;

	// Set up svp image size
	Device.m_SecondViewport.screenWidth = u32((sd.BufferDesc.Width / 32) * psSVPImageSizeK) * 32;
	Device.m_SecondViewport.screenHeight = u32((sd.BufferDesc.Height / 32) * psSVPImageSizeK) * 32;

	// Create a render target views
	viewPortHWResList.insert(mk_pair(MAIN_VIEWPORT, ViewPortHWResources()));
	viewPortHWResList.insert(mk_pair(SECONDARY_WEAPON_SCOPE, ViewPortHWResources()));

	ID3DTexture2D* temp1;
	ID3DTexture2D* temp2;

	R = m_pSwapChain->GetBuffer(0, __uuidof(ID3DTexture2D), reinterpret_cast<void**>(&temp1));
	R_CHK2(R, "!Erroneous buffer result");

	D3D_TEXTURE2D_DESC desc;
	temp1->GetDesc(&desc);
	desc.Width = Device.m_SecondViewport.screenWidth;
	desc.Height = Device.m_SecondViewport.screenHeight;

	R = pDevice->CreateTexture2D(&desc, NULL, &temp2);
	R_CHK(R);

	R = pDevice->CreateRenderTargetView(temp1, NULL, &viewPortHWResList.at(MAIN_VIEWPORT).baseRT);
	R_CHK(R);

	R = pDevice->CreateRenderTargetView(temp2, NULL, &viewPortHWResList.at(SECONDARY_WEAPON_SCOPE).baseRT);
	R_CHK(R);

	temp1->Release();
	temp2->Release();

	//  Create Depth/stencil buffer
	ID3DTexture2D* depth_stencil = NULL;
	D3D_TEXTURE2D_DESC descDepth;

	descDepth.Width = sd.BufferDesc.Width;
	descDepth.Height = sd.BufferDesc.Height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = selectFormat(false);
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D_USAGE_DEFAULT;
	descDepth.BindFlags = D3D_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;

	R = pDevice->CreateTexture2D(&descDepth, NULL, &depth_stencil);
	R_CHK(R);

	R = pDevice->CreateDepthStencilView(depth_stencil, NULL, &viewPortHWResList.at(MAIN_VIEWPORT).baseZB);
	R_CHK(R);

	depth_stencil->Release();

	descDepth.Width = Device.m_SecondViewport.screenWidth;
	descDepth.Height = Device.m_SecondViewport.screenHeight;

	R = pDevice->CreateTexture2D(&descDepth, NULL, &depth_stencil);
	R_CHK(R);

	R = pDevice->CreateDepthStencilView(depth_stencil, NULL, &viewPortHWResList.at(SECONDARY_WEAPON_SCOPE).baseZB);
	R_CHK(R);

	depth_stencil->Release();

	// first init
	pBaseRT = viewPortHWResList.at(MAIN_VIEWPORT).baseRT;
	pBaseZB = viewPortHWResList.at(MAIN_VIEWPORT).baseZB;
}

