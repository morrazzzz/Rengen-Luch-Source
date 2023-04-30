#include "stdafx.h"
#include "alife_update_manager.h"
#include "alife_spawn_registry.h"
#include "alife_object_registry.h"

#define ANOMALIES_PER_UPDATE__SERVER 200

void CALifeUpdateManager::UpdateServerAnomalies()
{
	if (!IsAnomalyPoolCreated())
		CreateAnomaliesPool();

	if (anomalyPoolUpdIndex_ > anomaliesOnServer_.size())
		anomalyPoolUpdIndex_ = anomaliesOnServer_.size();

	for (u32 i = anomalyPoolUpdIndex_ - ANOMALIES_PER_UPDATE__SERVER; i < anomalyPoolUpdIndex_; i++) // update only part of anomalies to not overload engine
	{
		CSE_ALifeAnomalousZone* server_anomaly = anomaliesOnServer_[i];

		if (server_anomaly && server_anomaly->anomalyDoesTimerRespawn_)
			server_anomaly->ServerUpdate();
	}

	if (anomalyPoolUpdIndex_ >= anomaliesOnServer_.size())
		anomalyPoolUpdIndex_ = ANOMALIES_PER_UPDATE__SERVER;
	else
		anomalyPoolUpdIndex_ += ANOMALIES_PER_UPDATE__SERVER;
}

void CALifeUpdateManager::DoForceRespawnArts()
{
	if (!IsAnomalyPoolCreated())
		CreateAnomaliesPool();

	for (u32 i = 0; i < anomaliesOnServer_.size(); i++)
	{
		CSE_ALifeAnomalousZone* server_anomaly = anomaliesOnServer_[i];

		if (server_anomaly)
			server_anomaly->RespawnArtefacts(true);
	}

	CountAnomalyArtsOnServer();

	Msg("~ Total Artefacts On Server %u", GetTotalSpawnedAnomArts());
}

void CALifeUpdateManager::CreateAnomaliesPool()
{
	ALife::D_OBJECT_P_MAP::const_iterator	I = objects().objects().begin();
	ALife::D_OBJECT_P_MAP::const_iterator	E = objects().objects().end();

	u32 count = 0;

	for (; I != E; ++I)
	{
		CSE_ALifeAnomalousZone* server_anomaly = smart_cast<CSE_ALifeAnomalousZone *>((*I).second);

		if (server_anomaly)
		{
			anomaliesOnServer_.push_back(server_anomaly);
			count++;
		}
	}

	if (anomaliesOnServer_.size() > 0)
	{
		anomalyPoolCreated_ = true;

		CountAnomalyArtsOnServer();
	}
}

void CALifeUpdateManager::CountAnomalyArtsOnServer()
{
	totalSpawnedAnomalyArts_ = 0;

	for (u32 i = 0; i < anomaliesOnServer_.size(); i++)
	{
		CSE_ALifeAnomalousZone* server_anomaly = anomaliesOnServer_[i];

		if (server_anomaly)
			totalSpawnedAnomalyArts_ += server_anomaly->suroundingArtsID_.size();
	}
}

u32 CALifeUpdateManager::GetTotalSpawnedAnomArts()
{
	if (totalSpawnedAnomalyArts_ <= 0)
		CountAnomalyArtsOnServer();

	return totalSpawnedAnomalyArts_;
}