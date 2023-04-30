#include "pch_script.h"
#include "InventoryOwner.h"
#include "entity_alive.h"
#include "pda.h"
#include "actor.h"
#include "actorcondition.h"
#include "trade.h"
#include "inventory.h"
#include "xrserver_objects_alife_items.h"
#include "character_info.h"
#include "script_game_object.h"
#include "AI_PhraseDialogManager.h"
#include "level.h"
#include "xrserver.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "alife_registry_wrappers.h"
#include "ai_object_location.h"
#include "script_callback_ex.h"
#include "game_object_space.h"
#include "AI/Monsters/BaseMonster/base_monster.h"
#include "trade_parameters.h"
#include "purchase_list.h"
#include "clsid_game.h"
#include "artifact.h"

#include "alife_object_registry.h"

#include "CustomOutfit.h"

CInventoryOwner::CInventoryOwner			()
{
	m_pTrade					= NULL;
	m_trade_parameters			= 0;

	m_inventory					= xr_new <CInventory>();
	m_pCharacterInfo			= xr_new <CCharacterInfo>();
	
	EnableTalk();
	EnableTrade();
	
	bDisableBreakDialog			= false;
	m_known_info_registry		= xr_new <CInfoPortionWrapper>();
	m_tmp_active_slot_num		= NO_ACTIVE_SLOT;
	m_need_osoznanie_mode		= FALSE;

	special_map_spot			= nullptr;
	noInvWnd_					= false;
	m_deadbody_can_take				= true;
	m_deadbody_closed				= false;
	m_play_show_hide_reload_sounds	= true;

	m_bAllowTalk				= true;
	m_bAllowTrade				= true;
	m_bAllowUpgrade				= true;
}

DLL_Pure *CInventoryOwner::_construct		()
{
	m_trade_parameters			= 0;
	m_purchase_list				= 0;

	return						(smart_cast<DLL_Pure*>(this));
}

CInventoryOwner::~CInventoryOwner			() 
{
	xr_delete					(m_inventory);
	xr_delete					(m_pTrade);
	xr_delete					(m_pCharacterInfo);
	xr_delete					(m_known_info_registry);
	xr_delete					(m_trade_parameters);
	xr_delete					(m_purchase_list);
}

void CInventoryOwner::LoadCfg					(LPCSTR section)
{
	inventory().LoadCfg(section);

	if(pSettings->line_exist(section, "need_osoznanie_mode"))
	{
		m_need_osoznanie_mode=pSettings->r_bool(section,"need_osoznanie_mode");
	}
	else
	{
		m_need_osoznanie_mode=FALSE;
	}


	m_bDrawSecWpn				= !!READ_IF_EXISTS(pSettings, r_bool, section, "draw_secondary_weapon", TRUE);
	noInvWnd_					= !!READ_IF_EXISTS(pSettings, r_bool, section, "no_inv_window", FALSE);
}

void CInventoryOwner::reload				(LPCSTR section)
{
	inventory().Clear			();
	inventory().m_pOwner		= this;

	inventory().SetSlotsUseful (true);

	m_money						= 0;
	m_bTrading					= false;
	m_bTalking					= false;
	m_pTalkPartner				= NULL;

	CAttachmentOwner::reload	(section);
}

void CInventoryOwner::reinit				()
{
	CAttachmentOwner::reinit	();
	m_item_to_spawn				= shared_str();
	m_ammo_in_box_to_spawn		= 0;
}

#include "map_manager.h"

