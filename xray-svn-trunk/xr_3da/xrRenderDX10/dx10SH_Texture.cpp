#include "stdafx.h"
#pragma hdrstop

#include "../xrRender/ResourceManager.h"

#include "../render.h"
#include "../tntQAVI.h"
#include "../xrTheora_Surface.h"
#include "../xrRender/dxRenderDeviceRender.h"
#include "StateManager/dx10ShaderResourceStateCache.h"

#define PRIORITY_HIGH 12
#define PRIORITY_NORMAL 8
#define PRIORITY_LOW 4

ENGINE_API BOOL mt_dynTextureLoad_;

void resptrcode_texture::create(LPCSTR _name, bool mt_anyway)
{
	_set(DEV->_CreateTexture(_name));
}

CTexture::CTexture()
{
	DEV->createdTexturesCnt_++;

	pSurface			= nullptr;
	m_pSRView			= nullptr;
	pAVI				= nullptr;
	pTheora				= nullptr;
	desc_cache			= 0;
	seqMSPF				= 0;

	flags.MemoryUsage	= 0;
	flags.bLoaded		= false;
	flags.bUser			= false;
	flags.seqCycles		= FALSE;
	flags.bLoadedAsStaging = FALSE;

	m_material			= 1.0f;
	bind				= fastdelegate::FastDelegate1<u32>(this, &CTexture::apply_load);

	isProcessingMT_		= false;

	mtLoadedViewRes_	= nullptr;
	mtLoadedSurface_	= nullptr;

	readyToDestroy_		= false;
	alreadyInDestroyQueue_ = false;
}

CTexture::~CTexture()
{
	R_ASSERT2(IsReadyToDestroy(), make_string("Textures should be deleted through recource manager"));
	 
	UnloadTexture();

	DEV->deletedTexturesCnt_++;
}

void CTexture::PreDelete()
{
	// release external reference
	DEV->_DeleteTextureRef(this); // important to be called when texture marked to be deleted, other wise crashes some code
}

void CTexture::surface_set(ID3DBaseTexture* surf)
{
	if (surf)
		surf->AddRef();

	if (pSurface)
		_RELEASE(pSurface);

	if (m_pSRView)
		_RELEASE(m_pSRView);

	pSurface = surf;

	desc_update();

	m_pSRView = CreateShaderRes(pSurface);
}

void CTexture::SurfaceSetRT(ID3DBaseTexture* surf, ID3DShaderResourceView* sh_res_view)
{
	pSurface = surf;
	m_pSRView = sh_res_view;
}

ID3DShaderResourceView* CTexture::CreateShaderRes(ID3DBaseTexture * surf)
{
	pSurface = surf;

	desc_update();

	if (surf)
	{
		ID3DShaderResourceView* sh_res_view = nullptr;

		D3D_RESOURCE_DIMENSION type;
		surf->GetType(&type);

		if (D3D_RESOURCE_DIMENSION_TEXTURE2D == type)
		{
			D3D_SHADER_RESOURCE_VIEW_DESC ViewDesc;

			if (desc.MiscFlags&D3D_RESOURCE_MISC_TEXTURECUBE)
			{
				ViewDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURECUBE;
				ViewDesc.TextureCube.MostDetailedMip = 0;
				ViewDesc.TextureCube.MipLevels = desc.MipLevels;
			}
			else
			{
				if (desc.SampleDesc.Count <= 1)
				{
					ViewDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
					ViewDesc.Texture2D.MostDetailedMip = 0;
					ViewDesc.Texture2D.MipLevels = desc.MipLevels;
				}
				else
				{
					ViewDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2DMS;
					ViewDesc.Texture2D.MostDetailedMip = 0;
					ViewDesc.Texture2D.MipLevels = desc.MipLevels;
				}
			}

			ViewDesc.Format = DXGI_FORMAT_UNKNOWN;

			switch (desc.Format)
			{
			case DXGI_FORMAT_R24G8_TYPELESS:
				ViewDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
				break;
			case DXGI_FORMAT_R32_TYPELESS:
				ViewDesc.Format = DXGI_FORMAT_R32_FLOAT;
				break;
			}

			if ((desc.SampleDesc.Count <= 1) || (ViewDesc.Format != DXGI_FORMAT_R24_UNORM_X8_TYPELESS))
			{
				R_CHK(HW.pDevice->CreateShaderResourceView(surf, &ViewDesc, &sh_res_view));
				R_ASSERT(sh_res_view);
			}
		}
		else
			R_CHK(HW.pDevice->CreateShaderResourceView(surf, nullptr, &sh_res_view));

		return sh_res_view;
	}

	return nullptr;
}

