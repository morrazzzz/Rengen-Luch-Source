////////////////////////////////////////////////////////////////////////////
//	Module 		: eatable_item.cpp
//	Created 	: 24.03.2003
//  Modified 	: 29.01.2004
//	Author		: Yuri Dobronravin
//	Description : Eatable item
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "eatable_item.h"
#include "physic_item.h"
#include "Level.h"
#include "entity_alive.h"
#include "EntityCondition.h"
#include "ActorCondition.h"
#include "InventoryOwner.h"
#include "actor.h"
#include "UIGameCustom.h"
#include "UI/UIInventoryWnd.h"
#include "string_table.h"
#include "GameConstants.h"

CEatableItem::CEatableItem()
{
	m_fHealthInfluence = 0;
	m_fPowerInfluence = 0;
	m_fSatietyInfluence = 0;
	m_fThirstyInfluence = 0;
	m_fPsyHealthInfluence = 0;
	m_fRadiationInfluence = 0;

	m_iPortionsNum = 1;
	startingPortionsNum_ = 1;

	m_physic_item	= 0;

	bProlongedEffect = 0;
	iEffectorBlockingGroup = 0;

	notForQSlot_ = FALSE;
}

CEatableItem::~CEatableItem()
{
}

DLL_Pure *CEatableItem::_construct	()
{
	m_physic_item	= smart_cast<CPhysicItem*>(this);
	return			(inherited::_construct());
}

void CEatableItem::LoadCfg(LPCSTR section)
{
	inherited::LoadCfg(section);
	
	m_fHealthInfluence			= pSettings->r_float(section, "eat_health");
	m_fPowerInfluence			= pSettings->r_float(section, "eat_power");
	m_fSatietyInfluence			= pSettings->r_float(section, "eat_satiety");
	m_fThirstyInfluence			= pSettings->r_float(section, "eat_thirst");
	m_fPsyHealthInfluence		= pSettings->r_float(section, "eat_psy_health");
	m_fRadiationInfluence		= pSettings->r_float(section, "eat_radiation");
	m_fWoundsHealPerc			= pSettings->r_float(section, "wounds_heal_perc");
	m_falcohol					= READ_IF_EXISTS(pSettings, r_float, section, "eat_alcohol", 0.0f);

	clamp						(m_fWoundsHealPerc, 0.f, 1.f);

	m_iPortionsNum				= READ_IF_EXISTS	(pSettings,r_u32,section, "eat_portions_num", 1);

	startingPortionsNum_ = m_iPortionsNum; // remember the starting portions count

	m_fMaxPowerUpInfluence		= READ_IF_EXISTS	(pSettings,r_float,section,"eat_max_power",0.0f);
	VERIFY2						(m_iPortionsNum<10000 || m_iPortionsNum == -1, make_string("'eat_portions_num' should be < 10000. Wrong section [%s]",section));

	use_sound_line				= READ_IF_EXISTS(pSettings, r_string, section, "use_sound", NULL);

	notForQSlot_				= READ_IF_EXISTS(pSettings, r_bool, section, "not_for_quick_slot", FALSE);


	//iEffectsAffectedStat 1 = ��������
	//iEffectsAffectedStat 2 = ������������
	//iEffectsAffectedStat 3 = ��������
	//iEffectsAffectedStat 4 = ���-��������
	//iEffectsAffectedStat 5 = ���
	//iEffectsAffectedStat 6 = ����
	//iEffectsAffectedStat 7 = �������
	//iEffectsAffectedStat 8 = ���������

	bProlongedEffect = !!READ_IF_EXISTS(pSettings, r_bool, section, "use_prolonged_effect", FALSE);

	if (bProlongedEffect){

		iEffectorBlockingGroup	= READ_IF_EXISTS(pSettings, r_u8, section, "effector_blocking_group", 0);

		fItemUseTime			= READ_IF_EXISTS(pSettings, r_float, section, "item_use_time", 0.0f);

		LPCSTR templist			= READ_IF_EXISTS(pSettings, r_string, section, "effects_list", "null");
		if (templist && templist[0])
		{
			string128		effect;
			int				count = _GetItemCount(templist);
			for (int it = 0; it < count; ++it)
			{
				_GetItem(templist, it, effect);
				sEffectList.push_back(effect);
			}
		}

		string128		temp_string128;
		float			tempfloat;
		u8				tempint;
		bool			tempbool = false;
		for (u16 i = 0; i < sEffectList.size(); ++i)
		{

			xr_sprintf(temp_string128, "%s_rate", sEffectList[i].c_str());

			tempfloat = READ_IF_EXISTS(pSettings, r_float, section, temp_string128, 0.0f);
			fEffectsRate.push_back(tempfloat);

			xr_sprintf(temp_string128, "%s_dur", sEffectList[i].c_str());

			tempfloat = READ_IF_EXISTS(pSettings, r_float, section, temp_string128, 0.0f);
			fEffectsDur.push_back(tempfloat);

			xr_sprintf(temp_string128, "%s_affected_stat", sEffectList[i].c_str());

			tempint = READ_IF_EXISTS(pSettings, r_u8, section, temp_string128, 1);
			iEffectsAffectedStat.push_back(tempint);

			xr_sprintf(temp_string128, "%s_is_booster", sEffectList[i].c_str());

			tempbool = !!READ_IF_EXISTS(pSettings, r_bool, section, temp_string128, FALSE);
			BoosterParams tembusterparams;
			tembusterparams.EffectIsBooster = (u8)tempbool;
			VectorBoosterParam.push_back(tembusterparams);

			if (tempbool)
			{

				xr_sprintf(temp_string128, "%s_hit_absorbation_sect", sEffectList[i].c_str());

				LPCSTR tempsection = pSettings->r_string(section, temp_string128);
				CHitImmunity tempimun;

				tempimun.LoadImmunities(tempsection, pSettings);
				VectorBoosterParam[i].BoosterHitImmunities = tempimun;
				VectorBoosterParam[i].HitImmunitySect = tempsection;

				xr_sprintf(temp_string128, "%s_additional_weight", sEffectList[i].c_str());

				tempfloat = READ_IF_EXISTS(pSettings, r_float, section, temp_string128, 0.0f);
				VectorBoosterParam[i].AddWeight = tempfloat;
			}

		}
		
		//for (int i = 0; i < sEffectList.size(); ++i)
		//{
		//	Msg("fEffectsRate %i = %f", i, fEffectsRate[i]);
		//	Msg("fEffectsDur %i = %f", i, fEffectsDur[i]);
		//	Msg("fUseTime %i = %f", i, fItemUseTime);
		//	Msg("iEffectsAffectedStat %i = %i", i, iEffectsAffectedStat[i]);
		//}
		
	}
}