//call this after CGameObject::SpawnAndImportSOData
BOOL CInventoryOwner::SpawnAndImportSOData(CSE_Abstract* data_containing_so)
{
	if (!m_pTrade)
		m_pTrade = xr_new <CTrade>(this);

	if (m_trade_parameters)
		xr_delete(m_trade_parameters);

	m_trade_parameters = xr_new <CTradeParameters>(trade_section());

	//получить указатель на объект, InventoryOwner
	//m_inventory->setSlotsBlocked(false);
	CGameObject			*pThis = smart_cast<CGameObject*>(this);
	if (!pThis) return FALSE;
	CSE_Abstract* E = (CSE_Abstract*)(data_containing_so);

	CSE_ALifeTraderAbstract* pTrader = NULL;
	if (E) pTrader = smart_cast<CSE_ALifeTraderAbstract*>(E);
	if (!pTrader) return FALSE;

	R_ASSERT(pTrader->character_profile().size());

	//синхронизируем параметры персонажа с серверным объектом
	CharacterInfo().InitCInfo(pTrader);

	isTrader_ = SpecificCharacter().IsTrader();

	auto customMonster = smart_cast<CCustomMonster*>(this);
	if (customMonster)
	{
		customMonster->invulnerable(SpecificCharacter().Invulnerable());
	}

	//-------------------------------------
	m_known_info_registry->registry().init(E->ID);
	//-------------------------------------

	m_deadbody_can_take = pTrader->m_deadbody_can_take;
	m_deadbody_closed = pTrader->m_deadbody_closed;

	CAI_PhraseDialogManager* dialog_manager = smart_cast<CAI_PhraseDialogManager*>(this);
	if (dialog_manager && !dialog_manager->GetStartDialog().size())
	{
		dialog_manager->SetStartDialog(CharacterInfo().StartDialog());
		dialog_manager->SetDefaultStartDialog(CharacterInfo().StartDialog());
	}

	m_game_name = pTrader->m_character_name;

	//Special Map Spot assigned in character desc
	LPCSTR str = SpecificCharacter().MapSpotString();
	if (xr_strcmp(str, "null"))
	{
		special_map_spot = Level().MapManager().AddMapLocation(str, E->ID);
	}

	return TRUE;
}

void CInventoryOwner::DestroyClientObj()
{
	CAttachmentOwner::DestroyClientObj();
	
	inventory().Clear();
	inventory().SetActiveSlot(NO_ACTIVE_SLOT);
}


void	CInventoryOwner::save	(NET_Packet &output_packet)
{
	u32 active_slot = inventory().GetActiveSlot();
	if(active_slot == NO_ACTIVE_SLOT /*|| active_slot == BOLT_SLOT*/)
		output_packet.w_u8((u8)(-1));
	else
		output_packet.w_u8((u8)active_slot);

	CharacterInfo().save(output_packet);
	save_data	(m_game_name, output_packet);
	save_data	(m_money,	output_packet);
}
void	CInventoryOwner::load	(IReader &input_packet)
{
	u8 active_slot = input_packet.r_u8();
	if(active_slot == u8(-1))
		inventory().SetActiveSlot(NO_ACTIVE_SLOT);
	else
		inventory().ActivateSlot(active_slot);

	m_tmp_active_slot_num		 = active_slot;

	CharacterInfo().load(input_packet);
	load_data		(m_game_name, input_packet);
	load_data		(m_money,	input_packet);
}


void CInventoryOwner::UpdateInventoryOwner(u32 deltaT)
{
	inventory().Update();

	if (m_pTrade)
	{
		m_pTrade->UpdateTrade();
	}
	if (IsTrading())
	{
		//если мы умерли, то нет "trade"
		if (!is_alive())
		{
			StopTrading();
		}
	}

	if (IsTalking())
	{
		//если наш собеседник перестал говорить с нами,
		//то и нам нечего ждать.
		if (!m_pTalkPartner->IsTalking())
		{
			StopTalk();
		}

		//если мы умерли, то тоже не говорить
		if (!is_alive())
		{
			StopTalk();
		}
	}
}


//достать PDA из специального слота инвентаря
CPda* CInventoryOwner::GetPDA() const
{
	return (CPda*)(m_inventory->ItemFromSlot(PDA_SLOT));
}

CTrade* CInventoryOwner::GetTrade() 
{
	R_ASSERT2(m_pTrade, "trade for object does not init yet");
	return m_pTrade;
}


//состояние диалога

//нам предлагают поговорить,
//проверяем наше отношение 
//и если не враг начинаем разговор
bool CInventoryOwner::OfferTalk(CInventoryOwner* talk_partner)
{
	if(!IsTalkEnabled()) return false;

	//проверить отношение к собеседнику
	CEntityAlive* pOurEntityAlive = smart_cast<CEntityAlive*>(this);
	R_ASSERT(pOurEntityAlive);

	CEntityAlive* pPartnerEntityAlive = smart_cast<CEntityAlive*>(talk_partner);
	R_ASSERT(pPartnerEntityAlive);
	
//	ALife::ERelationType relation = RELATION_REGISTRY().GetRelationType(this, talk_partner);
//	if(relation == ALife::eRelationTypeEnemy) return false;

	if(!is_alive() || !pPartnerEntityAlive->g_Alive()) return false;

	StartTalk(talk_partner);

	return true;
}


