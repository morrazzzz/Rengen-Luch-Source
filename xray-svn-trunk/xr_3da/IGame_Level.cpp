#include "stdafx.h"
#include "igame_level.h"
#include "igame_persistent.h"

#include "x_ray.h"
#include "BaseInstanceClasses.h"
#include "customHUD.h"
#include "render.h"
#include "gamefont.h"
#include "xrLevel.h"
#include "CameraManager.h"
#include "xr_object.h"
#include "feel_sound.h"

#include "ps_instance.h"

#include "securom_api.h"

ENGINE_API IGame_Level* g_pGameLevel = nullptr;

extern ENGINE_API BOOL mt_texture_loading;

IGame_Level::IGame_Level()
{
	m_pCameras					= xr_new <CCameraManager>(true);
	g_pGameLevel				= this;
	pLevel						= NULL;
	bReady						= false;
	pCurrentEntity				= NULL;
	pCurrentViewEntity			= NULL;

	lastApplyCameraVPNear		= -1.f;

	Device.DumpResourcesMemoryUsage();
}

extern BOOL keep_necessary_textures;

IGame_Level::~IGame_Level()
{
	if (keep_necessary_textures == TRUE)
	{
		Device.m_pRender->ResourcesStoreNecessaryTextures();
	}

	xr_delete					(pLevel);

	// Render-level unload
	Render->level_Unload		();
	xr_delete					(m_pCameras);

	// Unregister
	Device.seqRender.Remove		(this);
	Device.seqFrame.Remove		(this);
	Device.seqFrameEnd.Remove	(this);
	Device.seqFrameBegin.Remove	(this);

	CCameraManager::ResetPP		();

	Sound->set_geometry_occ		(NULL);
	Sound->set_handler			(NULL);

	Device.DumpResourcesMemoryUsage();

	u32 m_base = 0, c_base = 0, m_lmaps = 0, c_lmaps = 0;

	if (Device.m_pRender) 
		Device.m_pRender->ResourcesGetMemoryUsage(m_base, c_base, m_lmaps, c_lmaps);

	Msg("* [ D3D ]: textures[%d K]", (m_base+m_lmaps) / 1024);

	Particles.ClearParticles(true); // just in case
}

void IGame_Level::net_Stop()
{
	R_ASSERT(!engineState.test(FRAME_PROCESING));

	Engine.Sheduler.SchedulerFrameEnd();
	ISpatial_DB::OnFrameEnd();

	for (int i = 0; i < 6; i++)
	{
		Objects.ObjectListOnFrameEnd();
		Objects.ObjectListUpdate(true, true);
		Objects.DestroyOldObjects(true);
	}

	// Destroy all objects
	Objects.Unload				();
	Particles.Unload			();

	IR_Release					();

	bReady						= false;	

	lastApplyCameraVPNear		= -1.f;
}

void __stdcall _sound_event(ref_sound_data_ptr S, float range)
{
	if (g_pGameLevel && S && S->feedback)
		g_pGameLevel->SoundEvent_Register(S, range);
}

static void __stdcall build_callback(Fvector* V, int Vcnt, CDB::TRI* T, int Tcnt, void* params)
{
	g_pGameLevel->Load_GameSpecific_CFORM(T, Tcnt);
}

BOOL IGame_Level::Load(u32 dwNum) 
{
	// Initialize level data
	pApp->Level_Set				(dwNum);
	string_path					temp;

	if (!FS.exist(temp, "$level$", "level.ltx"))
		Debug.fatal	(DEBUG_INFO, "Can't find level configuration file '%s'.", temp);

	pLevel						= xr_new <CInifile>	(temp);
	
	// Open
	g_pGamePersistent->LoadTitle("st_opening_stream", "Opening Level Data");
	IReader* LL_Stream			= FS.r_open	("$level$", "level");
	IReader	&fs					= *LL_Stream;

	// Header
	hdrLEVEL					H;

	fs.r_chunk_safe				(fsL_HEADER, &H, sizeof(H));
	R_ASSERT2					(XRCL_PRODUCTION_VERSION == H.XRLC_version, "Incompatible level version.");

	// CForms
	g_pGamePersistent->LoadTitle("st_loading_cform", "Loading CForms");
	ObjectSpace.Load			(build_callback);

	Sound->set_geometry_occ		(ObjectSpace.GetStaticModel());
	Sound->set_handler			(_sound_event);

	// HUD + Environment
	if(!g_hud)
		g_hud					= (CCustomHUD*)NEW_INSTANCE	(CLSID_HUDMANAGER);

	// Render-level Load
	Render->level_Load			(LL_Stream);

	// Objects
	g_pGamePersistent->Environment().mods_load();

	R_ASSERT					(Load_GameSpecific_Before());

	Particles.Load				();
	Objects.Load				();

	R_ASSERT					(Load_GameSpecific_After ());

	// Done
	FS.r_close					(LL_Stream);

	bReady						= true;

	IR_Capture();

	Device.seqRender.Add		(this);
	Device.seqFrame.Add			(this);
	Device.seqFrameBegin.Add	(this);
	Device.seqFrameEnd.Add		(this, REG_PRIORITY_NORMAL + 2);

	return TRUE;
}

