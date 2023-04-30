#include "StdAfx.h"
#include "../xrRender/light.h"
#include "../cl_intersect.h"

extern float CPU_wait_GPU_lastFrame_;

u32 occDisabledUntil_ = 0;
bool occTemporaryDisabled_ = false;

#define VISIBLE_INTERVAL_MIN 220
#define VISIBLE_INTERVAL_MAX 440

#define INVISIBLE_INTERVAL_MIN 100
#define INVISIBLE_INTERVAL_MAX 120

#define MAX_CULL_FRAGMENTS 4

#define GPU_MAX_WAIT 0.2f
#define DISABLE_TIME 200

void CLightSource::vis_prepare()
{
	if (int(indirect_photons) != ps_r2_GI_photons)
		gi_generate();

	u32	engine_time = EngineTimeU();

	if (engine_time < vis.timeToTestOcc_)
		return;

	if (ps_r2_ls_flags.test(R2FLAG_GPU_OCC_OPTIMIZATION)) // If the GPU is causing Main thread to wait it, don't try to do even more work on it - skip the GPU occ testing, to avoid frame rate spikings
	{
		if (occTemporaryDisabled_)
		{
			if (CPU_wait_GPU_lastFrame_ > GPU_MAX_WAIT) // prolong disabling, if gpu is still striving its ass off
				occDisabledUntil_ = engine_time + DISABLE_TIME;

			if (occDisabledUntil_ > engine_time) // if we are still disabled - postpone the test and make light visible
			{
				vis.visible = true;
				vis.pending = false;

				vis.timeToTestOcc_ = engine_time + ::Random.randI(500, 600);

				return;
			}
			else
				occTemporaryDisabled_ = false; // enable the GPU occ testing, if we passed the "disabled" time without gpu slowdowns
		}
		else if (CPU_wait_GPU_lastFrame_ > GPU_MAX_WAIT) // Postpone the test and disable gpu light occ for some time
		{
			vis.visible = true;
			vis.pending = false;

			vis.timeToTestOcc_ = engine_time + ::Random.randI(500, 600);

			// Temporary disable occ, to avoid frame rate spikes
			occTemporaryDisabled_ = true;
			occDisabledUntil_ = engine_time + DISABLE_TIME;

			return;
		}
	}

	float safe_area = VIEWPORT_NEAR;
	{
		float a0 = deg2rad(Device.fFOV*Device.fASPECT / 2.f);
		float a1 = deg2rad(Device.fFOV / 2.f);
		float x0 = VIEWPORT_NEAR / _cos(a0);
		float x1 = VIEWPORT_NEAR / _cos(a1);
		float c = _sqrt(x0*x0 + x1 * x1);

		safe_area = _max(_max(VIEWPORT_NEAR, _max(x0, x1)), c);
	}

	bool skiptest = false;

	if (ps_r2_ls_flags.test(R2FLAG_EXP_DONT_TEST_UNSHADOWED) && !flags.bShadow)
		skiptest = true;

	if (ps_r2_ls_flags.test(R2FLAG_EXP_DONT_TEST_SHADOWED) && flags.bShadow)
		skiptest = true;

	// Skip test because of console commands flags or if light is in always visible area
	if (skiptest || Device.vCameraPosition.distance_to(spatial.sphere.P) <= (spatial.sphere.R * 1.01f + safe_area))
	{
		vis.visible = true;
		vis.pending = false;

		vis.timeToTestOcc_ = engine_time + ::Random.randI(500, 600);

		return;
	}

	// Testing
	vis.pending = true;

	xform_calc();

	RCache.set_xform_world(m_xform);

	vis.query_order = RImplementation.occq_begin(vis.query_id);

	//	Hack: Igor. Light is visible if it's frutum is visible. (Only for volumetric)
	//	Hope it won't slow down too much since there's not too much volumetric lights
	//	TODO: sort for performance improvement if this technique hurts

	if ((flags.type == IRender_Light::SPOT) && flags.bShadow && flags.bVolumetric)
		RCache.set_Stencil(FALSE);
	else
		RCache.set_Stencil(TRUE, D3DCMP_LESSEQUAL, 0x01, 0xff, 0x00);

	RImplementation.Target->draw_volume(this);

	RImplementation.occq_end(vis.query_id);
}

void CLightSource::vis_update()
{
	if (!vis.pending)
		return;

	u32	engine_time = EngineTimeU();

	u64 fragments = RImplementation.occq_get(vis.query_id);

	vis.visible = (fragments > MAX_CULL_FRAGMENTS);
	vis.pending = false;

	if (vis.visible) // Visible: Schedule for large interval
	{
		vis.timeToTestOcc_ = engine_time + ::Random.randI(VISIBLE_INTERVAL_MIN, VISIBLE_INTERVAL_MAX);
	}
	else // Invisible: Schedule for shorter interval, to avoid lights poping up, when rotating camera
	{
		vis.timeToTestOcc_ = engine_time + ::Random.randI(INVISIBLE_INTERVAL_MIN, INVISIBLE_INTERVAL_MAX);
	}
}
