// TextureManager.h: interface for the CTextureManager class.
//
//////////////////////////////////////////////////////////////////////

#ifndef ResourceManagerH
#define ResourceManagerH
#pragma once

#include "shader.h"
#include "tss_def.h"
#include "TextureDescrManager.h"

struct lua_State;
class dx10ConstantBuffer;


class ECORE_API CResourceManager
{
private:
	struct str_pred : public std::binary_function<char*, char*, bool>	
	{
		IC bool operator()(LPCSTR x, LPCSTR y) const
		{
			return xr_strcmp(x, y) < 0;
		}
	};

	struct texture_detail
	{
		const char*	T;
		R_constant_setup* cs;
	};

public:

	DEFINE_MAP_PRED(const char*, IBlender*,		map_Blender,	map_BlenderIt,		str_pred);
	DEFINE_MAP_PRED(const char*, CTexture*,		map_Texture,	map_TextureIt,		str_pred);
	DEFINE_MAP_PRED(const char*, CMatrix*,		map_Matrix,		map_MatrixIt,		str_pred);
	DEFINE_MAP_PRED(const char*, CConstant*,	map_Constant,	map_ConstantIt,		str_pred);
	DEFINE_MAP_PRED(const char*, CRT*,			map_RT,			map_RTIt,			str_pred);
	DEFINE_MAP_PRED(const char*, SVS*,			map_VS,			map_VSIt,			str_pred);
	DEFINE_MAP_PRED(const char*, SGS*,			map_GS,			map_GSIt,			str_pred);
	DEFINE_MAP_PRED(const char*, SHS*,			map_HS,			map_HSIt,			str_pred);
	DEFINE_MAP_PRED(const char*, SDS*,			map_DS,			map_DSIt,			str_pred);
	DEFINE_MAP_PRED(const char*, SCS*,			map_CS,			map_CSIt,			str_pred);
	DEFINE_MAP_PRED(const char*, SPS*,			map_PS,			map_PSIt,			str_pred);
	DEFINE_MAP_PRED(const char*, texture_detail,map_TD,			map_TDIt,			str_pred);

private:
	// data
	map_Blender											m_blenders;
	map_Texture											m_textures;
	map_Matrix											m_matrices;
	map_Constant										m_constants;

	AccessLock											protectTexturesToDelete_;
	xr_vector<CTexture*>								texturesToDelete_;

	map_RT												m_rtargets;
	map_VS												m_vs;
	map_PS												m_ps;
	map_GS												m_gs;
	map_TD												m_td;

	xr_vector<SState*>									v_states;
	xr_vector<SDeclaration*>							v_declarations;
	xr_vector<SGeometry*>								v_geoms;
	xr_vector<R_constant_table*>						v_constant_tables;
	xr_vector<dx10ConstantBuffer*>						v_constant_buffer;
	xr_vector<SInputSignature*>							v_input_signature;

	// lists
	xr_vector<STextureList*>							lst_textures;
	xr_vector<SMatrixList*>								lst_matrices;
	xr_vector<SConstantList*>							lst_constants;

	// main shader-array
	xr_vector<SPass*>									v_passes;
	xr_vector<ShaderElement*>							v_elements;
	xr_vector<Shader*>									v_shaders;
	xr_vector<Shader*>									v_shaders_templates;
	
	xr_vector<ref_texture>								m_necessary;
	// misc

	bool					isLoadingTextures_;
	AccessLock				protIsLoadingTextures_;

	// for testing ram
	xr_vector<U32Vec*>									ramTestingPool;

public:
	CTextureDescrMngr									m_textures_description;
	xr_vector<std::pair<shared_str,R_constant_setup*> >	v_constant_setup;
	lua_State*											LSVM;
	BOOL												bDeferredLoad;

	u32								createdTexturesCnt_;
	u32								deletedTexturesCnt_;

private:
	void							LS_Load				();
	void							LS_Unload			();

	static void						TextureLoadingThread(void* p);
public:
	// Miscelaneous
	void							_ParseList			(sh_list& dest, LPCSTR names);
	IBlender*						_GetBlender			(LPCSTR Name);
	IBlender* 						_FindBlender		(LPCSTR Name);
	void							_GetMemoryUsage		(u32& m_base, u32& c_base, u32& m_lmaps, u32& c_lmaps);
	void							_DumpMemoryUsage	();
	map_Blender&					_GetBlenders		()		{ return m_blenders; }

	// Debug
	void							DBG_VerifyGeoms		();
	void							DBG_VerifyTextures	();

	// Editor cooperation
	void							ED_UpdateBlender	(LPCSTR Name, IBlender* data);
	void							ED_UpdateMatrix		(LPCSTR Name, CMatrix* data);
	void							ED_UpdateConstant	(LPCSTR Name, CConstant* data);

	// Low level resource creation
	CTexture*						_CreateTexture		(LPCSTR Name, bool mt_anyway = false);
	// Add texture to destroy queue(MT safe removal)
	void							_DeleteTexture		(CTexture*& T);

	void							_DeleteTextureRef	(const CTexture* T);

	CMatrix*						_CreateMatrix		(LPCSTR Name);
	void							_DeleteMatrix		(const CMatrix* M);

	CConstant*						_CreateConstant		(LPCSTR Name);
	void							_DeleteConstant		(const CConstant* C);

	R_constant_table*				_CreateConstantTable		(R_constant_table& C);
	void							_DeleteConstantTable		(const R_constant_table* C);

