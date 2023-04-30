#pragma once

class ENGINE_API CPS_Instance;

class ENGINE_API CParticleList
{
private:

public:

	xr_vector<CPS_Instance*>	allParticles;
	xr_vector<CPS_Instance*>	particlesToDelete;
	xr_vector<CPS_Instance*>	particlesWaitingToPlay;

	AccessLock					protectParticleDeletion;

	//////////////////////////////////////////////	
	// static particles
	xr_vector<CPS_Instance*>	staticParticles;

	CParticleList();
	~CParticleList();

	void						Load					();
	void						Unload					();

	void						UpdateParticles			();
	void						ClearParticles			(const bool &all_particles);
	void						DeleteParticle			(CPS_Instance*& p_, bool delay_forced = false);
	void						DeleteParticleQueue		();
};

extern void ENGINE_API DestroyParticleInstanceInternal(CPS_Instance*& particle, bool delay_forced = false);

template <class T>

void DestroyParticleInstance(T*& p, bool delay_forced = false)
{
	if (p)
	{
		CPS_Instance* particle = dynamic_cast<CPS_Instance*>(p);

		if (particle)
		{
			DestroyParticleInstanceInternal(particle, delay_forced);

			p = nullptr;
		}
		else
			R_ASSERT(false);
	}
}
