#include "StdAfx.h"
#include "../../xr_3da/_d3d_extensions.h"
#include "../../xr_3da/xrLevel.h"
#include "../../xr_3da/igame_persistent.h"
#include "../../xr_3da/environment.h"
#include "../../xrLC/R_light.h"
#include "light_db.h"

extern BOOL	r__dynamicLights_;

CLight_DB::CLight_DB()
{
	createdLightsCnt_ = 0;
	deletedLightsCnt_ = 0;

	ldbTargetViewPortBuffer = nullptr;
}

CLight_DB::~CLight_DB()
{
}

void CLight_DB::Load(IReader* fs) 
{
	createdLightsCnt_ = 0;
	deletedLightsCnt_ = 0;

	IReader* F = 0;

	// Lights itself
	sun_original = NULL;
	sun_adapted = NULL;
	{
		F = fs->open_chunk(fsL_LIGHT_DYNAMIC);

		u32 size = F->length();
		u32 element = sizeof(Flight) + 4;
		u32 count = size / element;

		VERIFY(count * element == size);

		v_static.reserve(count);

		for (u32 i = 0; i<count; i++)
		{
			Flight Ldata;
			CLightSource* L = CreateLight();

			L->flags.bStatic = true;
			L->set_type(IRender_Light::POINT);
			L->set_shadow(true);

			u32 controller = 0;
			F->r(&controller, 4);
			F->r(&Ldata, sizeof(Flight));

			if (Ldata.type == D3DLIGHT_DIRECTIONAL)
			{
				Fvector tmp_R;
				tmp_R.set(1, 0, 0);

				// directional (base)
				sun_original = L;

				L->set_type(IRender_Light::DIRECT);
				L->set_shadow(true);
				L->set_rotation(Ldata.direction, tmp_R);

				// copy to env-sun
				sun_adapted = L = CreateLight();

				L->flags.bStatic = true;
				L->set_type(IRender_Light::DIRECT);
				L->set_shadow(true);
				L->set_rotation(Ldata.direction, tmp_R);
			}
			else
			{
				Fvector tmp_D, tmp_R;
				tmp_D.set(0, 0, -1);// forward
				tmp_R.set(1, 0, 0);	// right

				// point
				v_static.push_back(L);
				L->set_position(Ldata.position);
				L->set_rotation(tmp_D, tmp_R);
				L->set_range(Ldata.range);
				L->set_color(Ldata.diffuse);
				L->set_active(true);
			}
		}

		F->close();
	}

	R_ASSERT2(sun_original && sun_adapted, "Where is sun?");
}

void CLight_DB::LoadHemi()
{
	string_path fn_game;

	if (FS.exist(fn_game, "$level$", "build.lights"))
	{
		IReader* F = FS.r_open(fn_game);

		{
			IReader* chunk = F->open_chunk(1);//Hemispheric light chunk

			if (chunk)
			{
				u32 size = chunk->length();
				u32 element = sizeof(R_Light);
				u32 count = size / element;

				VERIFY(count*element == size);

				v_hemi.reserve(count);

				for (u32 i = 0; i<count; i++)
				{
					R_Light Ldata;

					chunk->r(&Ldata, sizeof(R_Light));

					if (Ldata.type == D3DLIGHT_POINT)
					{
						CLightSource* L = CreateLight();

						L->flags.bStatic = true;
						L->set_type(IRender_Light::POINT);

						Fvector tmp_D, tmp_R;
						tmp_D.set(0, 0, -1); // forward
						tmp_R.set(1, 0, 0);	// right

						// point
						v_hemi.push_back(L);
						L->set_position(Ldata.position);
						L->set_rotation(tmp_D, tmp_R);
						L->set_range(Ldata.range);
						L->set_color(Ldata.diffuse.x, Ldata.diffuse.y, Ldata.diffuse.z);
						L->set_active(true);
#ifndef USE_PORTED_XRLC
						L->set_attenuation_params(Ldata.attenuation0, Ldata.attenuation1, Ldata.attenuation2, 1.0f / (Ldata.range * (Ldata.attenuation0 + Ldata.attenuation1 * Ldata.range + Ldata.attenuation2 * Ldata.range2)));
#else
						//L->set_attenuation_params(Ldata.attenuation0, Ldata.attenuation1, Ldata.attenuation2, Ldata.falloff);	
						L->set_attenuation_params(Ldata.attenuation0, Ldata.attenuation1, Ldata.attenuation2, Ldata.falloff())));
#endif
						L->spatial.s_type = STYPE_LIGHTSOURCEHEMI;
						//R_ASSERT(L->spatial.sector);
					}
				}

				chunk->close();
			}
		}

		FS.r_close(F);

		//Msg("* Hemisphere loaded...");
	}
}

