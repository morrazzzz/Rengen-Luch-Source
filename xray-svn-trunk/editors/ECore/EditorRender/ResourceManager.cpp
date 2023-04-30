// TextureManager. Implementation of the CResourceManager class.

#include "stdafx.h"
#pragma hdrstop

#pragma warning(disable:4995)
#include <d3dx9.h>
#pragma warning(default:4995)

#include "ResourceManager.h"
#include "tss.h"
#include "blenders\blender.h"
#include "blenders\blender_recorder.h"

#ifndef _EDITOR
	#include <thread>
#endif

//	Already defined in Texture.cpp
void fix_texture_name(LPSTR fn);

//--------------------------------------------------------------------------------------------------------------
template <class T>
BOOL	reclaim		(xr_vector<T*>& vec, const T* ptr)
{
#ifndef _EDITOR
	xr_vector<T*>::iterator it	= vec.begin	();
	xr_vector<T*>::iterator end	= vec.end	();
	for (; it!=end; it++)
		if (*it == ptr)	{ vec.erase	(it); return TRUE; }
		return FALSE;
#else
	xr_vector<T*>::iterator it = std::find(vec.begin(), vec.end(), ptr);
	if (it != vec.end())
	{
		vec.erase(it);
		return TRUE;
	}
	return FALSE;
#endif
}

//--------------------------------------------------------------------------------------------------------------
IBlender* CResourceManager::_GetBlender		(LPCSTR Name)
{
	R_ASSERT(Name && Name[0]);

	LPSTR N = LPSTR(Name);
	map_Blender::iterator I = m_blenders.find	(N);
#ifdef _EDITOR
	if (I==m_blenders.end())	return 0;
#else
//	TODO: DX10: When all shaders are ready switch to common path
#if defined(USE_DX10) || defined(USE_DX11)
	if (I==m_blenders.end())	
	{
		Msg("DX10: Shader '%s' not found in library.",Name); 
		return 0;
	}
#endif
	if (I==m_blenders.end())	{ Debug.fatal(DEBUG_INFO,"Shader '%s' not found in library.",Name); return 0; }
#endif
	else					return I->second;
}

IBlender* CResourceManager::_FindBlender		(LPCSTR Name)
{
	if (!(Name && Name[0])) return 0;

	LPSTR N = LPSTR(Name);
	map_Blender::iterator I = m_blenders.find	(N);
	if (I==m_blenders.end())	return 0;
	else						return I->second;
}

void	CResourceManager::ED_UpdateBlender	(LPCSTR Name, IBlender* data)
{
	LPSTR N = LPSTR(Name);
	map_Blender::iterator I = m_blenders.find	(N);
	if (I!=m_blenders.end())	{
		R_ASSERT	(data->getDescription().CLS == I->second->getDescription().CLS);
		xr_delete	(I->second);
		I->second	= data;
	} else {
		m_blenders.insert	(mk_pair(xr_strdup(Name),data));
	}
}

void	CResourceManager::_ParseList(sh_list& dest, LPCSTR names)
{
	if (0==names || 0==names[0])
 		names 	= "$null";

	ZeroMemory			(&dest, sizeof(dest));
	char*	P			= (char*) names;
	svector<char,128>	N;

	while (*P)
	{
		if (*P == ',') {
			// flush
			N.push_back	(0);
			strlwr		(N.begin());

			fix_texture_name( N.begin() );
			dest.push_back(N.begin());
			N.clear		();
		} else {
			N.push_back	(*P);
		}
		P++;
	}
	if (N.size())
	{
		// flush
		N.push_back	(0);
		strlwr		(N.begin());

		fix_texture_name( N.begin() );
		dest.push_back(N.begin());
	}
}

ShaderElement* CResourceManager::_CreateElement			(ShaderElement& S)
{
	if (S.passes.empty())		return	0;

	// Search equal in shaders array
	for (u32 it=0; it<v_elements.size(); it++)
		if (S.equal(*(v_elements[it])))	return v_elements[it];

	// Create _new_ entry
	ShaderElement*	N		=	xr_new<ShaderElement>(S);
	N->dwFlags				|=	xr_resource_flagged::RF_REGISTERED;
	v_elements.push_back	(N);
	return N;
}

void CResourceManager::_DeleteElement(const ShaderElement* S)
{
	if (0==(S->dwFlags&xr_resource_flagged::RF_REGISTERED))	return;
	if (reclaim(v_elements,S))						return;
	Msg	("! ERROR: Failed to find compiled 'shader-element'");
}

