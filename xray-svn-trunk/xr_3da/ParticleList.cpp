
#include "stdafx.h"
#include "ParticleList.h"
#include "PS_instance.h"
#include "IGame_Level.h"

CParticleList::CParticleList()
{
}

CParticleList::~CParticleList()
{
}

void CParticleList::Load()
{

}

void CParticleList::Unload()
{
	// clear all particles. Make sure to delete all other storages before or clear them after
	ClearParticles(true);
}

void CParticleList::UpdateParticles()
{
	Device.Statistic->Particles_starting = particlesWaitingToPlay.size();
	Device.Statistic->Particles_active = allParticles.size();
	Device.Statistic->Particles_destroy = particlesToDelete.size();

	// Play req particle systems
	while (particlesWaitingToPlay.size())
	{
		CPS_Instance* psi = particlesWaitingToPlay.back();
		particlesWaitingToPlay.pop_back();

		psi->Play(false);
	}

	for (auto p_it = allParticles.begin(); allParticles.end() != p_it; ++p_it)
	{
		if (*p_it)
		{
			(*p_it)->Update();
		}
	}
}

void CParticleList::DeleteParticleQueue()
{
	protectParticleDeletion.Enter();

#ifdef DEBUG
	u32 cnt = particlesToDelete.size();
#endif

	for (u32 it = 0; it < particlesToDelete.size(); it++)
	{
		R_ASSERT2(particlesToDelete[it], "Deleting particle, that is already deleted?");

		particlesToDelete[it]->readyToDestroy = true;
		xr_delete(particlesToDelete[it]);
	}

	particlesToDelete.clear();

	protectParticleDeletion.Leave();

#ifdef DEBUG
	if (cnt)
		Msg("* Post frame particles deletion: %u particles deleted", cnt);
#endif
}

void CParticleList::ClearParticles(const bool &all_particles)
{
	R_ASSERT(!engineState.test(FRAME_PROCESING));

	// destroy PSs
	for (auto p_it = staticParticles.begin(); staticParticles.end() != p_it; ++p_it)
		DestroyParticleInstance(*p_it);

	staticParticles.clear();

#ifndef _EDITOR
	particlesWaitingToPlay.clear();

	// delete active particles
	if (all_particles)
	{
		for (auto it = allParticles.begin(); it != allParticles.end(); ++it)
			DestroyParticleInstance(*it, true);
	}
	else
	{
		u32 active_size = allParticles.size();
		CPS_Instance** I = (CPS_Instance**)_alloca(active_size * sizeof(CPS_Instance*));
		std::copy(allParticles.begin(), allParticles.end(), I);

		struct destroy_on_game_load
		{
			static IC bool predicate(CPS_Instance*const& object)
			{
				return(!object->destroy_on_game_load());
			}
		};

		CPS_Instance** E = std::remove_if(I, I + active_size, &destroy_on_game_load::predicate);

		for (; I != E; ++I)
			DestroyParticleInstance((*I));
	}

	DeleteParticleQueue();

	VERIFY(particlesWaitingToPlay.empty() && particlesToDelete.empty() && (!all_particles || allParticles.empty()));
#endif
}

void CParticleList::DeleteParticle(CPS_Instance *& p_, bool delay_forced)
{
	if (!p_)
		return;

	protectParticleDeletion.Enter();

	if (engineState.test(FRAME_PROCESING) || delay_forced)
	{
		if (!p_->alreadyInDestroyQueue)
		{
			particlesToDelete.push_back(p_);

			p_->alreadyInDestroyQueue = true;
		}
	}
	else
	{
		p_->readyToDestroy = true;

		xr_delete(p_);
	}

	protectParticleDeletion.Leave();

	p_ = nullptr;
}

ENGINE_API void DestroyParticleInstanceInternal(CPS_Instance*& particle, bool delay_forced)
{
	if (particle)
	{
		particle->PreDestroy();
		g_pGameLevel->Particles.DeleteParticle(particle, delay_forced);
	}
}