ID3DBaseTexture* CTexture::surface_get()
{
	R_ASSERT2(this, "Hey, Where is a texture object?");

	if (flags.bLoadedAsStaging)
		ProcessStaging();

	if (pSurface)
		pSurface->AddRef();

	return pSurface;
}

void CTexture::surface_null()
{
	pSurface = nullptr;
	m_pSRView = nullptr;
}

void CTexture::PostLoad()
{
	if (pTheora)
		bind = fastdelegate::FastDelegate1<u32>(this, &CTexture::apply_theora);
	else if (pAVI)
		bind = fastdelegate::FastDelegate1<u32>(this, &CTexture::apply_avi);
	else if (!seqDATA.empty())
		bind = fastdelegate::FastDelegate1<u32>(this, &CTexture::apply_seq);
	else
		bind = fastdelegate::FastDelegate1<u32>(this, &CTexture::apply_normal);
}

void CTexture::apply_load(u32 dwStage)
{
	if (!flags.bLoaded)
		LoadTexture(false);
	else
		PostLoad();

	bind(dwStage);
};

void CTexture::ProcessStaging()
{
	R_ASSERT(!isProcessingMT_);
	VERIFY(pSurface);
	VERIFY(flags.bLoadedAsStaging);

	ID3DBaseTexture* pTargetSurface = 0;

	D3D_RESOURCE_DIMENSION type;
	pSurface->GetType(&type);

	switch (type)
	{
	case D3D_RESOURCE_DIMENSION_TEXTURE2D:
		{
			ID3DTexture2D* T = (ID3DTexture2D*)pSurface;
			D3D_TEXTURE2D_DESC TexDesc;

			T->GetDesc(&TexDesc);

			TexDesc.Usage = D3D_USAGE_DEFAULT;
			TexDesc.BindFlags = D3D_BIND_SHADER_RESOURCE;
			TexDesc.CPUAccessFlags = 0;

			T = 0;

			CHK_DX(HW.pDevice->CreateTexture2D (&TexDesc,	// Texture desc
												nullptr,	// Initial data
												&T));		// [out] Texture

			pTargetSurface = T;
		}
		break;
	case D3D_RESOURCE_DIMENSION_TEXTURE3D:
		{
			ID3DTexture3D* T = (ID3DTexture3D*)pSurface;
			D3D_TEXTURE3D_DESC TexDesc;

			T->GetDesc(&TexDesc);

			TexDesc.Usage = D3D_USAGE_DEFAULT;
			TexDesc.BindFlags = D3D_BIND_SHADER_RESOURCE;
			TexDesc.CPUAccessFlags = 0;

			T = 0;

			CHK_DX(HW.pDevice->CreateTexture3D( &TexDesc,	// Texture desc
												nullptr,	// Initial data
												&T));		// [out] Texture

			pTargetSurface = T;
		}
		break;
	default:
		VERIFY(!"CTexture::ProcessStaging unsupported dimensions.");
	}

	HW.pContext->CopyResource(pTargetSurface, pSurface);

	flags.bLoadedAsStaging = FALSE;

	//	Check if texture was not copied _before_ it was converted.
	ULONG RefCnt = pSurface->Release();
	pSurface = 0;

	VERIFY(!RefCnt);

	surface_set(pTargetSurface);

	_RELEASE(pTargetSurface);

	VERIFY(pSurface);
}

void CTexture::Apply(u32 dwStage)
{
	if (GetIsProcessingMT())
		return;

	if (flags.bLoadedAsStaging)
		ProcessStaging();

	if (dwStage < rstVertex) // Pixel shader stage resources
	{
		//HW.pDevice->PSSetShaderResources(dwStage, 1, &m_pSRView);
		SRVSManager.SetPSResource(dwStage, m_pSRView);
	}
	else if (dwStage < rstGeometry)	//	Vertex shader stage resources
	{
		//HW.pDevice->VSSetShaderResources(dwStage-rstVertex, 1, &m_pSRView);
		SRVSManager.SetVSResource(dwStage-rstVertex, m_pSRView);
	}
	else if (dwStage < rstHull)	//	Geometry shader stage resources
	{
		//HW.pDevice->GSSetShaderResources(dwStage-rstGeometry, 1, &m_pSRView);
		SRVSManager.SetGSResource(dwStage-rstGeometry, m_pSRView);
	}
	else if (dwStage < rstDomain)	//	Geometry shader stage resources
	{
		SRVSManager.SetHSResource(dwStage-rstHull, m_pSRView);
	}
	else if (dwStage < rstCompute)	//	Geometry shader stage resources
	{
		SRVSManager.SetDSResource(dwStage-rstDomain, m_pSRView);
	}
	else if (dwStage < rstInvalid)	//	Geometry shader stage resources
	{
		SRVSManager.SetCSResource(dwStage-rstCompute, m_pSRView);
	}
	else
		VERIFY("Invalid stage");
}