void CInventoryOwner::StartTalk(CInventoryOwner* talk_partner, bool start_trade)
{
	m_bTalking = true;
	m_pTalkPartner = talk_partner;

	//тут же включаем торговлю
	if(start_trade)
		GetTrade()->StartTradeEx(talk_partner);
}

#include "UIGameSP.h"
#include "HUDmanager.h"
#include "ui\UITalkWnd.h"

void CInventoryOwner::StopTalk()
{
	GetTrade()->StopTrade	();
	m_pTalkPartner = NULL;
	m_bTalking = false;

	CUIGameSP* ui_sp = smart_cast<CUIGameSP*>(CurrentGameUI());
	if (ui_sp && ui_sp->TalkMenu->IsShown())
		ui_sp->TalkMenu->Stop();
}

bool CInventoryOwner::IsTalking()
{
	return m_bTalking;
}

void CInventoryOwner::StartTrading()
{
	m_bTrading = true;
}

void CInventoryOwner::StopTrading()
{
	m_bTrading = false;

	CUIGameSP* ui_sp = smart_cast<CUIGameSP*>(CurrentGameUI());
	if (ui_sp)
	{
		ui_sp->HideActorMenu();
	}
}

bool CInventoryOwner::IsTrading()
{
	return m_bTrading;
}

void CInventoryOwner::renderable_Render(IRenderBuffer& render_buffer)
{
	auto activeItem = inventory().ActiveItem();

	if (activeItem)
		activeItem->renderable_Render(render_buffer);

	if (m_bDrawSecWpn)
	{
		auto secWeapon = inventory().ItemFromSlot(RIFLE_SLOT);

		if (secWeapon && secWeapon != activeItem)
			secWeapon->renderable_Render(render_buffer);

		secWeapon = inventory().ItemFromSlot(RIFLE_2_SLOT);
		if (secWeapon && secWeapon != activeItem)
			secWeapon->renderable_Render(render_buffer);
	}

	CAttachmentOwner::renderable_Render(render_buffer);
}

void CInventoryOwner::OnItemTake			(CInventoryItem *inventory_item, bool duringSpawn)
{
	CGameObject	*object = smart_cast<CGameObject*>(this);
	VERIFY		(object);
	object->callback(GameObject::eOnItemTake)(inventory_item->object().lua_game_object(), duringSpawn);

	attach		(inventory_item);

	if(m_tmp_active_slot_num!=NO_ACTIVE_SLOT && inventory_item->BaseSlot()==m_tmp_active_slot_num)
	{
		inventory().ActivateSlot(m_tmp_active_slot_num);
		m_tmp_active_slot_num	= NO_ACTIVE_SLOT;
	}
}

//возвращает текуший разброс стрельбы с учетом движения (в радианах)
float CInventoryOwner::GetWeaponAccuracy	() const
{
	return 0.f;
}

float CInventoryOwner::GetOutfitWeightBonus		() const
{
	const CCustomOutfit* outfit	= GetOutfit();
	if (outfit)
		return outfit->m_additional_weight2;

	return 0.f;
}

//максимальный переносимы вес
float  CInventoryOwner::MaxCarryWeight () const
{
	return inventory().GetMaxWeight() + GetAdditionalWeight();
}

float CInventoryOwner::GetAdditionalWeight () const
{
	float ret = GetOutfitWeightBonus();

	//tatarinrafa added additional_inventory_weight to artefacts
	for (u32 it = 0; it < inventory().artefactBelt_.size(); ++it)
	{
		CArtefact*	artefact = smart_cast<CArtefact*>(inventory().artefactBelt_[it]);
		if (artefact)
		{
			ret += artefact->m_additional_weight;
		}
	}

	return ret;
}

void CInventoryOwner::spawn_supplies		()
{
	CGameObject								*game_object = smart_cast<CGameObject*>(this);
	VERIFY									(game_object);
	if (smart_cast<CBaseMonster*>(this))	return;


	if (use_bolts())
		Level().spawn_item					("bolt",game_object->Position(),game_object->ai_location().level_vertex_id(),game_object->ID());

	if (!ai().get_alife()) {
		CSE_Abstract						*abstract = Level().spawn_item("device_pda",game_object->Position(),game_object->ai_location().level_vertex_id(),game_object->ID(),true);
		CSE_ALifeItemPDA					*pda = smart_cast<CSE_ALifeItemPDA*>(abstract);
		R_ASSERT							(pda);
		pda->m_original_owner				= (u16)game_object->ID();
		NET_Packet							P;
		abstract->Spawn_Write				(P,TRUE);
		Level().Send						(P,net_flags(TRUE));
		F_entity_Destroy					(abstract);
	}
}

