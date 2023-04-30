#pragma once

#include "inventory_item_object.h"
#include "NightVisionEffector.h"


class CNightVisionActor
	: public CInventoryItemObject
{
public:
	CNightVisionActor(CActor* actor);
	~CNightVisionActor();

	void UpdateCL() override;

	void SwitchNightVision();
	void SwitchNightVision(bool state, bool use_sounds = true);
	bool GetNightVisionStatus();
	CNightVisionEffector* GetNightVision() const;

	bool HasSounds() const;

	CInventoryItem* CurrentNightVisionItem;

	enum EStats
	{
		eNightVisionActive = (1 << 0),
		eAttached = (1 << 1)
	};

protected:
	CActor* m_actor;

	bool m_bNightVisionEnabled;

	CNightVisionEffector* m_night_vision;

private:
	typedef CInventoryItemObject inherited;
};