Shader*	CResourceManager::_cpp_Create(bool MakeCopyable, IBlender* B, LPCSTR s_shader, LPCSTR s_textures, LPCSTR s_constants, LPCSTR s_matrices)
{
#ifndef _EDITOR
	CTimer time; time.Start();
#endif
	CBlender_Compile	C;
	Shader				S;

	//.
	// if (strstr(s_shader,"transparent"))	__asm int 3;

	// Access to template
	C.BT				= B;
	C.bEditor			= FALSE;
	C.bDetail			= FALSE;
#ifdef _EDITOR
	if (!C.BT)			{ ELog.Msg(mtError,"Can't find shader '%s'",s_shader); return 0; }
	C.bEditor			= TRUE;
#endif
	// Parse names
	_ParseList			(C.L_textures,	s_textures	);
	_ParseList			(C.L_constants,	s_constants	);
	_ParseList			(C.L_matrices,	s_matrices	);
	// Compile element	(LOD0 - HQ)
	{
		C.iElement			= 0;
		C.bDetail			= m_textures_description.GetDetailTexture(C.L_textures[0],C.detail_texture,C.detail_scaler);
		ShaderElement		E;
		C._cpp_Compile		(&E);
		S.E[0]				= _CreateElement	(E);
	}
	// Compile element	(LOD1)
	{
		C.iElement			= 1;
		C.bDetail			= m_textures_description.GetDetailTexture(C.L_textures[0],C.detail_texture,C.detail_scaler);
		ShaderElement		E;
		C._cpp_Compile		(&E);
		S.E[1]				= _CreateElement	(E);
	}
	// Compile element
	{
		C.iElement			= 2;
		C.bDetail			= FALSE;
		ShaderElement		E;
		C._cpp_Compile		(&E);
		S.E[2]				= _CreateElement	(E);
	}
	// Compile element
	{
		C.iElement			= 3;
		C.bDetail			= FALSE;
		ShaderElement		E;
		C._cpp_Compile		(&E);
		S.E[3]				= _CreateElement	(E);
	}
	// Compile element
	{
		C.iElement			= 4;
		C.bDetail			= TRUE;	//.$$$ HACK :)
		ShaderElement		E;
		C._cpp_Compile		(&E);
		S.E[4]				= _CreateElement	(E);
	}

	// Compile element
	{
		C.iElement			= 5;
		C.bDetail			= FALSE;
		ShaderElement		E;
		C._cpp_Compile		(&E);
		S.E[5]				= _CreateElement	(E);
	}

	Shader* N = NULL;

	// Search equal in shaders array
	for (u32 it = 0; it < v_shaders.size(); it++){
		if (S.equal(v_shaders[it])){
			if (v_shaders[it]->copyable)
			{
				N = xr_new <Shader>(*v_shaders[it]); // if the shader is used as template, return its copy
			}
			else
				N = v_shaders[it]; // return direct instance from shader pool
		}
	}
	if (!N)
	{
		// Create _new_ entry
		N = xr_new <Shader>(S);
		N->dwFlags |= xr_resource_flagged::RF_REGISTERED;
		N->copyable = MakeCopyable; // Assign it as a template for future needs.
		v_shaders.push_back(N);
	}

#ifndef _EDITOR
	if (psDeviceFlags.test(rsPrefObjects) && !prefetching_in_progress && g_loading_events.empty() && time.GetElapsed_sec()*1000.f > 5.0)
		Msg("# Loading of %s made a %fms stutter", s_textures, time.GetElapsed_sec()*1000.f);
#endif

	return N;
}

Shader*	CResourceManager::_cpp_Create(bool MakeCopyable, LPCSTR s_shader, LPCSTR s_textures, LPCSTR s_constants, LPCSTR s_matrices)
{
	//	TODO: DX10: When all shaders are ready switch to common path
#if defined(USE_DX10) || defined(USE_DX11)
	IBlender	*pBlender = _GetBlender(s_shader?s_shader:"null");
	if (!pBlender) return NULL;
	return	_cpp_Create(MakeCopyable, pBlender, s_shader, s_textures, s_constants, s_matrices);
#else
	return	_cpp_Create(MakeCopyable, _GetBlender(s_shader?s_shader:"null"),s_shader,s_textures,s_constants,s_matrices);
#endif

}

Shader*	CResourceManager::Create	(IBlender*	B,		LPCSTR s_shader,	LPCSTR s_textures,	LPCSTR s_constants, LPCSTR s_matrices)
{
	return	_cpp_Create	(false, B, s_shader,s_textures,s_constants,s_matrices);
}

