
//	Module 		: ai_trader.cpp
//	Created 	: 13.05.2002
//	Author		: Jim
//	Description : AI Behaviour for monster "Trader"

#include "pch_script.h"
#include "ai_trader.h"
#include "../../trade.h"
#include "../../script_entity_action.h"
#include "../../script_game_object.h"
#include "../../inventory.h"
#include "../../xrserver_objects_alife_monsters.h"
#include "../../artifact.h"
#include "../../xrserver.h"
#include "../../relation_registry.h"
#include "../../object_broker.h"
#include "../../sound_player.h"
#include "../../level.h"
#include "../../script_callback_ex.h"
#include "../../game_object_space.h"
#include "../../clsid_game.h"
#include "trader_animation.h"

CAI_Trader::CAI_Trader()
{
	AnimMan	= xr_new <CTraderAnimation>(this);
} 

CAI_Trader::~CAI_Trader()
{
	xr_delete(m_sound_player);
	xr_delete(AnimMan);
}

void CAI_Trader::LoadCfg(LPCSTR section)
{
	inherited::LoadCfg(section);

	SetfHealth(pSettings->r_float(section, "Health"));

	float max_weight = pSettings->r_float(section, "max_item_mass");

	inventory().SetMaxWeight(max_weight * 1000);
	inventory().CalcTotalWeight();
}

void CAI_Trader::reinit()
{
	CScriptEntity::reinit();
	CEntityAlive::reinit();
	CInventoryOwner::reinit();
	sound().reinit();
	animation().reinit();

	m_busy_now = false;
}

void CAI_Trader::reload(LPCSTR section)
{
	CEntityAlive::reload(section);
	CInventoryOwner::reload(section);

	sound().reload(section);
}


bool CAI_Trader::bfAssignSound(CScriptEntityAction* tpEntityAction)
{
	if (!CScriptEntity::bfAssignSound(tpEntityAction))
	{
		return(false);
	}

	return(true);
}


// Look At Actor
void CAI_Trader::BoneCallback(CBoneInstance *B)
{
	CAI_Trader*	this_class = static_cast<CAI_Trader*>(B->callback_param());

	this_class->LookAtActor(B);
	R_ASSERT2( _valid( B->mTransform ), "CAI_Trader::BoneCallback" );
}

void CAI_Trader::LookAtActor(CBoneInstance *B)
{
	Fvector dir;
	dir.sub(Level().CurrentEntity()->Position(), Position());

	float yaw, pitch;
	dir.getHP(yaw, pitch);

	float h, p, b;
	XFORM().getHPB(h, p, b);
	float cur_yaw = h;
	float dy = _abs(angle_normalize_signed(yaw - cur_yaw));

	if (angle_normalize_signed(yaw - cur_yaw) > 0) dy *= -1.f;

	Fmatrix M;
	M.setHPB(0.f, -dy, 0.f);
	B->mTransform.mulB_43(M);
}

//////////////////////////////////////////////////////////////////////////

BOOL CAI_Trader::SpawnAndImportSOData(CSE_Abstract* data_containing_so)
{
	CSE_Abstract* e = (CSE_Abstract*)(data_containing_so);
	CSE_ALifeTrader* l_tpTrader = smart_cast<CSE_ALifeTrader*>(e);

	R_ASSERT(l_tpTrader);

	//проспавнить PDA у InventoryOwner
	if (!CInventoryOwner::SpawnAndImportSOData(data_containing_so))
		return(FALSE);

	if (!inherited::SpawnAndImportSOData(data_containing_so) || !CScriptEntity::SpawnAndImportSOData(data_containing_so))
		return(FALSE);

	setVisible(TRUE);
	setEnabled(TRUE);

	set_money(l_tpTrader->m_dwMoney, false);

	// Установка callback на кости
	CBoneInstance* bone_head = &smart_cast<IKinematics*>(Visual())->LL_GetBoneInstance(smart_cast<IKinematics*>(Visual())->LL_BoneID("bip01_head"));
	bone_head->set_callback(bctCustom, BoneCallback, this);

	shedule.t_min = 100;
	shedule.t_max = 2500; // This equaltiy is broken by Dima :-( // 30 * NET_Latency / 4;

	return(TRUE);
}

void CAI_Trader::ExportDataToServer(NET_Packet& P)
{
}

void CAI_Trader::OnEvent(NET_Packet& P, u16 type)
{
	inherited::OnEvent(P, type);
	CInventoryOwner::OnEvent(P, type);

	u16 id;
	CObject* Obj;

	switch (type)
	{
	case GE_TRADE_BUY:
	case GE_OWNERSHIP_TAKE:
	{
		P.r_u16(id);

		bool duringSpawn = !P.r_eof() && P.r_u8();

		Obj = Level().Objects.net_Find(id);

		PIItem item_to_take = smart_cast<PIItem>(Obj);

		if (inventory().CanTakeItem(item_to_take))
		{
			Obj->H_SetParent(this);
			inventory().TakeItem(item_to_take, false, false, duringSpawn);
		}
		else
		{
			NET_Packet				P;
			u_EventGen(P, GE_OWNERSHIP_REJECT, ID());
			P.w_u16(u16(Obj->ID()));
			u_EventSend(P);
		}
	}break;
	case GE_TRADE_SELL:
	case GE_OWNERSHIP_REJECT:
	{
		P.r_u16(id);

		Obj = Level().Objects.net_Find(id);

		bool just_before_destroy = !P.r_eof() && P.r_u8();

		Obj->SetTmpPreDestroy(just_before_destroy);

		PIItem item_to_drop = smart_cast<PIItem>(Obj);

		if (inventory().DropItem(item_to_drop))
			Obj->H_SetParent(0, just_before_destroy);
	}break;
	}
}

