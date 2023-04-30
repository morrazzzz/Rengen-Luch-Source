// Texture loading mechanism

#include "stdafx.h"

#include "ResourceManager.h"
#include "dxRenderDeviceRender.h"

extern ENGINE_API BOOL mtUseCustomAffinity_;

xr_vector<CTexture*> textures_; // a copy of textures pointers pool
AccessLock txLoadingMutex1_, protexctTexturesPool_; //these locks guard critical thread global vars, which can be raced by threads and go wrong

u32 threadsInWork_ = 0;

struct TexturesThreadParams
{
	u8 thread_id;

	TexturesThreadParams(u8 thread_index) { thread_id = thread_index; };
};

void CResourceManager::TextureLoadingThread(void* p)
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

	if (mtUseCustomAffinity_)
		SetThreadAffinityMask(GetCurrentThread(), CPU::GetSecondaryThreadBestAffinity());

	TexturesThreadParams* t_p = (TexturesThreadParams*)p;
	u8 thread_number = t_p->thread_id;

	bool continue_looping = false;
	xr_vector<CTexture*> textures_to_load;

	Msg("* Texture Thread #%u is spawned", thread_number);

ContinueLooping:
	protexctTexturesPool_.Enter();
	for (u32 i = 0; i < 50 && textures_.size(); i++)
	{
		textures_to_load.push_back(textures_.back());
		textures_.pop_back();
	}
	protexctTexturesPool_.Leave();


	for (u32 i = 0; i < textures_to_load.size(); i++)
	{
		if (textures_to_load[i]) //prevent empty texture crash
		{
			//LPCSTR texturename = *textures[i]->cName ? textures[i]->cName.c_str() : "Error in texture name; Texture will be skipped";
			//Msg("*Thread# %u: Loading texture# %u with name %s", threadnumber, i, texturename);
			textures_to_load[i]->LoadTexture(true);
		}
	}


	textures_to_load.clear();

	// Check if there are more textures to load
	protexctTexturesPool_.Enter();

	if (textures_.size())
		continue_looping = true;
	else
		continue_looping = false;

	protexctTexturesPool_.Leave();

	if (continue_looping)
		goto ContinueLooping; // go back to loading

	Msg("* Texture Thread #%u: Done!", thread_number);

	txLoadingMutex1_.Enter();

	threadsInWork_--;

	if (!threadsInWork_) // this check determines if the thread was the last one to finish and then clear temporary array
	{
		Msg("* MT Texture Loading Finished");
		
		DEV->LoadingTexturesSet(false);

		protexctTexturesPool_.Enter();

		textures_.clear();

		protexctTexturesPool_.Leave();
	}

	txLoadingMutex1_.Leave();

	xr_delete(t_p); // delete thread params from mem
}

void CResourceManager::DeferredUpload(BOOL multithreaded)
{
	if (!RDEVICE.b_is_Ready){ Log("! Texture loading: Device Not Ready, skipping texture loading"); return; }

	//Sync to not interupt prefetching, if its not finished yet (Possible on uneven cpus+ssd perfomance)
	if (!prefetching_in_progress)
	{
		SyncPrefetchLoading();
		g_pGamePersistent->LoadTitle("st_loading_textures", "Loading Textures");
	}

	LoadingTexturesSet(true);

	if (multithreaded) // !!!Multithreaded method
	{
		for (map_TextureIt t = m_textures.begin(); t != m_textures.end(); t++) // Copy pointers to textures pool, to avoid pool access races
		{
			textures_.push_back(t->second); // pushing all textures to a tempolary pool that will be processed by threads
		}

		std::random_shuffle(textures_.begin(), textures_.end()); // for even spread of workload

		u32 threads_to_spawn = CPU::GetLogicalCoresNum() - 1; // Leave one thread for system and other background apps
		_min(threads_to_spawn, 2); // Minimum 2 threads

		threadsInWork_ = threads_to_spawn;

		Msg("* Texture count = %u, Threads cnt = %u", textures_.size(), threads_to_spawn);

		for (u8 y = 1; y <= threads_to_spawn; y++) // Begin spawning threads
		{
			Msg("* Spawning texture thread# %u", y);

			string128 str;
			xr_sprintf(str, "Texture Loading Thread %u", y);

			TexturesThreadParams* t_p = xr_new<TexturesThreadParams>(y); // need to create it in heap mem, since we need to store it untill thread is fully spawned

			thread_spawn(TextureLoadingThread, str, 0, t_p);
		}

		Msg("* Spawning %u threads: Done!", threads_to_spawn);
	}
	else // !!!Single threaded method
	{
		for (map_TextureIt t = m_textures.begin(); t != m_textures.end(); t++)
		{
			t->second->LoadTexture(false);
		}

		LoadingTexturesSet(false);
	}
}

void CResourceManager::SyncPrefetchLoading()
{
	protexctTexturesPool_.Enter();

	bool need_sync = textures_.size() > 0;

	protexctTexturesPool_.Leave();

	g_pGamePersistent->LoadTitle("st_tx_loading_sync", "Syncing with prefetch threads, if any");

	if (need_sync)
	{
		while (true)
		{
			protexctTexturesPool_.Enter();

			need_sync = textures_.size() > 0;

			protexctTexturesPool_.Leave();

			if (!need_sync)
				break;

			Sleep(100);
		}
	}
}

void CResourceManager::SyncTexturesLoading()
{
	txLoadingMutex1_.Enter();

	u32 threads_in_work = threadsInWork_;

	txLoadingMutex1_.Leave();

	if (threads_in_work > 0)
	{
		Msg("# Syncing texture loading threads");

		while (true)
		{
			txLoadingMutex1_.Enter();

			threads_in_work = threadsInWork_;

			txLoadingMutex1_.Leave();

			if (!threads_in_work)
				break;

			Sleep(100);
		}

		Msg("# Syncing texture loading threads: Done!");
	}
}

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