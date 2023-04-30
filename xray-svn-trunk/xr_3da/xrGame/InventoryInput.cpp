#include "stdafx.h"
#include "inventory.h"
#include "actor.h"
#include "xr_level_controller.h"

bool CInventory::InputAction(int cmd, u32 flags)
{
	CActor *pActor = smart_cast<CActor*>(m_pOwner);

	if (pActor)
	{
		switch (cmd)
		{
		case kWPN_FIRE:
		{
			pActor->SetShotRndSeed();
		}break;
		case kWPN_ZOOM:
		{
			pActor->SetZoomRndSeed();
		}break;
		};
	};

	if (m_iActiveSlot < m_slots.size() &&
		m_slots[m_iActiveSlot].m_pIItem &&
		m_slots[m_iActiveSlot].m_pIItem->Action((u16)cmd, flags))
		return true;

	switch (cmd)
	{
	case kWPN_1:
	case kWPN_2:
	case kWPN_3:
	case kWPN_3b:
	case kWPN_4:
	case kWPN_5:
	case kWPN_6:
	{
		if (flags&CMD_START && !m_bHandsOnly)
		{
			auto desiredSlot = GetSlotByKey(cmd);
			if ((int)m_iActiveSlot == desiredSlot && m_slots[m_iActiveSlot].m_pIItem)
			{
				ActivateSlot(NO_ACTIVE_SLOT);
			}
			else
			{
				ActivateSlot(desiredSlot);
			}
		}
	}break;

	case kARTEFACT:
	{
		if (flags&CMD_START)
		{
			if ((int)m_iActiveSlot == ARTEFACT_SLOT &&
				m_slots[m_iActiveSlot].m_pIItem)
			{
				ActivateSlot(NO_ACTIVE_SLOT);

			}
			else {
				ActivateSlot(ARTEFACT_SLOT);

			}
		}
	}break;

	case kDETECTOR:
		if (flags&CMD_START)
		{
			ActivateSlot(DETECTOR_SLOT);
		}

		break;
	}

	return false;
}

TSlotId CInventory::GetSlotByKey(int cmd)
{
	switch (cmd)
	{
	case kWPN_1:		return KNIFE_SLOT;
	case kWPN_2:		return PISTOL_SLOT;
	case kWPN_3:		return RIFLE_SLOT;
	case kWPN_3b:		return RIFLE_2_SLOT;
	case kWPN_4:		return GRENADE_SLOT;
	case kWPN_5:		return APPARATUS_SLOT;
	case kWPN_6:		return BOLT_SLOT;
	case kARTEFACT:		return ARTEFACT_SLOT;
	case kDETECTOR:		return DETECTOR_SLOT;
	default:			return NO_ACTIVE_SLOT;
	}
}