////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_anomalous_zone.cpp
//	Created 	: 27.10.2005
//  Modified 	: 10.05.2017 tatarinrafa
//	Author		: Dmitriy Iassenev
//	Description : ALife anomalous zone class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "ai_space.h"
#include "alife_simulator.h"
#include "alife_spawn_registry.h"
#include "alife_graph_registry.h"
#include "GameConstants.h"

u32 newgameartscount = 0;

CSE_ALifeItemWeapon	*CSE_ALifeAnomalousZone::tpfGetBestWeapon(ALife::EHitType &tHitType, float &fHitPower)
{
	m_tpCurrentBestWeapon		= 0;
	m_tTimeID					= GetGameTime();
	fHitPower					= m_maxPower;
	tHitType					= m_tHitType;
	return						(m_tpCurrentBestWeapon);
}

ALife::EMeetActionType CSE_ALifeAnomalousZone::tfGetActionType(CSE_ALifeSchedulable *tpALifeSchedulable, int iGroupIndex, bool bMutualDetection)
{
	return						(ALife::eMeetActionTypeAttack);
}

bool CSE_ALifeAnomalousZone::bfActive()
{
	return						(fis_zero(m_maxPower,EPS_L) || !interactive());
}

CSE_ALifeDynamicObject *CSE_ALifeAnomalousZone::tpfGetBestDetector()
{
	VERIFY2						(false,"This function shouldn't be called");
	NODEFAULT;
#ifdef DEBUG
	return						(0);
#endif
}