bool IGame_Level::LoadTextures()
{
	if (!psDeviceFlags.test(rsSkipTextureLoading))
	{
		Device.m_pRender->DeferredLoad(FALSE);
		Device.m_pRender->ResourcesDeferredUpload(mt_texture_loading);

		if (!strstr(Core.Params, "-no_texture_sync"))
		{
			Device.m_pRender->SyncTexturesLoadingProcess();
			LL_CheckTextures();
		}
	}

	return true;
}

void IGame_Level::OnRender() 
{
	RenderViewPort(Render->currentViewPort);
}

void IGame_Level::RenderViewPort(u32 viewport)
{
	Render->BeforeViewRender((ViewPort)viewport); //--#SM+#-- +SecondVP+

	Render->CalculateRendering((ViewPort)viewport);
	Render->RenderFrame((ViewPort)viewport);

	Render->AfterViewRender((ViewPort)viewport); //--#SM+#-- +SecondVP+
}

void IGame_Level::ApplyCamera()
{

}

void IGame_Level::OnFrameBegin()
{

}

void IGame_Level::OnFrame() // Overriden by xrGame CLevel::OnFrame
{
#ifdef MEASURE_ON_FRAME
	CTimer measure_on_frame; measure_on_frame.Start();
#endif


	R_ASSERT(Particles.particlesToDelete.empty());
	VERIFY(bReady);

	// this needs to go first to be able to send them to aux thread as soon as possible
	Particles.UpdateParticles();

	if (g_pGamePersistent->levelEnvironment && (!Device.Paused() || Device.dwPrecacheFrame))
	{
		g_pGamePersistent->Environment().SetEnvTime(GetTimeForEnv());
		g_pGamePersistent->Environment().SetEnvTimeFactor(GetTimeFactorForEnv());

		g_pGamePersistent->Environment().OnFrame();
	}

	if (!Device.Paused())
	{
		// Spreaded across frames updates
		Engine.Sheduler.SchedulerUpdate();

		// Per frame update of valuable objects
		Objects.ObjectListUpdate(false, false);
	}

	if (g_hud)
		g_hud->OnFrame();

	// Ambience Sounds
	PlayRandomAmbientSnd();
	// Update weathers ambient effects
	PlayEnvironmentEffects();


#ifdef MEASURE_ON_FRAME
	Device.Statistic->onframe_ILevel_ += measure_on_frame.GetElapsed_ms_f();
#endif
}

void IGame_Level::OnFrameEnd()
{
	Objects.ObjectListOnFrameEnd();
	Engine.Sheduler.SchedulerFrameEnd();

	ISpatial_DB::OnFrameEnd();

	Particles.DeleteParticleQueue();
	Objects.DestroyOldObjects(false);

	lastApplyCameraVPNear = -1.f; // to avoid unhandeled situations, when camera is not updated at next frame(ex. level restart)
}

void IGame_Level::SetEntity(CObject* O)
{
	if (pCurrentEntity)
		pCurrentEntity->On_LostEntity();
	
	if (O)
		O->On_SetEntity();

	pCurrentEntity=pCurrentViewEntity=O;
}

void IGame_Level::SetViewEntity(CObject* O)
{
	if (pCurrentViewEntity)
		pCurrentViewEntity->On_LostEntity();

	if (O)
		O->On_SetEntity();

	pCurrentViewEntity=O;
}