void CLight_DB::Unload()
{
	v_static.clear();
	v_hemi.clear();

	sun_original.destroy();
	sun_adapted.destroy();

	Msg("* Light DB: created lights : %u | deleted lights : %u", createdLightsCnt_, deletedLightsCnt_);
}

CLightSource* CLight_DB::CreateLight()
{
	CLightSource* L = xr_new<CLightSource>();

	L->flags.bStatic = false;
	L->flags.bActive = false;
	L->flags.bShadow = true;

	return L;
}

void CLight_DB::DeleteLight(IRender_Light*& light_to_delete)
{
	if (!light_to_delete)
		return;

	protectLightDeletion_.Enter();

	if (engineState.test(FRAME_PROCESING))
	{
		if (!light_to_delete->alreadyInDestroyQueue_)
		{
			lightsToDelete_.push_back(light_to_delete);

			light_to_delete->alreadyInDestroyQueue_ = true;
		}
	}
	else
	{
		light_to_delete->readyToDestroy_ = true;
		xr_delete(light_to_delete);
	}

	protectLightDeletion_.Leave();

	light_to_delete = nullptr;
}

struct SRemoveLight
{
	CLightSource* l;

	SRemoveLight(CLightSource* light_to_compare)
	{
		l = light_to_compare;
	}

	bool operator() (const CLightSource* light) const
	{
		if (!light)
			return true;

		if (light == l)
			return true;

		return false;
	}
};

