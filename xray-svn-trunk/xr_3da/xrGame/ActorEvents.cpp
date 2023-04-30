#include "stdafx.h"
#include "actor.h"
#include "uigamesp.h"
#include "inventory.h"
#include "level.h"
#include "FoodItem.h"
#include "holder_custom.h"
#include "ui/UIInventoryWnd.h"

#ifdef DEBUG
#include "PHDebug.h"
#endif

void CActor::OnEvent(NET_Packet& P, u16 type)
{
	inherited::OnEvent			(P, type);
	CInventoryOwner::OnEvent	(P, type);

	u16 id;
	switch (type)
	{
	case GE_TRADE_BUY:
	case GE_OWNERSHIP_TAKE:
		{
			P.r_u16(id);

			bool duringSpawn = !P.r_eof() && P.r_u8();
			CObject* O = Level().Objects.net_Find(id);

			VERIFY2(O, make_string("GE_OWNERSHIP_TAKE: Object not found. object_id = [%d]", id).c_str());

			if (!O)
			{
				Msg("! GE_OWNERSHIP_TAKE: Object not found. object_id = [%d]", id);
				break;
			}

			CFoodItem* pFood = smart_cast<CFoodItem*>(O);

			if(pFood)
				pFood->SetCurrPlace(eItemPlaceRuck);

			PIItem item_to_take = smart_cast<PIItem>(O);
			
			if (inventory().CanTakeItem(item_to_take))
			{
				O->H_SetParent(smart_cast<CObject*>(this));

				inventory().TakeItem(item_to_take, false, true, duringSpawn);

				CUIGameSP* pGameSP = NULL;

				if(CurrentGameUI())
				{
					pGameSP = smart_cast<CUIGameSP*>(CurrentGameUI());

					if (Level().CurrentViewEntity() == this)
						CurrentGameUI()->ReInitShownUI();
				};
				
				//добавить отсоединенный аддон в инвентарь
				if(pGameSP)
				{
					if(pGameSP->TopInputReceiver() == pGameSP->m_InventoryMenu)
					{
						pGameSP->m_InventoryMenu->AddItemToBag(smart_cast<CInventoryItem*>(O));
					}
				}
			} 
			else 
			{
				NET_Packet P;
				u_EventGen(P,GE_OWNERSHIP_REJECT,ID());
				P.w_u16(u16(O->ID()));
				u_EventSend(P);
			}
		}
		break;
	case GE_TRADE_SELL:
	case GE_OWNERSHIP_REJECT:
		{
			P.r_u16(id);
			CObject* Obj = Level().Objects.net_Find(id);

			VERIFY2(Obj, make_string("GE_OWNERSHIP_REJECT: Object not found, id = %d", id).c_str());
			if (!Obj)
			{
				Msg("! GE_OWNERSHIP_REJECT: Object not found, id = %d", id);
				break;
			}

			VERIFY(Obj->H_Parent());
			if (!Obj->H_Parent())
			{
				Msg("! ERROR: Actor [%d][%s] tries to reject item [%d][%s] that has no parent",
					ID(), Name(), Obj->ID(), Obj->SectionName().c_str());
				break;
			}

			VERIFY2(Obj->H_Parent()->ID() == ID(),
				make_string("actor [%d][%s] tries to drop not own object [%d][%s]",
					ID(), Name(), Obj->ID(), Obj->SectionName().c_str()).c_str());

			if (Obj->H_Parent()->ID() != ID())
			{
				CActor* real_parent = smart_cast<CActor*>(Obj->H_Parent());
				Msg("! ERROR: Actor [%d][%s] tries to drop not own item [%d][%s], his parent is [%d][%s]",
					ID(), Name(), Obj->ID(), Obj->SectionName().c_str(), real_parent->ID(), real_parent->Name());
				break;
			}

			bool just_before_destroy = !P.r_eof() && P.r_u8();
			bool dont_create_shell = (type == GE_TRADE_SELL) || just_before_destroy;
			Obj->SetTmpPreDestroy(just_before_destroy);

			PIItem item_to_drop = smart_cast<PIItem>(Obj);

			if (!Obj->getDestroy() && inventory().DropItem(item_to_drop))
			{
				Obj->H_SetParent(0, just_before_destroy || dont_create_shell);
				Level().m_feel_deny.feel_touch_deny(Obj, 1000);

				Fvector dropPosition;
				if (!P.r_eof())
				{
					P.r_vec3(dropPosition);

					CGameObject* GO = smart_cast<CGameObject*>(Obj);

					GO->MoveTo(dropPosition);
				}
			}

			if (Level().CurrentViewEntity() == this && CurrentGameUI())
				CurrentGameUI()->ReInitShownUI();
		}
		break;
	}
}
