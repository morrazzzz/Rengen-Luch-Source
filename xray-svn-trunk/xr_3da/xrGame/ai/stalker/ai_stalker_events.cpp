////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_stalker_events.cpp
//	Created 	: 26.02.2003
//  Modified 	: 26.02.2003
//	Author		: Dmitriy Iassenev
//	Description : Events handling for monster "Stalker"
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ai_stalker.h"
#include "../../pda.h"
#include "../../inventory.h"
#include "../../xrmessages.h"
#include "../../shootingobject.h"
#include "../../level.h"
#include "../../ai_monster_space.h"
#include "../../characterphysicssupport.h"

using namespace StalkerSpace;
using namespace MonsterSpace;

#define SILENCE

void CAI_Stalker::OnEvent(NET_Packet& P, u16 type)
{
	inherited::OnEvent(P, type);
	CInventoryOwner::OnEvent(P, type);

	switch (type)
	{
	case GE_TRADE_BUY:
	case GE_OWNERSHIP_TAKE:
	{
		u16			id;
		P.r_u16(id);
		bool duringSpawn = !P.r_eof() && P.r_u8();
		CObject		*O = Level().Objects.net_Find(id);

		R_ASSERT(O);

#ifndef SILENCE
		Msg("Trying to take - %s (%d)", *O->ObjectName(), O->ID());
#endif

		CGameObject	*_O = smart_cast<CGameObject*>(O);
		PIItem item_to_take = smart_cast<PIItem>(O);

		if (inventory().CanTakeItem(item_to_take))
		{
			O->H_SetParent(this);
			inventory().TakeItem(item_to_take, true, false, duringSpawn);

			if (!inventory().ActiveItem() && GetScriptControl() && smart_cast<CShootingObject*>(O))
				CObjectHandler::set_goal(eObjectActionIdle, _O);

			on_after_take(_O);

#ifndef SILENCE
			Msg("TAKE - %s (%d)", *O->ObjectName(), O->ID());
#endif
		}
		else
		{
			NET_Packet P;
			u_EventGen(P, GE_OWNERSHIP_REJECT, ID());
			P.w_u16(u16(O->ID()));
			u_EventSend(P);

#ifndef SILENCE
			Msg("TAKE - can't take! - Dropping for valid server information %s (%d)", *O->ObjectName(), O->ID());
#endif
		}
		break;
	}
	case GE_TRADE_SELL:
	case GE_OWNERSHIP_REJECT:
	{
		u16 id;
		P.r_u16(id);

		CObject* O = Level().Objects.net_Find(id);

		if (!O)
			break;

		bool just_before_destroy = !P.r_eof() && P.r_u8();

		bool dont_create_shell = (type == GE_TRADE_SELL) || just_before_destroy;

		O->SetTmpPreDestroy(just_before_destroy);
		on_ownership_reject(O, dont_create_shell);

		break;
	}
	}
}

void CAI_Stalker::on_ownership_reject(CObject*O, bool just_before_destroy)
{
	m_pPhysics_support->in_UpdateCL();
	IKinematics* const kinematics = smart_cast<IKinematics*>(Visual());
	kinematics->CalculateBones_Invalidate();
	kinematics->CalculateBones(BonesCalcType::need_actual);

	CInventoryItem* const item = smart_cast<CInventoryItem*>(O);
	VERIFY(item);

	if (!inventory().DropItem(item))
	{
		O->H_SetParent(0, just_before_destroy);

		return;
	}

	if (O->getDestroy())
		return;

	//O->H_SetParent(0, just_before_destroy); // moved to drop item
	feel_touch_deny(O, 2000);
}

void CAI_Stalker::generate_take_event(CObject const* const object) const
{
	NET_Packet packet;
	u_EventGen(packet, GE_OWNERSHIP_TAKE, ID());
	packet.w_u16(object->ID());
	u_EventSend(packet);
}

void CAI_Stalker::DropItemSendMessage(CObject *O)
{
	if (!O || !O->H_Parent() || (this != O->H_Parent()))
		return;

#ifndef SILENCE
	Msg("Dropping item!");
#endif
	// We doesn't have similar weapon - pick up it
	NET_Packet P;
	u_EventGen(P, GE_OWNERSHIP_REJECT, ID());
	P.w_u16(u16(O->ID()));
	u_EventSend(P);
}

void CAI_Stalker::UpdateAvailableDialogs(CPhraseDialogManager* partner)
{
	CAI_PhraseDialogManager::UpdateAvailableDialogs(partner);
}

void CAI_Stalker::feel_touch_new(CObject* O)
{
	if (!g_Alive())
		return;

	if ((O->spatial.s_type | STYPE_VISIBLEFORAI) != O->spatial.s_type) return;

	// Now, test for game specific logical objects to minimize traffic
	CInventoryItem* I = smart_cast<CInventoryItem*>	(O);

	if (!wounded() && !critically_wounded() && I && I->useful_for_NPC() && can_take(I))
	{
#ifndef SILENCE
		Msg("Taking item %s (%d)!", *I->ObjectName(), I->ID());
#endif
		generate_take_event(O);
		return;
	}

	VERIFY2(
		std::find(m_ignored_touched_objects.begin(), m_ignored_touched_objects.end(), O) == m_ignored_touched_objects.end(),
		make_string("object %s is already in ignroed touched objects list", O->ObjectName().c_str())
	);
	m_ignored_touched_objects.push_back(O);
}