void CTexture::apply_theora(u32 dwStage)
{
	if (pTheora->Update(m_play_time != 0xFFFFFFFF ? m_play_time : EngineTimeContinual()))
	{
		D3D_RESOURCE_DIMENSION type;
		pSurface->GetType(&type);
		R_ASSERT(D3D_RESOURCE_DIMENSION_TEXTURE2D == type);

		ID3DTexture2D* T2D = (ID3DTexture2D*)pSurface;
		D3D_MAPPED_TEXTURE2D mapData;

		RECT rect;

		rect.left			= 0;
		rect.top			= 0;
		rect.right			= pTheora->Width(true);
		rect.bottom			= pTheora->Height(true);

		u32 _w				= pTheora->Width(false);

		//R_CHK				(T2D->LockRect(0,&R,&rect,0));
		R_CHK				(HW.pContext->Map(T2D, 0, D3D_MAP_WRITE_DISCARD, 0, &mapData));

		//R_ASSERT			(R.Pitch == int(pTheora->Width(false)*4));
		R_ASSERT			(mapData.RowPitch == int(pTheora->Width(false) * 4));

		int _pos			= 0;
		pTheora->DecompressFrame((u32*)mapData.pData, _w - rect.right, _pos);

		VERIFY				(u32(_pos) == rect.bottom*_w);

		HW.pContext->Unmap(T2D, 0);
	}

	Apply(dwStage);
};

void CTexture::apply_avi(u32 dwStage)	
{
	if (pAVI->NeedUpdate())
	{
		D3D_RESOURCE_DIMENSION type;
		pSurface->GetType(&type);

		R_ASSERT(D3D_RESOURCE_DIMENSION_TEXTURE2D == type);

		ID3DTexture2D* T2D = (ID3DTexture2D*)pSurface;

		D3D_MAPPED_TEXTURE2D mapData;

		// AVI
		R_CHK(HW.pContext->Map(T2D, 0, D3D_MAP_WRITE_DISCARD, 0, &mapData));

		R_ASSERT(mapData.RowPitch == int(pAVI->m_dwWidth * 4));

		BYTE* ptr; pAVI->GetFrame(&ptr);

		CopyMemory(mapData.pData,ptr,pAVI->m_dwWidth*pAVI->m_dwHeight * 4);

		HW.pContext->Unmap(T2D, 0);
	}

	Apply(dwStage);
};

void CTexture::apply_seq(u32 dwStage)
{
	// SEQ
	u32	frame = EngineTimeContinual() / seqMSPF; //EngineTimeU()
	u32	frame_data = seqDATA.size();

	if (flags.seqCycles)
	{
		u32	frame_id = frame % (frame_data * 2);

		if (frame_id >= frame_data)
			frame_id = (frame_data - 1) - (frame_id%frame_data);

		pSurface = seqDATA[frame_id];
		m_pSRView = m_seqSRView[frame_id];
	}
	else
	{
		u32	frame_id = frame%frame_data;
		pSurface = seqDATA[frame_id];
		m_pSRView = m_seqSRView[frame_id];
	}

	Apply(dwStage);
};

void CTexture::apply_normal(u32 dwStage)
{
	Apply(dwStage);
};

void CTexture::Preload()
{
	m_bumpmap = DEV->m_textures_description.GetBumpName(cName);
	m_material = DEV->m_textures_description.GetMaterial(cName);
}