void CLight_DB::ClearLightReffs(CLightSource* light_to_delete)
{
	// Delete light from all deffered lights vectors

	for (u8 i = 0; i < 3; i++)
	{

		xr_vector<CLightSource*>& storage = i == 0 ? ldbViewPortBuffer1.rawPackage_.v_point : i == 1 ? ldbViewPortBuffer1.rawPackage_.v_shadowed : ldbViewPortBuffer1.rawPackage_.v_spot;

		storage.erase(std::remove_if(storage.begin(), storage.end(), SRemoveLight(light_to_delete)), storage.end());

#ifdef DEBUG
		for (u32 y = 0; y < storage.size(); y++)
		{
			R_ASSERT(storage[y]);

			if (storage[y])
				R_ASSERT(storage[y]->lightDsGraphBuffer_);
		}
#endif
	}

	for (u8 i = 0; i < 3; i++)
	{
		xr_vector<CLightSource*>& storage = i == 0 ? ldbViewPortBuffer1.rawPackageDeffered_.v_point : i == 1 ? ldbViewPortBuffer1.rawPackageDeffered_.v_shadowed : ldbViewPortBuffer1.rawPackageDeffered_.v_spot;

		storage.erase(std::remove_if(storage.begin(), storage.end(), SRemoveLight(light_to_delete)), storage.end());

#ifdef DEBUG
		for (u32 y = 0; y < storage.size(); y++)
		{
			R_ASSERT(storage[y]);

			if (storage[y])
				R_ASSERT(storage[y]->lightDsGraphBuffer_);
		}
#endif
	}

	for (u8 i = 0; i < 3; i++)
	{
		xr_vector<CLightSource*>& storage = i == 0 ? ldbViewPortBuffer2.rawPackage_.v_point : i == 1 ? ldbViewPortBuffer2.rawPackage_.v_shadowed : ldbViewPortBuffer2.rawPackage_.v_spot;

		storage.erase(std::remove_if(storage.begin(), storage.end(), SRemoveLight(light_to_delete)), storage.end());

#ifdef DEBUG
		for (u32 y = 0; y < storage.size(); y++)
		{
			R_ASSERT(storage[y]);

			if (storage[y])
				R_ASSERT(storage[y]->lightDsGraphBuffer_);
		}
#endif
	}

	for (u8 i = 0; i < 3; i++)
	{
		xr_vector<CLightSource*>& storage = i == 0 ? ldbViewPortBuffer2.rawPackageDeffered_.v_point : i == 1 ? ldbViewPortBuffer2.rawPackageDeffered_.v_shadowed : ldbViewPortBuffer2.rawPackageDeffered_.v_spot;

		storage.erase(std::remove_if(storage.begin(), storage.end(), SRemoveLight(light_to_delete)), storage.end());

#ifdef DEBUG
		for (u32 y = 0; y < storage.size(); y++)
		{
			R_ASSERT(storage[y]);

			if (storage[y])
				R_ASSERT(storage[y]->lightDsGraphBuffer_);
		}
#endif
	}

	for (u32 j = 0; j < VIEW_PORTS_CNT; ++j)
	{
		ViewPortBuffers& vpbuffer = j == 0 ? RImplementation.viewPortBuffer1 : RImplementation.viewPortBuffer2;

		for (u8 i = 0; i < 5; i++)
		{
			xr_vector<CLightSource*>& storage = i == 0 ? vpbuffer.LP_normalNextFrame_.v_point : i == 1 ? vpbuffer.LP_normalNextFrame_.v_shadowed :
				i == 2 ? vpbuffer.LP_normalNextFrame_.v_spot : i == 3 ? vpbuffer.LP_normalNextFrame_.sortedShadowed_ : vpbuffer.LP_normalNextFrame_.sortedShadowedCopy_;

			storage.erase(std::remove_if(storage.begin(), storage.end(), SRemoveLight(light_to_delete)), storage.end());

#ifdef DEBUG
			for (u32 y = 0; y < storage.size(); y++)
			{
				R_ASSERT(storage[y]);

				if (storage[y])
					R_ASSERT(storage[y]->lightDsGraphBuffer_);
			}
#endif
		}

		for (u8 i = 0; i < 5; i++)
		{
			xr_vector<CLightSource*>& storage = i == 0 ? vpbuffer.LP_pendingNextFrame_.v_point : i == 1 ? vpbuffer.LP_pendingNextFrame_.v_shadowed :
				i == 2 ? vpbuffer.LP_pendingNextFrame_.v_spot : i == 3 ? vpbuffer.LP_pendingNextFrame_.sortedShadowed_ : vpbuffer.LP_pendingNextFrame_.sortedShadowedCopy_;

			storage.erase(std::remove_if(storage.begin(), storage.end(), SRemoveLight(light_to_delete)), storage.end());

#ifdef DEBUG
			for (u32 y = 0; y < storage.size(); y++)
			{
				R_ASSERT(storage[y]);

				if (storage[y])
					R_ASSERT(storage[y]->lightDsGraphBuffer_);
			}
#endif
		}

		{
			xr_vector<CLightSource*>& storage = vpbuffer.Lights_LastFrame;

			storage.erase(std::remove_if(storage.begin(), storage.end(), SRemoveLight(light_to_delete)), storage.end());

#ifdef DEBUG
			for (u32 y = 0; y < storage.size(); y++)
			{
				R_ASSERT(storage[y]);

				if (storage[y])
					R_ASSERT(storage[y]->lightDsGraphBuffer_);
			}
#endif
		}
	}


	if (RImplementation.Details)
	{
		xr_vector<CLightSource*>& storage = RImplementation.Details->lightsToCheck_;

		storage.erase(std::remove_if(storage.begin(), storage.end(), SRemoveLight(light_to_delete)), storage.end());

#ifndef DEBUG
		for (u32 y = 0; y < storage.size(); y++)
		{
			R_ASSERT(storage[y]);

			if (storage[y])
				R_ASSERT(storage[y]->lightDsGraphBuffer_);
		}
#endif
	}
}

