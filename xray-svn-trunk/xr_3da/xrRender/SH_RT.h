#ifndef SH_RT_H
#define SH_RT_H
#pragma once

enum ViewPort;

struct RtCreationParams
{
	u32 w;
	u32 h;
	ViewPort viewport;

	RtCreationParams(u32 W, u32 H, ViewPort vp)
	{
		w = W;
		h = H;
		viewport = vp;
	};
};

struct ViewPortRTResources
{
	ViewPortRTResources()
	{
		rtWidth = 0;
		rtHeight = 0;

		textureSurface					= nullptr;
		zBufferInstance					= nullptr;
		renderTargetInstance			= nullptr;
		unorderedAccessViewInstance		= nullptr;
		shaderResView					= nullptr;
	}

	ID3DTexture2D* textureSurface;

	u32	rtWidth;
	u32 rtHeight;

	ID3DDepthStencilView* zBufferInstance;
	ID3DRenderTargetView* renderTargetInstance;
	ID3D11UnorderedAccessView* unorderedAccessViewInstance;
	ID3DShaderResourceView*	shaderResView;
};

class CRT : public xr_resource_named
{
private:
	u32						rtWidth;
	u32						rtHeight;

public:
	CRT							();
	~CRT						();

	void	create				(LPCSTR Name, xr_vector<RtCreationParams>& vp_params, D3DFORMAT f, u32 SampleCount = 1, bool useUAV = false );

	void	destroy				();
	void	reset_begin			();
	void	reset_end			();

	void	SwitchViewPortResources	(ViewPort vp);

	u32		RTWidth				() { return rtWidth; };
	u32		RTHeight			() { return rtHeight; };

	IC BOOL	valid				() { return !!pTexture; }

public:
	ViewPort					vpStored;

	bool						isTwoViewPorts;
	ID3DTexture2D*				temp;

	xr_map<u32, ViewPortRTResources> viewPortRTResList;
	xr_vector<RtCreationParams>	creationParams;

	ID3DTexture2D*				pSurface;
	ID3DRenderTargetView*		pRT;
	ID3DDepthStencilView*		pZRT;

	ID3D11UnorderedAccessView*	pUAView;

	ref_texture					pTexture;

	D3DFORMAT					fmt;

	u64							_order;

	shared_str					rtName;
};

struct resptrcode_crt : public resptr_base<CRT>
{

	void				create			(LPCSTR Name, xr_vector<RtCreationParams>& vp_params, D3DFORMAT f, u32 SampleCount = 1, bool useUAV = false );

	void				create			(LPCSTR Name, RtCreationParams creation_params, D3DFORMAT f, u32 SampleCount = 1, bool useUAV = false)
	{
		xr_vector<RtCreationParams> params;
		params.push_back(creation_params);

		create(Name, params, f, SampleCount, useUAV);
	};

	void				create			(LPCSTR Name, RtCreationParams creation_params_1, RtCreationParams creation_params_2, D3DFORMAT f, u32 SampleCount = 1, bool useUAV = false)
	{
		xr_vector<RtCreationParams> params;
		params.push_back(creation_params_1);
		params.push_back(creation_params_2);

		create(Name, params, f, SampleCount, useUAV);
	};


	void				destroy			()	{ _set(NULL); }
};

typedef	resptr_core<CRT,resptrcode_crt> ref_rt;

#endif