void CTexture::LoadTexture(bool already_in_mt, bool mt_anyway)
{
	loadLock_.Enter();

	flags.bLoaded = true;
	desc_cache = 0;

	if (pSurface)
	{
		loadLock_.Leave();

		return;
	}

	flags.bUser = false;
	flags.MemoryUsage = 0;

	if (!cName) //Prevent crash when cName became null for some reason
	{
		Msg("!Texture got lost in memory, skipping its loading");

		loadLock_.Leave();

		return; 
	}

	if (0 == stricmp(*cName, "$null"))
	{
		loadLock_.Leave();

		return;
	}
	if (0 != strstr(*cName, "$user$"))
	{
		flags.bUser	= true;

		loadLock_.Leave();

		return;
	}

	Preload();

	bool bCreateView = true;

	// Check for OGM
	string_path fn;

	if (FS.exist(fn, "$game_textures$", *cName, ".ogm"))
	{
		// AVI
		pTheora = xr_new <CTheoraSurface>();

		m_play_time = 0xFFFFFFFF;

		BOOL theora_loaded = pTheora->Load(fn);

		if (!theora_loaded)
		{
			xr_delete(pTheora);

			FATAL("Can't open video stream");
		}
		else
		{
			flags.MemoryUsage = pTheora->Width(true)*pTheora->Height(true) * 4;

			pTheora->Play(TRUE, EngineTimeContinual());

			// Now create texture
			ID3DTexture2D*	pTexture = 0;
			u32 _w = pTheora->Width(false);
			u32 _h = pTheora->Height(false);

			D3D_TEXTURE2D_DESC	t_desc;
			t_desc.Width = _w;
			t_desc.Height = _h;
			t_desc.MipLevels = 1;
			t_desc.ArraySize = 1;
			t_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			t_desc.SampleDesc.Count = 1;
			t_desc.SampleDesc.Quality = 0;
			t_desc.Usage = D3D_USAGE_DYNAMIC;
			t_desc.BindFlags = D3D_BIND_SHADER_RESOURCE;
			t_desc.CPUAccessFlags = D3D_CPU_ACCESS_WRITE;
			t_desc.MiscFlags = 0;

			HRESULT hrr = HW.pDevice->CreateTexture2D(&t_desc, 0, &pTexture); //Microsoft says access to SINGLE Dx device by multiple threads is ok

			pSurface = pTexture;

			if (FAILED(hrr))
			{
				FATAL("Invalid video stream");
				R_CHK(hrr);

				xr_delete(pTheora);

				pSurface = 0;
				m_pSRView = 0;
			}
			else
			{
				CHK_DX(HW.pDevice->CreateShaderResourceView(pSurface, 0, &m_pSRView)); //Microsoft says access to SINGLE Dx device by multiple threads is ok
			}

		}
	}
	else if (FS.exist(fn, "$game_textures$", *cName, ".avi"))
	{
		// AVI
		pAVI = xr_new<CAviPlayerCustom>();

		if (!pAVI->Load(fn))
		{
			xr_delete(pAVI);

			FATAL("Can't open video stream");
		}
		else
		{
			flags.MemoryUsage = pAVI->m_dwWidth*pAVI->m_dwHeight * 4;

			// Now create texture
			ID3DTexture2D* pTexture = 0;

			D3D_TEXTURE2D_DESC t_desc;

			t_desc.Width = pAVI->m_dwWidth;
			t_desc.Height = pAVI->m_dwHeight;
			t_desc.MipLevels = 1;
			t_desc.ArraySize = 1;
			t_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			t_desc.SampleDesc.Count = 1;
			t_desc.SampleDesc.Quality = 0;
			t_desc.Usage = D3D_USAGE_DYNAMIC;
			t_desc.BindFlags = D3D_BIND_SHADER_RESOURCE;
			t_desc.CPUAccessFlags = D3D_CPU_ACCESS_WRITE;
			t_desc.MiscFlags = 0;

			HRESULT hrr = HW.pDevice->CreateTexture2D(&t_desc, 0, &pTexture); //Microsoft says access to SINGLE Dx device by multiple threads is ok

			pSurface = pTexture;

			if (FAILED(hrr))
			{
				FATAL("Invalid video stream");

				R_CHK(hrr);

				xr_delete(pAVI);

				pSurface = 0;
				m_pSRView = 0;
			}
			else
			{
				CHK_DX(HW.pDevice->CreateShaderResourceView(pSurface, 0, &m_pSRView)); //Microsoft says access to SINGLE Dx device by multiple threads is ok
			}

		}
	}
	else if (FS.exist(fn, "$game_textures$", *cName, ".seq"))
	{
		// Sequence
		string256 buffer;
		IReader* _fs = FS.r_open(fn);

		flags.seqCycles = FALSE;
		_fs->r_string(buffer, sizeof(buffer));

		if (0 == stricmp(buffer, "cycled"))
		{
			flags.seqCycles = TRUE;
			_fs->r_string(buffer, sizeof(buffer));
		}

		u32 fps = atoi(buffer);
		seqMSPF = 1000 / fps;

		while (!_fs->eof())
		{
			_fs->r_string(buffer, sizeof(buffer));
			_Trim(buffer);

			if (buffer[0])
			{
				// Load another texture
				u32	mem = 0;
				pSurface = ::RImplementation.texture_load(buffer, mem);

				if (pSurface)
				{
					seqDATA.push_back(pSurface);
					m_seqSRView.push_back(0);
					HW.pDevice->CreateShaderResourceView(seqDATA.back(), nullptr, &m_seqSRView.back()); //Microsoft says access to SINGLE Dx device by multiple threads is ok
					flags.MemoryUsage += mem;
				}
			}
		}

		pSurface = 0;

		FS.r_close(_fs);
	}
	else // Regular texture
	{
		CTimer T; T.Start();

		u32	mem = 0;

		bool mm_active = (g_pGamePersistent && g_pGamePersistent->m_pMainMenu && g_pGamePersistent->m_pMainMenu->IsActive());
		bool compatable_texture = (!strstr(*cName, "intro\\") && !strstr(*cName, "ui\\")); // no ui and shits
		bool mt = (mt_dynTextureLoad_ || mt_anyway) && !already_in_mt && !mm_active && compatable_texture; // only during rendering scene and when there is no texture loading thread running

		if (mt)
		{
			pSurface = ::RImplementation.texture_load("ed\\ed_not_existing_texture", mem, false); // load small dummy texture as temporary

			flags.bLoadedAsStaging = FALSE;
			bCreateView = false;
		}
		else
		{
			pSurface = ::RImplementation.texture_load(*cName, mem, true);

			if (GetUsage(pSurface) == D3D_USAGE_STAGING)
			{
				flags.bLoadedAsStaging = TRUE;
				bCreateView = false;
			}
		}

		// Calc memory usage and preload into vid-mem
		if (pSurface)
			flags.MemoryUsage = mem;

		if (mt) // push back into aux thread workload if alowed
		{
			SetIsProcessingMT(true);

			Device.AddToResUploadingThread_1_Pool(fastdelegate::FastDelegate0<>(this, &CTexture::LoadMT));

			Msg("* Loading texture in secondary thread: %s", *cName);
		}

		if (pSurface && bCreateView)
			CHK_DX(HW.pDevice->CreateShaderResourceView(pSurface, nullptr, &m_pSRView)); //Microsoft says access to SINGLE Dx device by multiple threads is ok

		if (!mt)
		{
			PostLoad();
		}

		//Msg("# Loading texture mt %d Time %f %s", mt, T.GetElapsed_sec() * 1000.f, *cName);
	}

	if (pSurface && bCreateView)
		CHK_DX(HW.pDevice->CreateShaderResourceView(pSurface, nullptr, &m_pSRView)); //Microsoft says access to SINGLE Dx device by multiple threads is ok

	PostLoad();

	loadLock_.Leave();
}