Shader*	CResourceManager::Create	(LPCSTR s_shader,	LPCSTR s_textures,	LPCSTR s_constants,	LPCSTR s_matrices)
{
		//	TODO: DX10: When all shaders are ready switch to common path
#if defined(USE_DX10) || defined(USE_DX11)
		if	(_lua_HasShader(s_shader))		
			return	_lua_Create	(s_shader,s_textures);
		else								
		{
			Shader* pShader = _cpp_Create	(false, s_shader,s_textures,s_constants,s_matrices);
			if (pShader)
				return pShader;
			else
			{
				if (_lua_HasShader("stub_default"))
					return	_lua_Create	("stub_default",s_textures);
				else
				{
					FATAL("Can't find stub_default.s");
					return 0;
				}
			}
		}
#else
#ifndef _EDITOR
		if	(_lua_HasShader(s_shader))		
			return	_lua_Create	(s_shader,s_textures);
		else								
#endif
		return	_cpp_Create	(false, s_shader,s_textures,s_constants,s_matrices);
#endif

}

void CResourceManager::Delete(const Shader* S)
{
	if (S->copyable) return; // if its a "copied" shader, then dont delete its pointer in shaders pool, since there isn't one there
	if (0==(S->dwFlags&xr_resource_flagged::RF_REGISTERED))	return;
	if (reclaim(v_shaders,S))						return;
	Msg	("! ERROR: Failed to find complete shader");
}


#ifndef _EDITOR // if not SDK - do MT Txs. Loading

xr_vector<CTexture*> textures_; // a copy of textures pointers pool
AccessLock txLoadingMutex1_, txLoadingMutex2_; //these locks guard critical thread indexing globals, which can be raced by threads and go wrong

u32 texturePerThread_; // Amount of textures for each thread
u32 spawnedTxThreads_ = 0;
u32 readyThreadsCount_ = 0;

xr_vector<shared_str> threadNames_;

void CResourceManager::TextureLoadingThread(void* p)
{
	CResourceManager& resource_manager = *static_cast<CResourceManager*>(p);

	txLoadingMutex1_.Enter();
	spawnedTxThreads_ += 1;
	u32 thread_number = spawnedTxThreads_;
	txLoadingMutex1_.Leave();

	u32 upper_bound = thread_number * texturePerThread_;
	u32 lower_bound = upper_bound - texturePerThread_;

	Msg("*Thread# %u is for textures from %u to %u", thread_number, lower_bound, upper_bound);

	for (u32 i = lower_bound; i < upper_bound; i++)
	{
		if (i < textures_.size() && textures_[i]) //prevent empty texture crash
		{
			//LPCSTR texturename = *textures[i]->cName ? textures[i]->cName.c_str() : "Error in texture name; Texture will be skipped";
			//Msg("*Thread# %u: Loading texture# %u with name %s", threadnumber, i, texturename);
			textures_[i]->Load();
		}
	}

	Msg("*Thread# %u : Done!", thread_number);

	txLoadingMutex2_.Enter();
	readyThreadsCount_ += 1;
	if (readyThreadsCount_ == spawnedTxThreads_) // this check determines if the thread was the last one to finish and then clear temporary array
	{ 
		Msg("*MT Texture Loading Finished");
		resource_manager.LoadingTexturesSet(false);

		textures_.clear();
	}
	txLoadingMutex2_.Leave();

}

void CResourceManager::DeferredUpload(BOOL multithreaded)
{
	if (!RDEVICE.b_is_Ready){ Log("!Texture loading: Device Not Ready, skipping texture loading"); return; }

	std::random_shuffle(textures_.begin(), textures_.end()); // for even spread of workload

	if (textures_.size() > 0) g_pGamePersistent->LoadTitle("st_tx_loading_sync"); Msg("#Syncing texture loading threads");

	while (textures_.size() > 0) Sleep(50); //Sync to not interupt prefetching, if its not finished yet (Possible on uneven cpus+ssd perfomance)

	LoadingTexturesSet(true);

	if (multithreaded) // multithreaded method
	{

		for (map_TextureIt t = m_textures.begin(); t != m_textures.end(); t++) // Copy pointers to textures pool, to avoid pool access races
		{
			textures_.push_back(t->second); // pushing all textures to a tempolary pool that will be processed by threads
		}

		u32 threads_to_spawn = std::thread::hardware_concurrency() - 1; // Leave one thread for system and other background apps
		_min(threads_to_spawn, 2); // Minimum 2 threads

		texturePerThread_ = (textures_.size() / threads_to_spawn) + 1; // +1 is to compensate uneven devision. Its OK, thread will not try to load it if index gets out of textures size

		readyThreadsCount_ = 0;
		spawnedTxThreads_ = 0;
		Msg("*Texture count = %u, i = %u, delta = %u", textures_.size(), threads_to_spawn, texturePerThread_);

		threadNames_.clear();

		for (u32 y = 1; y <= threads_to_spawn; y++) // Begin spawning threads
		{
			Msg("*Spawning thread# %u", y);

			string128 str;
			xr_sprintf(str, "Texture Loading Thread %u", y);
			threadNames_.push_back(str);

			thread_spawn(TextureLoadingThread, *threadNames_.back(), 0, this);
		}

		Msg("*Spawning %u threads: Done!", threads_to_spawn);
	}
	else // single threaded method
	{
		for (map_TextureIt t=m_textures.begin(); t!=m_textures.end(); t++)
		{
			t->second->Load();
		}

		LoadingTexturesSet(false);
	}
}