void CAI_Trader::feel_touch_new(CObject* O)
{
	if (!g_Alive())
		return;

	// Now, test for game specific logical objects to minimize traffic
	CInventoryItem* I = smart_cast<CInventoryItem*>	(O);

	if (I && I->useful_for_NPC())
	{
		Msg("Taking item %s!", *I->object().ObjectName());

		NET_Packet P;

		u_EventGen(P, GE_OWNERSHIP_TAKE, ID());
		P.w_u16(u16(I->object().ID()));
		u_EventSend(P);
	}
}

void CAI_Trader::DropItemSendMessage(CObject *O)
{
	if (!O || !O->H_Parent() || (this != O->H_Parent()))
		return;

	Msg("Dropping item!");
	// We doesn't have similar weapon - pick up it

	NET_Packet				P;
	u_EventGen(P, GE_OWNERSHIP_REJECT, ID());
	P.w_u16(u16(O->ID()));
	u_EventSend(P);
}

void CAI_Trader::ScheduledUpdate(u32 dt)
{
#ifdef MEASURE_UPDATES
	CTimer measure_sc_update; measure_sc_update.Start();
#endif


	inherited::ScheduledUpdate(dt);

	UpdateInventoryOwner(dt);

	if (GetScriptControl())
		ProcessScripts();
	else
		Think();


#ifdef MEASURE_UPDATES
	Device.Statistic->scheduler_AITrader_ += measure_sc_update.GetElapsed_sec();
#endif
}

void CAI_Trader::g_WeaponBones(int &L, int &R1, int &R2)
{
	IKinematics *V = smart_cast<IKinematics*>(Visual());

	R1 = V->LL_BoneID("bip01_r_hand");
	R2 = V->LL_BoneID("bip01_r_finger2");
	L = V->LL_BoneID("bip01_l_finger1");
}

void CAI_Trader::g_fireParams(const CHudItem* pHudItem, Fvector& P, Fvector& D)
{
	VERIFY(inventory().ActiveItem());

	if (g_Alive() && inventory().ActiveItem())
	{
		Center(P);
		D.setHP(0, 0);
		D.normalize_safe();
	}
}

void CAI_Trader::Think()
{
}

void CAI_Trader::Die(CObject* who)
{
	inherited::Die(who);
}

void CAI_Trader::DestroyClientObj()
{
	inherited::DestroyClientObj();
	CScriptEntity::DestroyClientObj();
}

void CAI_Trader::UpdateCL()
{
#ifdef MEASURE_UPDATES
	CTimer measure_updatecl; measure_updatecl.Start();
#endif


	inherited::UpdateCL();

	sound().update(TimeDelta());

	if (!GetScriptControl() && !bfScriptAnimation())
		animation().update_frame();


#ifdef MEASURE_UPDATES
	Device.Statistic->updateCL_AITrader_ += measure_updatecl.GetElapsed_sec();
#endif
}

BOOL CAI_Trader::UsedAI_Locations()
{
	return(TRUE);
}

void CAI_Trader::OnStartTrade()
{
	m_busy_now = true;
	callback(GameObject::eTradeStart)();
}

void CAI_Trader::OnStopTrade()
{
	m_busy_now = false;
	callback(GameObject::eTradeStop)();
}

bool CAI_Trader::can_attach(const CInventoryItem *inventory_item) const
{
	return(false);
}

bool CAI_Trader::use_bolts() const
{
	return(false);
}

void CAI_Trader::spawn_supplies()
{
	inherited::spawn_supplies();
	CInventoryOwner::spawn_supplies();
}

void CAI_Trader::save(NET_Packet &output_packet)
{
	inherited::save(output_packet);
	CInventoryOwner::save(output_packet);
}

void CAI_Trader::load(IReader &input_packet)
{
	inherited::load(input_packet);
	CInventoryOwner::load(input_packet);
}

//проверяет список артефактов в заказах
u32 CAI_Trader::ArtefactPrice(CArtefact* pArtefact)
{
	return pArtefact->Cost();
}

//продажа артефакта, с последуещим изменением списка заказов (true - если артефакт был в списке)
bool CAI_Trader::BuyArtefact(CArtefact* pArtefact)
{
	VERIFY(pArtefact);

	return false;
}

ALife::ERelationType  CAI_Trader::tfGetRelationType(const CEntityAlive *tpEntityAlive) const
{
	const CInventoryOwner* pOtherIO = smart_cast<const CInventoryOwner*>(tpEntityAlive);

	ALife::ERelationType relation = ALife::eRelationTypeDummy;

	if (pOtherIO && !(const_cast<CEntityAlive *>(tpEntityAlive)->cast_base_monster()))
		relation = RELATION_REGISTRY().GetRelationType(static_cast<const CInventoryOwner*>(this), pOtherIO);

	if (ALife::eRelationTypeDummy != relation)
		return relation;
	else
		return inherited::tfGetRelationType(tpEntityAlive);
}

DLL_Pure* CAI_Trader::_construct()
{
	m_sound_player = xr_new <CSoundPlayer>(this);

	CEntityAlive::_construct();
	CInventoryOwner::_construct();
	CScriptEntity::_construct();

	return(this);
}

bool CAI_Trader::AllowItemToTrade(CInventoryItem const * item, EItemPlace place) const
{
	if (!g_Alive())
		return(true);

	if (item->object().CLS_ID == CLSID_DEVICE_PDA)
		return(false);

	return(CInventoryOwner::AllowItemToTrade(item, place));
}

void CAI_Trader::dialog_sound_start(LPCSTR phrase)
{
	animation().external_sound_start(phrase);
}

void CAI_Trader::dialog_sound_stop()
{
	animation().external_sound_stop();
}