void CTexture::LoadMT()
{
	if (IsReadyToDestroy()) // Should not realy happen, but in case
	{
#ifdef DEBUG
		Msg("!Deleting texture and MT loading at the same time?");
#endif

		return;
	}

	R_ASSERT(this); // because mt

#ifdef DEBUG
	Msg("Loading texture in parallel: %s", *cName);
#endif

	bool bCreateView = true;
	u32	mem = 0;

	// Load texture into another pointer
	mtLoadedSurface_ = ::RImplementation.texture_load(*cName, mem, true);

	if (GetUsage(mtLoadedSurface_) == D3D_USAGE_STAGING)
		bCreateView = false;

	// Calc memory usage and preload into vid-mem
	if (mtLoadedSurface_)
		flags.MemoryUsage = mem;

	if (mtLoadedSurface_ && bCreateView)
		CHK_DX(HW.pDevice->CreateShaderResourceView(mtLoadedSurface_, nullptr, &mtLoadedViewRes_)); //Microsoft says access to SINGLE Dx device by multiple threads is ok

	RImplementation.protectDefTexturesPool_.Enter();
	RImplementation.mtDefferedLoadedTexturesCB_.push_back(fastdelegate::FastDelegate0<>(this, &CTexture::LoadMTFinish));
	RImplementation.protectDefTexturesPool_.Leave();
}