#else // if SDK - do regular STreaded Txs. Loading
void CResourceManager::DeferredUpload(BOOL multithreaded)
{
	if (!RDEVICE.b_is_Ready) return;
	for (map_TextureIt t=m_textures.begin(); t!=m_textures.end(); t++)
	{
		t->second->Load();
	}
}
#endif

void CResourceManager::RMPrefetchUITextures()
{
	v_shaders_templates.clear();
	CTimer time; time.Start();
	CInifile::Sect& sect = pSettings->r_section("prefetch_ui_textures");
	for (CInifile::SectCIt I = sect.Data.begin(); I != sect.Data.end(); I++)
	{
		const CInifile::Item& item = *I;
		LPCSTR string = item.first.c_str();
		if (string && string[0])
		{
			string128 texturename;
			string128 shadername;

			_GetItem(string, 0, texturename);
			_GetItem(string, 1, shadername);
			LPCSTR temptexturename = texturename;
			LPCSTR tempshadername = shadername;
			Msg("*Prefetching %s, %s", temptexturename, tempshadername);
			Shader* temp = _cpp_Create(true, tempshadername, temptexturename);
			v_shaders_templates.push_back(temp);
		}
	}
	Msg("*RMPrefetchUITextures %fms", time.GetElapsed_sec()*1000.f);
}

#ifdef _EDITOR
void	CResourceManager::ED_UpdateTextures(AStringVec* names)
{
	// 1. Unload
	if (names){
		for (u32 nid=0; nid<names->size(); nid++)
		{
			map_TextureIt I = m_textures.find	((*names)[nid].c_str());
			if (I!=m_textures.end())	I->second->Unload();
		}
	}else{
		for (map_TextureIt t=m_textures.begin(); t!=m_textures.end(); t++)
			t->second->Unload();
	}

	// 2. Load
	// DeferredUpload	();
}
#endif

void	CResourceManager::_GetMemoryUsage(u32& m_base, u32& c_base, u32& m_lmaps, u32& c_lmaps)
{
	m_base=c_base=m_lmaps=c_lmaps=0;

	map_Texture::iterator I = m_textures.begin	();
	map_Texture::iterator E = m_textures.end	();
	for (; I!=E; I++)
	{
		u32 m = I->second->flags.MemoryUsage;
		if (strstr(I->first,"lmap"))
		{
			c_lmaps	++;
			m_lmaps	+= m;
		} else {
			c_base	++;
			m_base	+= m;
		}
	}
}
void	CResourceManager::_DumpMemoryUsage		()
{
	xr_multimap<u32,std::pair<u32,shared_str> >		mtex	;

	// sort
	{
		map_Texture::iterator I = m_textures.begin	();
		map_Texture::iterator E = m_textures.end	();
		for (; I!=E; I++)
		{
			u32			m = I->second->flags.MemoryUsage;
			shared_str	n = I->second->cName;
			mtex.insert (mk_pair(m,mk_pair(I->second->dwReference,n) ));
		}
	}

	// dump
#ifdef DEBUG
	{
		xr_multimap<u32,std::pair<u32,shared_str> >::iterator I = mtex.begin	();
		xr_multimap<u32,std::pair<u32,shared_str> >::iterator E = mtex.end		();
		for (; I!=E; I++)
			Msg			("* %4.1f : [%4d] %s",float(I->first)/1024.f, I->second.first, I->second.second.c_str());
	}
#endif
}

void	CResourceManager::Evict()
{
	//	TODO: DX10: check if we really need this method
#if !defined(USE_DX10) && !defined(USE_DX11)
	CHK_DX	(HW.pDevice->EvictManagedResources());
#endif	//	USE_DX10
}