BOOL CEatableItem::SpawnAndImportSOData(CSE_Abstract* data_containing_so)
{
	if (!inherited::SpawnAndImportSOData(data_containing_so)) return FALSE;

	CSE_ALifeEatableItem* server_eatable_item = smart_cast<CSE_ALifeEatableItem*>(data_containing_so);
	R_ASSERT(server_eatable_item);

	m_iPortionsNum = server_eatable_item->numOfPortionsServer_;

	return TRUE;
};

void CEatableItem::ExportDataToServer(NET_Packet& P)
{
	inherited::ExportDataToServer(P);

	P.w_u16(u16(m_iPortionsNum));
}

bool CEatableItem::Useful() const
{
	if(!inherited::Useful()) return false;

	//��������� �� ��� �� ��� �������
	if(Empty()) return false;

	return true;
}

void CEatableItem::BeforeDetachFromParent(bool just_before_destroy)
{
	if(!Useful()) 
	{
		object().setVisible(FALSE);
		object().setEnabled(FALSE);
		if (m_physic_item)
			m_physic_item->m_ready_to_destroy	= true;
	}
	inherited::BeforeDetachFromParent(just_before_destroy);
}

void CEatableItem::save				(NET_Packet &packet)
{
	inherited::save				(packet);
}

void CEatableItem::load				(IReader &packet)
{
	inherited::load				(packet);
}

extern bool debug_effects;
bool CEatableItem::UseBy (CEntityAlive* entity_alive)
{
	CInventoryOwner* IO = smart_cast<CInventoryOwner*>(entity_alive);

	R_ASSERT(IO);
	R_ASSERT(m_pCurrentInventory == IO->m_inventory);
	R_ASSERT(object().H_Parent()->ID() == entity_alive->ID());

	if (bProlongedEffect) // ���� �������� �����, �� ������������ �������� ��������. ���� ��� - ���������� �����������
	{ 

		CActor* ActorEntity = smart_cast<CActor*>(entity_alive);

		if (ActorEntity)
			return ProlongedUse(ActorEntity);
		else
		{	
			// ��� ���
			InstantUse(entity_alive);

			return true;
		}
	}
	else
	{
		InstantUse(entity_alive);

		CActor* ActorEntity = smart_cast<CActor*>(entity_alive);

		if (ActorEntity && use_sound_line)
			CurrentGameUI()->m_InventoryMenu->PlayUseSound(use_sound_line);

		return true;
	}
}

void CEatableItem::SubstructPortion()
{
	if (m_iPortionsNum != -1)
	{
		//��������� ���������� ������
		if (m_iPortionsNum > 0)
			--(m_iPortionsNum);
		else
			m_iPortionsNum = 0; // (�������� ���������� � ����. ����)
	}
}