//игровое имя 
LPCSTR	CInventoryOwner::Name () const
{
//	return CharacterInfo().Name();
	return m_game_name.c_str();
}

LPCSTR	CInventoryOwner::IconName () const
{
	return CharacterInfo().IconName().c_str();
}

void CInventoryOwner::NewPdaContact		(CInventoryOwner* pInvOwner)
{
}

void CInventoryOwner::LostPdaContact	(CInventoryOwner* pInvOwner)
{
}

//////////////////////////////////////////////////////////////////////////
//для работы с relation system
u16 CInventoryOwner::object_id	()  const
{
	return smart_cast<const CGameObject*>(this)->ID();
}


//////////////////////////////////////////////////////////////////////////
//установка группировки на клиентском и серверном объкте

void CInventoryOwner::SetCommunity	(CHARACTER_COMMUNITY_INDEX new_community)
{
	CEntityAlive* EA					= smart_cast<CEntityAlive*>(this); VERIFY(EA);

	CSE_Abstract* e_entity				= ai().alife().objects().object(EA->ID(), false);
	if(!e_entity) return;

	CharacterInfo().SetCommunity( new_community );
	if( EA->g_Alive() )
	{
		EA->ChangeTeam(CharacterInfo().Community().team(), EA->g_Squad(), EA->g_Group());
	}

	CSE_ALifeTraderAbstract* trader		= smart_cast<CSE_ALifeTraderAbstract*>(e_entity);
	if(!trader) return;

	trader->m_community_index  = new_community;
}

#include "ai/stalker/ai_stalker.h"

void CInventoryOwner::SetIORank			(CHARACTER_RANK_VALUE rank)
{
	CEntityAlive* EA					= smart_cast<CEntityAlive*>(this); VERIFY(EA);
	CSE_Abstract* e_entity				= ai().alife().objects().object(EA->ID(), false);

	if(!e_entity)
		return;

	CSE_ALifeTraderAbstract* trader		= smart_cast<CSE_ALifeTraderAbstract*>(e_entity);

	if(!trader)
		return;

	CharacterInfo().SetCharacterRank(rank);
	trader->SetRankServer(rank);

	CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(this);

	if (stalker)
		stalker->RankChangeCallBack();
}

void CInventoryOwner::ChangeIORank		(CHARACTER_RANK_VALUE delta)
{
	SetIORank(GetIORank() + delta);
}

void CInventoryOwner::SetReputation		(CHARACTER_REPUTATION_VALUE reputation)
{
	CEntityAlive* EA					= smart_cast<CEntityAlive*>(this); VERIFY(EA);
	CSE_Abstract* e_entity				= ai().alife().objects().object(EA->ID(), false);
	if(!e_entity) return;

	CSE_ALifeTraderAbstract* trader		= smart_cast<CSE_ALifeTraderAbstract*>(e_entity);
	if(!trader) return;

	CharacterInfo().m_CurrentReputation.set(reputation);
	trader->m_reputation  = reputation;
}

void CInventoryOwner::ChangeReputation	(CHARACTER_REPUTATION_VALUE delta)
{
	SetReputation(Reputation() + delta);
}


void CInventoryOwner::OnItemDrop			(CInventoryItem *inventory_item)
{
	CGameObject	*object = smart_cast<CGameObject*>(this);
	VERIFY		(object);
	object->callback(GameObject::eOnItemDrop)(inventory_item->object().lua_game_object(), inventory_item->object().GetTmpPreDestroy() != 0);

	detach		(inventory_item);
}

void CInventoryOwner::OnItemDropUpdate ()
{
}

void CInventoryOwner::OnItemBelt	(CInventoryItem *inventory_item, const EItemPlace& previous_place)
{
}

void CInventoryOwner::OnItemRuck	(CInventoryItem *inventory_item, const EItemPlace& previous_place)
{
	detach		(inventory_item);
}
void CInventoryOwner::OnItemSlot	(CInventoryItem *inventory_item, const EItemPlace& previous_place)
{
	attach		(inventory_item);
}

CInventoryItem* CInventoryOwner::GetCurrentOutfit() const
{
    return inventory().m_slots[OUTFIT_SLOT].m_pIItem;
}

CInventoryItem* CInventoryOwner::GetCurrentHelmet() const
{
	return inventory().m_slots[HELMET_SLOT].m_pIItem;
}


void CInventoryOwner::on_weapon_shot_start(CWeapon *weapon)
{
}

