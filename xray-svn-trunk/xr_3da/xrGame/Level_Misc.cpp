#include "stdafx.h"
#include "Level.h"
#include "xrServer.h"
#include "xrserver_object_base.h"
#include "xrMessages.h"
#include "actor.h"
#include "ParticlesObject.h"
#include "../environment.h"
#include "../igame_persistent.h"

void CLevel::DirectMoveItem(u16 from_id, u16 to_id, u16 what_id)
{
	ClientID _clid;
	_clid.set(1);

	// Reject
	{
		NET_Packet P;
		P.write_start();
		P.w_u16(what_id);
		P.w_u8(1); // send just_before_destroy flag, so physical shell does not activates and disrupts nearby objects

		P.read_start();

		CSE_Abstract* receiver = Server->Server_game_sv_base->get_entity_from_eid(from_id);

		if (receiver)
		{
			R_ASSERT(receiver->owner);

			receiver->OnEvent(P, GE_OWNERSHIP_REJECT, timeServer(), _clid);
		};

		Server->Process_event_reject(P, _clid, timeServer(), from_id, what_id, false);

		cl_Process_Event(from_id, GE_OWNERSHIP_REJECT, P);
	}

	// Grab
	{
		NET_Packet P;
		P.write_start();
		P.w_u16(what_id);

		P.read_start();

		CSE_Abstract* receiver = Server->Server_game_sv_base->get_entity_from_eid(to_id);

		if (receiver)
		{
			R_ASSERT(receiver->owner);

			receiver->OnEvent(P, GE_OWNERSHIP_TAKE, timeServer(), _clid);
		};

		Server->Process_event_ownership(P, _clid, timeServer(), to_id, what_id, false);

		cl_Process_Event(to_id, GE_OWNERSHIP_TAKE, P);
	}
}

#include "actor.h"
#include "actoreffector.h"
#include "holder_custom.h"

void CLevel::ApplyCamera()
{
	inherited::ApplyCamera();

	if(lastApplyCameraVPNear > -1.f)
		lastApplyCamera(lastApplyCameraVPNear);
}

u64 CLevel::GetTimeForEnv()
{
	return Times::GetGameTime();
}

float CLevel::GetTimeFactorForEnv()
{
	return Times::GetGameTimeFactor();
}

void CLevel::PlayEnvironmentEffects()
{
	CActor* actor = smart_cast<CActor*>(Level().CurrentViewEntity());
	BOOL bIndoor = TRUE;

	if (actor)
		bIndoor = actor->renderable_ROS()->get_luminocity_hemi() < 0.05f;

	int data_set = (Random.randF() < (1.f - g_pGamePersistent->Environment().CurrentEnv->weight)) ? 0 : 1;

	CEnvDescriptor* const _env = g_pGamePersistent->Environment().Current[data_set];

	VERIFY(_env);

	CEnvAmbient* env_amb = _env->env_ambient;

	// start sound
	if (env_amb)
	{
		if (EngineTimeU() > ambient_sound_next_time)
		{
			ref_sound* snd = env_amb->get_rnd_sound();

			ambient_sound_next_time = EngineTimeU() + env_amb->get_rnd_sound_time();

			if (snd)
			{
				Fvector	pos;
				float angle = ::Random.randF(PI_MUL_2);

				pos.x = _cos(angle);
				pos.y = 0;
				pos.z = _sin(angle);
				pos.normalize().mul(env_amb->get_rnd_sound_dist()).add(Device.vCameraPosition);
				pos.y += 10.f;

				snd->play_at_pos(0, pos);
			}
		}

		// start effect
		if ((bIndoor == FALSE) && (0 == ambient_particles) && EngineTimeU() > ambient_effect_next_time)
		{
			CEnvAmbient::SEffect* eff = env_amb->get_rnd_effect();

			if (eff)
			{
				g_pGamePersistent->Environment().wind_gust_factor = eff->wind_gust_factor;

				ambient_effect_next_time = EngineTimeU() + env_amb->get_rnd_effect_time();
				ambient_effect_stop_time = EngineTimeU() + eff->life_time;
				ambient_particles = CParticlesObject::Create(eff->particles.c_str(), FALSE, false);

				Fvector pos; pos.add(Device.vCameraPosition, eff->offset);

				ambient_particles->play_at_pos(pos);

				if (eff->sound._handle())
					eff->sound.play_at_pos(0, pos);
			}
		}
	}

	// stop if time exceed or indoor
	if (bIndoor || EngineTimeU() >= ambient_effect_stop_time)
	{
		if (ambient_particles)
			ambient_particles->Stop();

		g_pGamePersistent->Environment().wind_gust_factor = 0.f;
	}

	// if particles not playing - destroy
	if (ambient_particles && !ambient_particles->IsPlaying())
		DestroyParticleInstance(ambient_particles);
}