void IGame_Level::SoundEvent_Register(ref_sound_data_ptr S, float range)
{
	if (!engineState.test(LEVEL_LOADED))
		return;

	if (!S)
		return;

	if (S->g_object && S->g_object->getDestroy())
	{
		S->g_object = 0;

		return;
	}

	if (!S->feedback)
		return;

	clamp(range, 0.1f, 500.f);

	const CSound_params* p = S->feedback->get_params();
	Fvector snd_position = p->position;

	if(S->feedback->is_2D())
	{
		snd_position.add(Sound->listener_position());
	}

	VERIFY(p && _valid(range));

	range = _min(range, p->max_ai_distance);

	VERIFY(_valid(snd_position));
	VERIFY(_valid(p->max_ai_distance));
	VERIFY(_valid(p->volume));

	// Query objects
	Fvector bb_size	= {range, range, range};

	g_SpatialSpace->q_box(snd_ER, 0, STYPE_REACTTOSOUND, snd_position, bb_size);

	// Iterate
	xr_vector<ISpatial*>::iterator it = snd_ER.begin();
	xr_vector<ISpatial*>::iterator end = snd_ER.end();

	for (; it != end; it++)
	{
		Feel::Sound* L = (*it)->dcast_FeelSound();

		if (!L)
			continue;

		CObject* CO = (*it)->dcast_CObject();
		
		VERIFY(CO);

		if (CO->getDestroy())
			continue;

		// Energy and signal
		VERIFY(_valid((*it)->spatial.sphere.P));

		float dist = snd_position.distance_to((*it)->spatial.sphere.P);

		if (dist > p->max_ai_distance)
			continue;

		VERIFY(_valid(dist));
		VERIFY2(!fis_zero(p->max_ai_distance), S->handle->file_name());

		float Power = (1.f - dist / p->max_ai_distance) * p->volume;

		VERIFY(_valid(Power));

		if (Power > EPS_S)
		{
			float occ = Sound->get_occlusion_to((*it)->spatial.sphere.P, snd_position);

			VERIFY(_valid(occ));

			Power *= occ;

			if (Power > EPS_S)
			{
				_esound_delegate D = { L, S, Power };

				snd_Events.push_back(D);
			}
		}
	}

	snd_ER.clear_not_free();
}

void IGame_Level::SoundEvent_Dispatch()
{
	while(!snd_Events.empty())
	{
		_esound_delegate& D = snd_Events.back();

		VERIFY(D.dest && D.source);

		if (D.source->feedback)
		{
			D.dest->feel_sound_new(D.source->g_object, D.source->g_type, D.source->g_userdata, D.source->feedback->is_2D() ? Device.vCameraPosition : D.source->feedback->get_params()->position, D.power);
		}

		snd_Events.pop_back();
	}
}

void IGame_Level::SoundEvent_OnDestDestroy(Feel::Sound* obj)
{
	struct rem_pred
	{
		rem_pred(Feel::Sound* obj) : m_obj(obj) {}

		bool operator () (const _esound_delegate& d)
		{
			return d.dest == m_obj;
		}

	private:
		Feel::Sound* m_obj;
	};

	snd_Events.erase(std::remove_if(snd_Events.begin(), snd_Events.end(), rem_pred(obj)), snd_Events.end());
}

void IGame_Level::PlayRandomAmbientSnd()
{
	if (Sounds_Random.size() && (EngineTimeU() > Sounds_Random_dwNextTime))
	{
		Sounds_Random_dwNextTime = EngineTimeU() + ::Random.randI(10000, 20000);
		Fvector	pos;

		pos.random_dir().normalize().mul(::Random.randF(30, 100)).add(Device.vCameraPosition);

		int id = ::Random.randI(Sounds_Random.size());

		if (Sounds_Random_Enabled)
		{
			Sounds_Random[id].play_at_pos(0, pos, 0);
			Sounds_Random[id].set_volume(1.f);
			Sounds_Random[id].set_range(10, 200);
		}
	}
}

void IGame_Level::DestroyEnvironment()
{
	g_pGamePersistent->Environment().OnDeviceDestroy();
	g_pGamePersistent->Environment().unload();
}

void IGame_Level::CreateEnvironment()
{
	g_pGamePersistent->Environment().load();
	g_pGamePersistent->Environment().OnDeviceCreate();
	g_pGamePersistent->Environment().bNeed_re_create_env = TRUE;
}

void IGame_Level::PlayEnvironmentEffects()
{

}