void CInventoryOwner::on_weapon_shot_update()
{
}

void CInventoryOwner::on_weapon_shot_stop()
{
}

void CInventoryOwner::on_weapon_shot_remove(CWeapon *weapon)
{
}

void CInventoryOwner::on_weapon_hide(CWeapon *weapon)
{
}

LPCSTR CInventoryOwner::trade_section			() const
{
	const CGameObject			*game_object = smart_cast<const CGameObject*>(this);
	VERIFY						(game_object);
	return						(READ_IF_EXISTS(pSettings,r_string,game_object->SectionName(),"trade_section","trade"));
}

float CInventoryOwner::deficit_factor			(const shared_str &section) const
{
	if (!m_purchase_list)
		return					(1.f);

	return						(m_purchase_list->deficit(section));
}

void CInventoryOwner::sell_useless_items		()
{
	CGameObject					*object = smart_cast<CGameObject*>(this);

	TIItemContainer::iterator	I = inventory().allContainer_.begin();
	TIItemContainer::iterator	E = inventory().allContainer_.end();
	for ( ; I != E; ++I) {
		if ((*I)->object().CLS_ID == CLSID_IITEM_BOLT)
			continue;

		if ((*I)->object().CLS_ID == CLSID_DEVICE_PDA) {
			CPda				*pda = smart_cast<CPda*>(*I);
			VERIFY				(pda);
			if (pda->GetOriginalOwnerID() == object->ID())
				continue;
		}

		(*I)->object().DestroyObject();
	}
}

bool CInventoryOwner::AllowItemToTrade 			(CInventoryItem const * item, EItemPlace place) const
{
	return						(
		trade_parameters().enabled(
			CTradeParameters::action_sell(0),
			item->object().SectionName()
		)
	);
}

void CInventoryOwner::set_money		(u32 amount, bool bSendEvent)
{
	if(InfinitiveMoney())
		m_money					= _max(m_money, amount);
	else
		m_money					= amount;

	if(bSendEvent)
	{
		CGameObject* object = smart_cast<CGameObject*>(this);
		CSE_ALifeDynamicObject* se_obj = Alife()->objects().object(object->ID());
		CSE_ALifeTraderAbstract* se_trader = smart_cast<CSE_ALifeTraderAbstract*>(se_obj);

		se_trader->m_dwMoney = m_money;
	}
}

bool CInventoryOwner::use_default_throw_force	()
{
	return						(true);
}

float CInventoryOwner::missile_throw_force		() 
{
	NODEFAULT;
#ifdef DEBUG
	return						(0.f);
#endif
}

bool CInventoryOwner::use_throw_randomness		()
{
	return						(true);
}

bool CInventoryOwner::IsUpgradeEnabled(const CInventoryOwner* oth) const
{
	if(!m_bAllowUpgrade) // flag for temporary disabling
		return false;

	shared_str str = SpecificCharacter().CanUpgrade();

	if (str == "1" || str == "true")
	{
		return true;
	}
	else if (str == "0" || str == "false")
	{
		return false;
	}
	else
	{
		return oth->HasInfo(str);
	}
}

CPurchaseList* CInventoryOwner::get_create_purchase_list()
{
	if (!m_purchase_list)
	{
		m_purchase_list = xr_new <CPurchaseList>();
	}
	return m_purchase_list;
}

bool CInventoryOwner::is_alive()
{
	CEntityAlive* pEntityAlive = smart_cast<CEntityAlive*>(this);
	R_ASSERT(pEntityAlive);
	return (!!pEntityAlive->g_Alive());
}


void CInventoryOwner::deadbody_can_take(bool status)
{
	if (is_alive())
	{
		return;
	}
	m_deadbody_can_take = status;

	NET_Packet P;
	CGameObject::u_EventGen(P, GE_INV_OWNER_STATUS, object_id());
	P.w_u8((m_deadbody_can_take) ? 1 : 0);
	P.w_u8((m_deadbody_closed) ? 1 : 0);
	CGameObject::u_EventSend(P);
}

void CInventoryOwner::deadbody_closed(bool status)
{
	if (is_alive())
	{
		return;
	}
	m_deadbody_closed = status;

	NET_Packet P;
	CGameObject::u_EventGen(P, GE_INV_OWNER_STATUS, object_id());
	P.w_u8((m_deadbody_can_take) ? 1 : 0);
	P.w_u8((m_deadbody_closed) ? 1 : 0);
	CGameObject::u_EventSend(P);
}