bool CEatableItem::ProlongedUse(CActor* actor)
{
	bool hands_are_blocked = false;
	bool same_blockin_ggroup = false;
	for (u16 i = 0; i < actor->conditions().Eat_Effects.size(); ++i)
	{
		Eat_Effect effector = actor->conditions().Eat_Effects[i];

		if (actor->conditions().fHandsHideTime > EngineTime()) // ��������� �� ������ �� ���� ������ ����������� ���������
		{
			hands_are_blocked = true;
		}
		if (iEffectorBlockingGroup != 0 && iEffectorBlockingGroup == effector.BlockingGroup) //0 - ���� ������ �����. ��������� ����������� �� ������������� ��� ������������ ���������
		{
			same_blockin_ggroup = true;
		}
	}

	if (!hands_are_blocked && !same_blockin_ggroup) // ���� ��� ����, ������ ������� ��������
	{
		actor->SetWeaponHideState(INV_STATE_BLOCK_ALL, true);	//������ ����
		actor->conditions().fHandsHideTime = fItemUseTime + EngineTime();

		for (u16 i = 0; i < sEffectList.size(); ++i)
		{
			Eat_Effect eat_effect;
			eat_effect.DurationExpiration = EngineTime() + fEffectsDur[i] + fItemUseTime;
			eat_effect.Rate = fEffectsRate[i];
			eat_effect.UseTimeExpiration = EngineTime() + fItemUseTime;
			eat_effect.BlockingGroup = iEffectorBlockingGroup; //��� ��� � �������� ������ ����� ��������, ����� ������� �� ���� ���� ��������
			eat_effect.AffectedStat = iEffectsAffectedStat[i];
			eat_effect.BoosterParam = VectorBoosterParam[i];

			if (eat_effect.BoosterParam.EffectIsBooster)
			{
				actor->conditions().m_fBoostersAddWeight += eat_effect.BoosterParam.AddWeight;
			}

			if (debug_effects)
				Msg("Setting Up Effect: [%i] EngineTime() %f DurationExpiration %f, UseTimeExpiration = %f, Rate = %f, AffectedStat = %u, BlockingGroup = %u, EffectIsBooster = %d, AddWeight = %f, HitImmunitySect = %s",
				i, EngineTime(), eat_effect.DurationExpiration, eat_effect.UseTimeExpiration, eat_effect.Rate, eat_effect.AffectedStat,
				eat_effect.BlockingGroup, eat_effect.BoosterParam.EffectIsBooster, eat_effect.BoosterParam.AddWeight, eat_effect.BoosterParam.HitImmunitySect.c_str());

			actor->conditions().Eat_Effects.push_back(eat_effect);
		}

		SubstructPortion();

		if (use_sound_line)
		{
			CurrentGameUI()->m_InventoryMenu->PlayUseSound(use_sound_line);
		}

		return true;
	}

	SDrawStaticStruct* HudMessage = CurrentGameUI()->AddCustomStatic("inv_hud_message", true);
	HudMessage->m_endTime = EngineTime() + 3.0f;

	string1024 str;
	if (hands_are_blocked)
		xr_sprintf(str, "%s", *CStringTable().translate("st_item_hands_are_buisy"));
	else
		xr_sprintf(str, "%s", *CStringTable().translate("st_item_refuse"));

	HudMessage->wnd()->TextItemControl()->SetText(str);

	return false;
}

void CEatableItem::InstantUse(CEntityAlive* entity_alive)
{
	entity_alive->conditions().ChangeHealth(m_fHealthInfluence);
	entity_alive->conditions().ChangePower(m_fPowerInfluence);
	entity_alive->conditions().ChangeSatiety(m_fSatietyInfluence);
	entity_alive->conditions().ChangeThirsty(m_fThirstyInfluence);
	entity_alive->conditions().ChangePsyHealth(m_fPsyHealthInfluence);
	entity_alive->conditions().ChangeRadiation(m_fRadiationInfluence);
	entity_alive->conditions().ChangeBleeding(m_fWoundsHealPerc);
	entity_alive->conditions().SetMaxPower(entity_alive->conditions().GetMaxPower() + m_fMaxPowerUpInfluence);

	SubstructPortion();
}

float CEatableItem::Weight() const
{
	float res = inherited::Weight();

	float temp = res * (float)GetPortionsNum() / (float)GetBasePortionsNum();

	if (temp != res)
	{
		temp *= 1.0f + GameConstants::GetEIContainerWeightPerc();
	}

	return temp;
}

u32 CEatableItem::Cost() const
{
	float res = (float)inherited::Cost();

	res *= (float)GetPortionsNum() / (float)GetBasePortionsNum();

	u8 missing_portions = u8(GetBasePortionsNum() - GetPortionsNum());

	res *= 1.f - GameConstants::GetCostPenaltyForMissingPortions(missing_portions); // nobody wants to pay for "open box" products =)

	return u32(res);
}