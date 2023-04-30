#include "stdafx.h"
#include "LightTrack.h"
#include "../../include/xrRender/RenderVisual.h"
#include "../../xr_3da/xr_object.h"
#include "../../xr_3da/xrGame/clsid_game.h"

#include "../../xr_3da/igame_persistent.h"
#include "../../xr_3da/environment.h"

extern bool	pred_energy(const CROS_impl::Light& L1, const CROS_impl::Light& L2);

ENGINE_API extern float fActor_Lum; //temp monitoring

void CROS_impl::update_ai_luminocity(IRenderable* O)
{
	if (CurrentFrame() < aiLumNextFrame_)
	{
		return;
	}

	aiLumNextFrame_ = CurrentFrame() + (u32)::Random.randI(4, 10); //optimization: no need to update this code each frame;

	CTimer time; // Timer to measure perfomance inpact
	time.Start();

	CObject* _object = dynamic_cast<CObject*>(O);
	Fvector	accumulated, hemi_influence, sun_influence;

	accumulated = hemi_influence = sun_influence = { 0, 0, 0 };

	//---Part 1: Find out environment influenced luminocity
	{

		CEnvDescriptor&	desc = *g_pGamePersistent->Environment().CurrentEnv;

		accumulated =		{ desc.ambient.x, desc.ambient.y, desc.ambient.z };
		hemi_influence =	{ desc.hemi_color.x, desc.hemi_color.y, desc.hemi_color.z };
		sun_influence =		{ desc.sun_color.x, desc.sun_color.y, desc.sun_color.z };

		//Msg("hemi_sky_tests_coef = %f", hemi_sky_tests_coef);

		hemi_influence.mul(hemi_sky_tests_coef);

		protectSunSetGet_.Enter();
		sun_influence.mul(sun_smooth);
		protectSunSetGet_.Leave();


		accumulated.add(hemi_influence);
		accumulated.add(sun_influence);


		float result = _max(accumulated.x, _max(accumulated.y, accumulated.z));
		clamp(result, 0.f, 1.f);

		protectAILumSetGet_.Enter();
		ai_luminocity = result;
		protectAILumSetGet_.Leave();

		if (result > 0.95f) //optimization: exit calculations if lum is close to 1.0
		{
			return;
		}
	}


	//---Part 2: If the result is still less then 1.0, then add luminocity from light sources(more heavysome procedure)
	{
		vis_data &vis = O->renderable.visual->getVisData();
		float object_radius = vis.sphere.R;

		Fvector bb_size = { object_radius, object_radius, object_radius };
		Fvector	position;


		O->renderable.xform.transform_tiny(position, vis.sphere.P);


		//Spatials update
		g_SpatialSpace->q_box(Spatials, 0, STYPE_LIGHTSOURCE, position, bb_size/*???*/); //find all light sources in radius


		//Light source pool update
		//Find Lights from spatials pool
		float dt = TimeDelta();

		for (u32 i = 0; i < Spatials.size(); i++)
		{
			ISpatial* spatial = Spatials[i];
			CLightSource* source = (CLightSource*)(spatial->dcast_Light());

			R_ASSERT(source);

			float total_range = object_radius + source->range;

			if (position.distance_to(source->position) < total_range)
			{
				// Search
				bool add = true;

				for (xr_vector<Item>::iterator I = Ai_Unsorted_Items.begin(); I != Ai_Unsorted_Items.end(); I++)
					if (source == I->source)
					{
						I->frame_touched = CurrentFrame();
						add = false;
					}

				// Register new
				if (add)
				{
					Ai_Unsorted_Items.push_back(Item());
					Item& L = Ai_Unsorted_Items.back();

					L.frame_touched = CurrentFrame();
					L.source = source;

					L.cache.verts[0].set(0, 0, 0);
					L.cache.verts[1].set(0, 0, 0);
					L.cache.verts[2].set(0, 0, 0);

					L.test = 0.f;
					L.energy = 0.f;
				}
			}
		}


		Ai_Tracking_Lights.clear();

		//Sort only influincing lights
		for (s32 id = 0; id < s32(Ai_Unsorted_Items.size()); id++)
		{
			// remove untouched lights
			xr_vector<CROS_impl::Item>::iterator I = Ai_Unsorted_Items.begin() + id;

			if (I->frame_touched != CurrentFrame())
			{
				Ai_Unsorted_Items.erase(I);
				id--;

				continue;
			}

			// Trace visibility
			Fvector P, D;
			P = position;

			CLightSource* Ligth_to_sort = I->source;

			Fvector &LP = Ligth_to_sort->position;

			float amount = 0;

			// point/spot
			float f = D.sub(P, LP).magnitude();

			CObject* ray_result = nullptr;

#pragma todo("need to move raycast origin point a little towards the _object, so that raycast does not hit another light source or the 'lamp cover' mesh")
			BOOL res = g_pGameLevel->ObjectSpace.RayTestD(LP, D.div(f), f, collide::rqtBoth, &I->cache, _object, ray_result);

			if (res)
				amount -= lt_dec;
			else
				amount += lt_inc;

			I->test += amount * dt;
			clamp(I->test, -.5f, 1.f);

			I->energy = .9f * I->energy + .1f * I->test;

			float E = I->energy * Ligth_to_sort->color.intensity();
			if (E > EPS)
			{
				// Select light
				Ai_Tracking_Lights.push_back(CROS_impl::Light());
				CROS_impl::Light& L = Ai_Tracking_Lights.back();

				L.source = Ligth_to_sort;
				L.color.mul_rgb(Ligth_to_sort->color, I->energy / 2);
				L.energy = I->energy / 2;

				if (!Ligth_to_sort->flags.bStatic)
				{
					L.color.mul_rgb(.5f);
					L.energy *= .5f;
				}
			}
		}

		// Sort lights by importance
		std::sort(Ai_Tracking_Lights.begin(), Ai_Tracking_Lights.end(), pred_energy);

		//Actual Light luminocity update
		for (u32 i = 0; i < Ai_Tracking_Lights.size(); i++)
		{
			Fvector lacc = { 0, 0, 0 };
			CLightSource* light_item = Ai_Tracking_Lights[i].source;

			float d = light_item->position.distance_to(position);

#pragma todo("This not very accurate, since it does not check light direction")
			float dist_koef = (1.0f - d / (light_item->range + object_radius)); // distance influence
			clamp(dist_koef, 0.0001f, 1.f);

			lacc.x += light_item->color.r * dist_koef;
			lacc.y += light_item->color.g * dist_koef;
			lacc.z += light_item->color.b * dist_koef;

			accumulated.add(lacc);

			float result = _max(accumulated.x, _max(accumulated.y, accumulated.z));

			protectAILumSetGet_.Enter();

			ai_luminocity += result;

			clamp(ai_luminocity, 0.01f, 1.f);

			if (ai_luminocity > 0.95f) //optimization, exit calculations if lum is close to 1.0
			{
				protectAILumSetGet_.Leave();

				return;
			}

			protectAILumSetGet_.Leave();
		}
	}
}