void CSE_ALifeAnomalousZone::spawn_artefacts(bool respawn)
{
	if (suroundingArtsID_.size() > maxSurroundingArtefacts_)
		return;

	if (Alife()->GetTotalSpawnedAnomArts() > GameConstants::GetMaxAnomalySpawnedArtefacts())
		return;

	float					m_min_start_power	= pSettings->r_float(name(),"min_start_power");
	float					m_max_start_power	= pSettings->r_float(name(),"max_start_power");

	//---- ����
	float m_fArtefactSpawnProbability = 0.f;
	u8 m_iArtSpawnCicles = 0;
	LPCSTR l_caParameters = nullptr;
	BOOL separate_resp_config = !!READ_IF_EXISTS(pSettings, r_bool, name(), "separate_resp_config", FALSE);

	if (respawn && separate_resp_config)
	{
		m_fArtefactSpawnProbability		= READ_IF_EXISTS(pSettings, r_float, name(), "respawn_art_prob", 0.0f);
		m_iArtSpawnCicles				= READ_IF_EXISTS(pSettings, r_u8, name(), "respawn_art_spawn_cicles", 2);

		l_caParameters					= READ_IF_EXISTS(pSettings, r_string, name(), "respawn_artefacts", "");
	}
	else
	{
		BOOL m_bSpawnArtefact			= !!READ_IF_EXISTS(pSettings, r_bool, name(), "art_onstart_spawn", FALSE);

		if (!m_bSpawnArtefact)
			return;

		m_fArtefactSpawnProbability		= READ_IF_EXISTS(pSettings, r_float, name(), "art_onstart_spawn_prob", 0.0f);
		m_iArtSpawnCicles				= READ_IF_EXISTS(pSettings, r_u8, name(), "art_onstart_spawn_cicles", 2);

		l_caParameters					= READ_IF_EXISTS(pSettings, r_string, name(), "artefacts", "");
	}


	//---- ���� ������ ����������
	struct ARTEFACT_SPAWN
	{
		shared_str	section;
		float		probability;
	};

	DEFINE_VECTOR(ARTEFACT_SPAWN, ARTEFACT_SPAWN_VECTOR, ARTEFACT_SPAWN_IT);
	ARTEFACT_SPAWN_VECTOR	m_ArtefactSpawn;

	u16 m_wItemCount = (u16)_GetItemCount(l_caParameters);

	R_ASSERT2(!(m_wItemCount & 1), "Invalid number of parameters in string 'artefacts' in the 'system.ltx'!");

	m_wItemCount >>= 1;

	m_ArtefactSpawn.clear();
	string512 l_caBuffer;
	float total_probability = 0.f;
	m_ArtefactSpawn.resize(m_wItemCount);

	for (u16 i = 0; i<m_wItemCount; ++i)
	{
		ARTEFACT_SPAWN& artefact_spawn = m_ArtefactSpawn[i];
		artefact_spawn.section = _GetItem(l_caParameters, i << 1, l_caBuffer);
		artefact_spawn.probability = (float)atof(_GetItem(l_caParameters, (i << 1) | 1, l_caBuffer));
		total_probability += artefact_spawn.probability;
	}

	if (total_probability == 0.f)
		total_probability = 1.0;

	R_ASSERT3(!fis_zero(total_probability), "The probability of artefact spawn is zero!", name());

	for (u16 i = 0; i<m_ArtefactSpawn.size(); ++i)	//��������������� �����������
	{
		m_ArtefactSpawn[i].probability = m_ArtefactSpawn[i].probability / total_probability;
	}

	if (m_min_start_power == m_max_start_power)
		m_maxPower = m_min_start_power;
	else
		m_maxPower = randF(m_min_start_power, m_max_start_power);

	if (m_ArtefactSpawn.empty())
		return;

	//---- ��������� �����
	for (int i = 0; i < m_iArtSpawnCicles; ++i) //Lets add an oportunity of spawning several arts
	{	
		//---- ����� ���� �� ������ ���������� (��������� �������� ������������� ������������)
		if (::Random.randF(0.f, 1.f) < m_fArtefactSpawnProbability)
		{
			float rnd = ::Random.randF(.0f, 1.f - EPS_L);
			float prob_threshold = 0.f;

			std::size_t i = 0;
			for (; i<m_ArtefactSpawn.size(); i++)
			{
				prob_threshold += m_ArtefactSpawn[i].probability;
				if (rnd<prob_threshold) break;
			}

			R_ASSERT(i<m_ArtefactSpawn.size());

			newgameartscount += 1;

			Fvector pos						= position();
			CSE_Abstract* l_tpSE_Abstract	= alife().spawn_item(*m_ArtefactSpawn[i].section, pos, m_tNodeID, m_tGraphID, 0xffff);

			//---- Alife shit
			R_ASSERT3(l_tpSE_Abstract, "Can't spawn artefact ", m_ArtefactSpawn[i].section.c_str());

			CSE_ALifeDynamicObject*	oALifeDynamicObject = smart_cast<CSE_ALifeDynamicObject*>(l_tpSE_Abstract);

			R_ASSERT3(oALifeDynamicObject, "Non-ALife artefact spawned. Check class inherancy", m_ArtefactSpawn[i].section.c_str());

			oALifeDynamicObject->m_tSpawnID			= m_tSpawnID;
			oALifeDynamicObject->m_bALifeControl	= true;
			ai().alife().spawns().assign_artefact_position(this, oALifeDynamicObject);

			Fvector	t		= oALifeDynamicObject->o_Position;
			u32	p			= oALifeDynamicObject->m_tNodeID;
			float q			= oALifeDynamicObject->m_fDistance;

			alife().graph().change(oALifeDynamicObject, m_tGraphID, oALifeDynamicObject->m_tGraphID);

			oALifeDynamicObject->o_Position			= t;
			oALifeDynamicObject->m_tNodeID			= p;
			oALifeDynamicObject->m_fDistance		= q;

			CSE_ALifeItemArtefact* server_art = smart_cast<CSE_ALifeItemArtefact*>(l_tpSE_Abstract);

			if (server_art)
			{
				server_art->serverOwningAnomalyID_ = ID;
				suroundingArtsID_.push_back(server_art->ID);

				Alife()->totalSpawnedAnomalyArts_++;

#ifdef DEBUG
				Msg("^Spawned ART ID %u, Anomaly ID %u, Artefact spawning: %s -> %s, Arts owned by anomalies in game %u", server_art->ID, ID, name(), m_ArtefactSpawn[i].section.c_str(), Alife()->totalSpawnedAnomalyArts_);
#endif
			}
		}
	}
}

void CSE_ALifeAnomalousZone::on_spawn()
{
	inherited::on_spawn();
	spawn_artefacts(false);
}

void CSE_ALifeAnomalousZone::RespawnArtefacts(bool no_time_check)
{
	if (anomalyHasRespawner_)
	{
		u32 game_hours = GetGameTimeInHours();

		if (no_time_check)
		{
			spawn_artefacts(true);

			nextRespawnTime_ = game_hours + artRespawnTime_;
		}
		else
		{
			if (nextRespawnTime_ == 0)
			{
				nextRespawnTime_ = game_hours;

				return;
			}

			if (game_hours > nextRespawnTime_)
			{
				spawn_artefacts(true);

				nextRespawnTime_ = game_hours + artRespawnTime_;
			}
		}
	}
}

void CSE_ALifeAnomalousZone::ServerUpdate()
{
	RespawnArtefacts();
}

bool CSE_ALifeAnomalousZone::keep_saved_data_anyway() const
{
	return (true);
}
