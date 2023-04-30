#include "stdafx.h"
#include "NightVisionActor.h"
#include "NightVisionDevice.h"
#include "entity.h"
#include "Actor.h"
#include "inventory.h"
#include "CustomOutfit.h"

#define NV_SOUND_SECTION "night_vision_base"


CNightVisionEffector* CreateNVEffector();

CNightVisionActor::CNightVisionActor(CActor* actor)
	: CurrentNightVisionItem(nullptr)
	, m_night_vision(nullptr)
	, m_actor(actor)
	, m_bNightVisionEnabled(true)
{}

CNightVisionActor::~CNightVisionActor()
{
	xr_delete(m_night_vision);
}

void CNightVisionActor::UpdateCL()
{
	if (m_night_vision && m_night_vision->IsActive())
	{
		if (CurrentNightVisionItem)
		{
			if (m_actor && 
				CurrentNightVisionItem != m_actor->inventory().ItemFromSlot(PNV_SLOT) &&
				CurrentNightVisionItem != m_actor->inventory().ItemFromSlot(HELMET_SLOT) &&
				CurrentNightVisionItem != m_actor->inventory().ItemFromSlot(OUTFIT_SLOT))
			{
				CurrentNightVisionItem = nullptr;

				SwitchNightVision(false);
			}
		}
	}
}

void CNightVisionActor::SwitchNightVision()
{
	if (!m_night_vision)
		m_night_vision = CreateNVEffector();

	SwitchNightVision(!m_night_vision->IsActive());
}

void CNightVisionActor::SwitchNightVision(bool state, bool use_sounds)
{
	if (!m_bNightVisionEnabled || !m_actor)
		return;

	if (!m_night_vision)
		m_night_vision = CreateNVEffector();

	bool bIsActiveNow = m_night_vision->IsActive();

	if (state)
	{
		if (!bIsActiveNow)
		{
			CInventoryItem*	pOutfit = m_actor->inventory().ItemFromSlot(OUTFIT_SLOT);
			CNightVisionUsable* outfit_nv = static_cast<COutfitBase*>(pOutfit);

			CInventoryItem* pHelmet = m_actor->inventory().ItemFromSlot(HELMET_SLOT);
			CNightVisionUsable* helmet_nv = static_cast<COutfitBase*>(pHelmet);

			CInventoryItem* pNVDevice = m_actor->inventory().ItemFromSlot(PNV_SLOT);
			CNightVisionUsable* device_nv = static_cast<CNightVisionDevice*>(pNVDevice);

			if (pOutfit && (outfit_nv->NightVisionSectionPPE.size() || outfit_nv->NightVisionSectionShader.size()))
			{
				if (outfit_nv->NightVisionSectionPPE.size())
					m_night_vision->StartPPE(outfit_nv->NightVisionSectionPPE, m_actor, use_sounds);
				else
					m_night_vision->StartShader(outfit_nv->NightVisionSectionShader, m_actor, use_sounds);

				CurrentNightVisionItem = pOutfit;
			}
			else if (pHelmet && (helmet_nv->NightVisionSectionPPE.size() || helmet_nv->NightVisionSectionShader.size()))
			{
				if (helmet_nv->NightVisionSectionPPE.size())
					m_night_vision->StartPPE(helmet_nv->NightVisionSectionPPE, m_actor, use_sounds);
				else
					m_night_vision->StartShader(helmet_nv->NightVisionSectionShader, m_actor, use_sounds);

				CurrentNightVisionItem = pHelmet;
			}
			else if (pNVDevice && (device_nv->NightVisionSectionPPE.size() || device_nv->NightVisionSectionShader.size()))
			{
				if (device_nv->NightVisionSectionPPE.size())
					m_night_vision->StartPPE(device_nv->NightVisionSectionPPE, m_actor, use_sounds);
				else
					m_night_vision->StartShader(device_nv->NightVisionSectionShader, m_actor, use_sounds);

				CurrentNightVisionItem = pNVDevice;
			}
		}
	}
	else
	{
		if (bIsActiveNow)
		{
			m_night_vision->StopPPE(100000.0f, use_sounds);
			m_night_vision->StopShader(use_sounds);

			CurrentNightVisionItem = nullptr;
		}
	}
}

bool CNightVisionActor::GetNightVisionStatus()
{ 
	if (m_night_vision)
		return m_night_vision->IsActive();
	else
		return false;
}

CNightVisionEffector* CNightVisionActor::GetNightVision() const 
{ 
	return m_night_vision; 
}

bool CNightVisionActor::HasSounds() const 
{ 
	return m_night_vision && m_night_vision->HasSounds();
}

CNightVisionEffector* CreateNVEffector()
{
	CNightVisionEffector* nv_eff = xr_new<CNightVisionEffector>(NV_SOUND_SECTION);

	R_ASSERT(nv_eff);

	return nv_eff;
}