void CLight_DB::DeleteQueue()
{
	protectLightDeletion_.Enter();

#ifdef DEBUG
	u32 cnt = lightsToDelete_.size();
#endif

	for (u32 it = 0; it < lightsToDelete_.size(); it++)
	{
		R_ASSERT2(lightsToDelete_[it], "Deleting light, that is already deleted?");

		lightsToDelete_[it]->readyToDestroy_ = true;
		xr_delete(lightsToDelete_[it]);
	}

	lightsToDelete_.clear();

	protectLightDeletion_.Leave();

#ifdef DEBUG
	if (cnt)
		Msg("* Post frame light deletion: %u lights deleted", cnt);
#endif
}

void CLight_DB::add_light(CLightSource* L)
{
	LightViewProtbuffer* lightvp = &(RImplementation.currentViewPort == MAIN_VIEWPORT ? L->lightViewPortBuffer1 : L->lightViewPortBuffer2);

	if (CurrentFrame() == lightvp->frame_render)
		return;

	lightvp->frame_render = CurrentFrame();

	if (RImplementation.o.noshadows)
		L->flags.bShadow = FALSE;
	
	if (L->flags.bStatic && !ps_r2_ls_flags.test(R2FLAG_R1LIGHTS))
		return;
	else if (!r__dynamicLights_)
		return;
	
	L->export_(ldbTargetViewPortBuffer->rawPackage_);
}

void CLight_DB::UpdateLight(ViewPort viewport)
{
	if (viewport == MAIN_VIEWPORT)
		ldbTargetViewPortBuffer = &ldbViewPortBuffer1;
	else if(viewport == SECONDARY_WEAPON_SCOPE)
		ldbTargetViewPortBuffer = &ldbViewPortBuffer2;

	// set sun params
	if (sun_original && sun_adapted)
	{
		CLightSource* _sun_original = (CLightSource*)sun_original._get();
		CLightSource* _sun_adapted = (CLightSource*)sun_adapted._get();

		CEnvDescriptor&	E = *g_pGamePersistent->Environment().CurrentEnv;

		VERIFY(_valid(E.sun_dir));

#ifdef _DEBUG
		if(E.sun_dir.y>=0)
		{
//			Log("sect_name", E.sect_name.c_str());
			Log("E.sun_dir", E.sun_dir);
			Log("E.wind_direction",E.wind_direction);
			Log("E.wind_velocity",E.wind_velocity);
			Log("E.sun_color",E.sun_color);
			Log("E.rain_color",E.rain_color);
			Log("E.rain_density",E.rain_density);
			Log("E.fog_distance",E.fog_distance);
			Log("E.fog_density",E.fog_density);
			Log("E.fog_color",E.fog_color);
			Log("E.far_plane",E.far_plane);
			Log("E.sky_rotation",E.sky_rotation);
			Log("E.sky_color",E.sky_color);
		}
#endif

		Fvector OD, OP, AD, AP;

		OD.set(E.sun_dir).normalize();
		OP.mad(Device.vCameraPosition, OD, -500.f);
		AD.set(0, -.75f, 0).add(E.sun_dir);

		// for some reason E.sun_dir can point-up
		int counter = 0;

		while (AD.magnitude() < 0.001 && counter < 10)
		{
			AD.add(E.sun_dir); counter++;
		}

		AD.normalize();
		AP.mad(Device.vCameraPosition, AD, -500.f);
		sun_original->set_rotation(OD, _sun_original->right);
		sun_original->set_position(OP);
		sun_original->set_color(E.sun_color.x, E.sun_color.y, E.sun_color.z);
		sun_original->set_range(600.f);
		sun_adapted->set_rotation(AD, _sun_adapted->right);
		sun_adapted->set_position(AP);

		float lumscale = E.sun_lumscale * ps_r2_sun_lumscale;

		sun_adapted->set_color(E.sun_color.x*lumscale, E.sun_color.y*lumscale, E.sun_color.z*lumscale);
		sun_adapted->set_range(600.f);

		if (!::Render->is_sun_static())
		{
			sun_adapted->set_rotation(OD, _sun_original->right);
			sun_adapted->set_position(OP);
		}
	}

	ldbTargetViewPortBuffer->rawPackageDeffered_ = ldbTargetViewPortBuffer->rawPackage_;

	// Clear selection
	ldbTargetViewPortBuffer->rawPackage_.ClearPackage();
}