	dx10ConstantBuffer*				_CreateConstantBuffer		(ID3DShaderReflectionConstantBuffer* pTable);
	void							_DeleteConstantBuffer		(const dx10ConstantBuffer* pBuffer);

	SInputSignature*				_CreateInputSignature		(ID3DBlob* pBlob);
	void							_DeleteInputSignature		(const SInputSignature* pSignature);

	CRT*							_CreateRT			(LPCSTR Name, xr_vector<RtCreationParams>& vp_params, D3DFORMAT f, u32 SampleCount = 1, bool useUAV = false );
	void							_DeleteRT			(const CRT*	RT);

	SGS*							_CreateGS			(LPCSTR Name);
	void							_DeleteGS			(const SGS*	GS);

	SHS*							_CreateHS			(LPCSTR Name);
	void							_DeleteHS			(const SHS*	HS);

	SDS*							_CreateDS			(LPCSTR Name);
	void							_DeleteDS			(const SDS*	DS);

    SCS*							_CreateCS			(LPCSTR Name);
	void							_DeleteCS			(const SCS*	CS);

	SPS*							_CreatePS			(LPCSTR Name);
	void							_DeletePS			(const SPS*	PS);

	SVS*							_CreateVS			(LPCSTR Name);
	void							_DeleteVS			(const SVS*	VS);

	SPass*							_CreatePass			(const SPass& proto);
	void							_DeletePass			(const SPass* P);

	// Shader compiling / optimizing
	SState*							_CreateState		(SimulatorStates& Code);
	void							_DeleteState		(const SState* SB);

	SDeclaration*					_CreateDecl			(D3DVERTEXELEMENT9* dcl);
	void							_DeleteDecl			(const SDeclaration* dcl);

	STextureList*					_CreateTextureList	(STextureList& L);
	void							_DeleteTextureList	(const STextureList* L);

	SMatrixList*					_CreateMatrixList	(SMatrixList& L);
	void							_DeleteMatrixList	(const SMatrixList* L);

	SConstantList*					_CreateConstantList	(SConstantList& L);
	void							_DeleteConstantList	(const SConstantList* L);

	ShaderElement*					_CreateElement		(ShaderElement& L);
	void							_DeleteElement		(const ShaderElement* L);

	Shader*							_cpp_Create			(bool MakeCopyable, LPCSTR s_shader, LPCSTR s_textures = 0, LPCSTR s_constants = 0, LPCSTR s_matrices = 0);
	Shader*							_cpp_Create			(bool MakeCopyable, IBlender* B, LPCSTR s_shader = 0, LPCSTR s_textures = 0, LPCSTR s_constants = 0, LPCSTR s_matrices = 0);

	Shader*							_lua_Create			(LPCSTR s_shader, LPCSTR s_textures);
	BOOL							_lua_HasShader		(LPCSTR s_shader);

	const map_RT&					GetRTList			() const { return m_rtargets; };

	CResourceManager						() : bDeferredLoad(TRUE) { LSVM = nullptr; }
	~CResourceManager						();

	void			OnDeviceCreate			(IReader* F);
	void			OnDeviceCreate			(LPCSTR name);
	void			OnDeviceDestroy			(BOOL bKeepTextures);

	void			reset_begin				();
	void			reset_end				();

	// Creation/Destroying
	Shader*			Create					(LPCSTR s_shader = 0, LPCSTR s_textures = 0, LPCSTR s_constants = 0, LPCSTR s_matrices = 0);
	Shader*			Create					(IBlender* B, LPCSTR s_shader = 0, LPCSTR s_textures = 0, LPCSTR s_constants = 0, LPCSTR s_matrices = 0);
	void			Delete					(const Shader* S);

	void			RegisterConstantSetup	(LPCSTR name, R_constant_setup* s)	{ v_constant_setup.push_back(mk_pair(shared_str(name), s)); }

	SGeometry*		CreateGeom				(D3DVERTEXELEMENT9* decl, ID3DVertexBuffer* vb, ID3DIndexBuffer* ib);
	SGeometry*		CreateGeom				(u32 FVF , ID3DVertexBuffer* vb, ID3DIndexBuffer* ib);
	void			DeleteGeom				(const SGeometry* VS);

	void			DeferredLoad			(BOOL E)					{ bDeferredLoad = E; }
	void			DeferredUpload			(BOOL multithreaded);
	void			SyncPrefetchLoading		();
	void			SyncTexturesLoading		();

	void			Evict					();
	void			StoreNecessaryTextures	();
	void			DestroyNecessaryTextures();
	void			Dump					(bool bBrief);
	void			RMPrefetchUITextures	();

	bool			IsLoadingTextures		()				{ protIsLoadingTextures_.Enter(); bool ret = isLoadingTextures_; protIsLoadingTextures_.Leave(); return ret; };
	void			LoadingTexturesSet		(bool val)		{ protIsLoadingTextures_.Enter(); isLoadingTextures_ = val; protIsLoadingTextures_.Leave(); };
	
	// Process mt safe texture removal
	u32				DeleteTextureQueue		(bool forced = false);

	void			TestRAM					(u32 amount = 512);

private:

	map_DS	m_ds;
	map_HS	m_hs;
	map_CS	m_cs;

	template<typename T>
	T& GetShaderMap();

	template<typename T>
	T* CreateShader(const char* name);

	template<typename T>
	void DestroyShader(const T* sh);
};

#endif