void CTexture::LoadMTFinish() // this func is called at the frame end in the main thread
{
	R_ASSERT(this); // because mt

	flags.bLoaded = true;

	if (mtLoadedSurface_)
	{
		if (pSurface)
			_RELEASE(pSurface);

		pSurface = mtLoadedSurface_;

		mtLoadedSurface_ = nullptr;
	}

	if (mtLoadedViewRes_)
	{
		if (m_pSRView)
			_RELEASE(m_pSRView);

		m_pSRView = mtLoadedViewRes_;

		mtLoadedViewRes_ = nullptr;
	}

	desc_cache = 0;

	PostLoad();

	if (GetUsage(pSurface) == D3D_USAGE_STAGING)
		flags.bLoadedAsStaging = TRUE;

	SetIsProcessingMT(false);
}

void CTexture::UnloadTexture()
{
#ifdef DEBUG
	string_path msg_buff;
	xr_sprintf(msg_buff, sizeof(msg_buff), "* Unloading texture [%s] pSurface RefCount=", cName.c_str());
#endif

	flags.bLoaded = FALSE;
	flags.bLoadedAsStaging = FALSE;

	if (!seqDATA.empty())
	{
		for (u32 I = 0; I<seqDATA.size(); I++)
		{
			_RELEASE(seqDATA[I]);
			_RELEASE(m_seqSRView[I]);
		}

		seqDATA.clear();
		m_seqSRView.clear();

		pSurface = nullptr;
		m_pSRView = nullptr;
	}

#ifdef DEBUG
	_SHOW_REF(msg_buff, pSurface);
#endif
	if (pSurface)
		_RELEASE(pSurface);

	if (m_pSRView)
		_RELEASE(m_pSRView);

	if (mtLoadedViewRes_)
		_RELEASE(mtLoadedViewRes_);

	if (mtLoadedSurface_)
		_RELEASE(mtLoadedSurface_);

	xr_delete(pAVI);
	xr_delete(pTheora);

	bind = fastdelegate::FastDelegate1<u32>(this, &CTexture::apply_load);
}

void CTexture::desc_update()
{
	desc_cache = pSurface;

	if (pSurface)
	{
		D3D_RESOURCE_DIMENSION type;
		pSurface->GetType(&type);

		if (D3D_RESOURCE_DIMENSION_TEXTURE2D == type)
		{
			ID3DTexture2D*	T = (ID3DTexture2D*)pSurface;
			T->GetDesc(&desc);
		}
	}
}

D3D_USAGE CTexture::GetUsage(ID3DBaseTexture* surface)
{
	D3D_USAGE res = D3D_USAGE_DEFAULT;

	if (surface)
	{
		D3D_RESOURCE_DIMENSION type;
		surface->GetType(&type);

		switch (type)
		{
		case D3D_RESOURCE_DIMENSION_TEXTURE1D:
		{
			ID3DTexture1D* T = (ID3DTexture1D*)surface;
			D3D_TEXTURE1D_DESC descr;

			T->GetDesc(&descr);
			res = descr.Usage;
		}
		break;

		case D3D_RESOURCE_DIMENSION_TEXTURE2D:
		{
			ID3DTexture2D* T = (ID3DTexture2D*)surface;
			D3D_TEXTURE2D_DESC descr;

			T->GetDesc(&descr);
			res = descr.Usage;
		}
		break;

		case D3D_RESOURCE_DIMENSION_TEXTURE3D:
		{
			ID3DTexture3D* T = (ID3DTexture3D*)surface;
			D3D_TEXTURE3D_DESC descr;

			T->GetDesc(&descr);
			res = descr.Usage;
		}
		break;

		default:
			VERIFY(!"Unknown texture format???");
		}
	}

	return res;
}

void CTexture::video_Play(BOOL looped, u32 _time)
{
	if (pTheora)
		pTheora->Play(looped, (_time != 0xFFFFFFFF) ? (m_play_time = _time) : EngineTimeContinual());
}

void CTexture::video_Pause(BOOL state)
{
	if (pTheora)
		pTheora->Pause(state);
}

void CTexture::video_Stop()
{
	if (pTheora)
		pTheora->Stop();
}

BOOL CTexture::video_IsPlaying()
{
	return (pTheora) ? pTheora->IsPlaying() : FALSE;
}

void resptrcode_texture::_dec()
{
	if (!p_)
		return;

	p_->dwReference--;

	if (!p_->dwReference)
		DEV->_DeleteTexture(p_);
}