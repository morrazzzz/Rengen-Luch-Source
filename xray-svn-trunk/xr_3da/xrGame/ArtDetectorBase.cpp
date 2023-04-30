#include "stdafx.h"
#include "ArtDetectorBase.h"
#include "ui/ArtefactDetectorUI.h"

#include "inventory.h"

#include "actor.h"

#include "player_hud.h"
#include "weapon.h"
#include "level.h"

CArtDetectorBase::CArtDetectorBase()
{
	m_ui = NULL;
	m_bFastAnimMode = false;
	m_bNeedActivation = false;

	freqq = 0.f;
	cur_periodperiod = 0.f;
	snd_timetime = 0.f;
}

void CArtDetectorBase::ToggleDetector(bool bFastMode)
{
	if (GetState() == eHidden)
	{
		ShowDetector(bFastMode);
	}
	else
	{
		HideDetector(bFastMode);
	}
}

void CArtDetectorBase::OnStateSwitch(u32 S)
{
	inherited::OnStateSwitch(S);

	switch(S)
	{
	case eShowing:
		{
			TurnDetectorInternal		(true);
			m_pCurrentInventory->SetCurrentDetector(this);
			g_player_hud->attach_item	(this);
			m_sounds.PlaySound			("sndShow", Fvector().set(0,0,0), this, true, false);
			PlayHUDMotion				("anm_show", FALSE, this, GetState());
			SetPending					(TRUE);
		}break;
	case eHiding:
		{
			m_sounds.PlaySound			("sndHide", Fvector().set(0,0,0), this, true, false);
			PlayHUDMotion				(m_bFastAnimMode?"anm_hide_fast":"anm_hide", TRUE, this, GetState());
			SetPending					(TRUE);
		}break;
	case eHidden:
		{
			m_pCurrentInventory->SetCurrentDetector(NULL);
			g_player_hud->detach_item	(this);
			StopCurrentAnimWithoutCallback	();
			TurnDetectorInternal			(false);
		}break;
	case eIdle:
		{
			PlayAnimIdle				();
			SetPending					(FALSE);
		}break;
	}
}

void CArtDetectorBase::OnAnimationEnd(u32 state)
{
	inherited::OnAnimationEnd	(state);
	switch(state)
	{
	case eShowing:
		{
			SwitchState					(eIdle);
		} break;
	case eHiding:
		{
			SwitchState					(eHidden);
		} break;
	}
}

void CArtDetectorBase::UpdateXForm()
{
	CInventoryItem::UpdateXForm();
}

void CArtDetectorBase::OnActiveItem()
{
	ShowDetector(false);
	inherited::OnActiveItem();
}

void CArtDetectorBase::OnHiddenItem()
{
	HideDetector(false);
	inherited::OnHiddenItem();
}

CArtDetectorBase::~CArtDetectorBase()
{
	//m_artefacts.destroy		();
	TurnDetectorInternal	(false);
	xr_delete				(m_ui);
	if (detect_sndsnd_line)detect_snd.destroy();
}

BOOL CArtDetectorBase::SpawnAndImportSOData(CSE_Abstract* data_containing_so)
{
	TurnDetectorInternal(false);
	return		(inherited::SpawnAndImportSOData(data_containing_so));
}

void CArtDetectorBase::LoadCfg(LPCSTR section)
{

	inherited::LoadCfg(section);

	fdetect_radius			= pSettings->r_float(section, "detect_radius");//Считываем базовый радиус действия
	foverallrangetocheck	= READ_IF_EXISTS(pSettings, r_float, section, "overall_range_to_check", 50.0f);
	fortestrangetocheck		= READ_IF_EXISTS(pSettings, r_float, section, "test_range_to_check", 250.0f);
	for_test				= !!READ_IF_EXISTS(pSettings, r_bool, section, "for_test", FALSE);
	reaction_sound_off		= !!READ_IF_EXISTS(pSettings, r_bool, section, "reaction_sound_off", FALSE);

	// Подгрузим список всех артов, список прописан в линии af_to_detect_list. Пока будет так, так как по другому сделать не хватает опыта
	xr_vector<shared_str>			af_total_list;

	LPCSTR	S = READ_IF_EXISTS(pSettings, r_string, section, "af_to_detect_list", "null");

	if (S && S[0])
	{
		string128		artefact;
		int				count = _GetItemCount(S);
		for (int it = 0; it < count; ++it)
		{
			_GetItem(S, it, artefact);
			af_total_list.push_back(artefact);
		}
	}


	// Отсеим только те арты, которые построчно прописаны в конфиге детектора
	af_types.clear();

	for (u16 it = 0; it < af_total_list.size(); ++it)
	{
		u8 af_line_true_false;
		af_line_true_false = READ_IF_EXISTS(pSettings, r_u8, section, af_total_list[it].c_str(), 0);
		if (af_line_true_false == 1 || (xr_strcmp(af_total_list[it].c_str(), "all") == 0)){
			af_types.push_back(af_total_list[it]);
		}
	}

	//Для проверки
	//for (int it = 0; it < af_types.size(); ++it)
	//{
	//Msg("af_types%i == %s, it was marked true", it, af_types[it].c_str());
	//}


	m_sounds.LoadSound(section, "snd_draw", "sndShow", 0);
	m_sounds.LoadSound(section, "snd_holster", "sndHide", 0);

	//Этот звук берется по дефолту если не задана строка "sounds"
	detect_sndsnd_line = READ_IF_EXISTS(pSettings, r_string, section, "af_sound_if_no_artefact_sound", NULL);
	if (detect_sndsnd_line){
		detect_snd.create(pSettings->r_string(section, "af_sound_if_no_artefact_sound"), st_Effect, SOUND_TYPE_ITEM);
	}

	//Добавил врзможность задать разные звуки для различных артов
	af_sounds_section = READ_IF_EXISTS(pSettings, r_string, section, "sounds", "null");

	detector_section = section;

}


void CArtDetectorBase::ScheduledUpdate(u32 dt)
{
#ifdef MEASURE_UPDATES
	CTimer measure_sc_update; measure_sc_update.Start();
#endif
	
	
	inherited::ScheduledUpdate(dt);
	
	if( !IsWorking() )			return;

	Position().set(H_Parent()->Position());

	Fvector						P; 
	P.set						(H_Parent()->Position());
	
	
#ifdef MEASURE_UPDATES
	Device.Statistic->scheduler_VariousItems_ += measure_sc_update.GetElapsed_sec();
#endif
}


bool CArtDetectorBase::IsWorking()
{
	return m_bWorking && H_Parent() && H_Parent()==Level().CurrentViewEntity();
}

void CArtDetectorBase::UpdateCL()
{
#ifdef MEASURE_UPDATES
	CTimer measure_updatecl; measure_updatecl.Start();
#endif


	inherited::UpdateCL();

	if(H_Parent() != Level().CurrentEntity())
		return;

	UpdateVisibility();

	if(!IsWorking())
		return;

	UpdateWork();
	
	
#ifdef MEASURE_UPDATES
	Device.Statistic->updateCL_VariousItems_ += measure_updatecl.GetElapsed_sec();
#endif
}

void CArtDetectorBase::AfterAttachToParent()
{
	inherited::AfterAttachToParent();
}

void CArtDetectorBase::BeforeDetachFromParent(bool just_before_destroy)
{
	inherited::BeforeDetachFromParent(just_before_destroy);
}

void CArtDetectorBase::TurnDetectorInternal(bool b)
{
	m_bWorking				= b;
	if(b && m_ui==NULL)
	{
		CreateUI			();
	}else
	{
		xr_delete			(m_ui);
	}
}
///------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void CArtDetectorBase::HideDetector(bool bFastMode, bool needToReactivate)
{
	// Msg("Hide Detector, fast:%d, needActivate: %d", bFastMode, needToReactivate);
	m_bFastAnimMode = bFastMode;
	m_bNeedActivation = needToReactivate;

	if (IsHiding()) return;

	SendDeactivateItem();
}

void CArtDetectorBase::ShowDetector(bool bFastMode)
{
	// Msg("Show Detector, fast:%d", bFastMode);
	m_bFastAnimMode = bFastMode;
	m_bNeedActivation = false;

	if (!IsHidden()) return;

	PIItem iitem = m_pCurrentInventory->ActiveItem();
	CHudItem* itm = (iitem) ? iitem->cast_hud_item() : NULL;

	if (iitem == nullptr)
	{
		// no weapon, take out detector and bolt together
		SwitchToBolt();
	}
	else if (iitem->BaseSlot() != DETECTOR_SLOT && !iitem->IsSingleHand())
	{
		// Two-handed weapon, switch to bolt and activate detector later
		SwitchToBolt();
		m_bNeedActivation = true;
		return;
	}
	else if (itm->IsHiding() || !IsItemStateCompatible(itm))
	{
		// Weapon needs both hands right now, can't show
		return;
	}

	SwitchState(eShowing);
}

void CArtDetectorBase::SwitchToBolt()
{
	auto bolt = m_pCurrentInventory->ItemFromSlot(BOLT_SLOT);
	// Msg("No bolt selected or weapon is not single hand, %s.", bolt != nullptr ? "Activate bolt" : "Hide weapon");
	m_pCurrentInventory->ActivateSlot(bolt != nullptr ? BOLT_SLOT : NO_ACTIVE_SLOT);
}

bool CArtDetectorBase::IsItemStateCompatible(CHudItem* itm)
{
	if (!itm) return true;

	auto W = smart_cast<CWeapon*>(itm);
	if (!W) return true;
	return W->GetState() != CWeapon::eReload
		&& W->GetState() != CWeapon::eSwitch
		&& !W->IsZoomed();
}

void CArtDetectorBase::HideDetectorInstantly(bool needToReactivate)
{
	// Msg("Hide Detector Instantly, needActivate:%d", needToReactivate);
	SwitchState(eHidden);
	m_bNeedActivation = needToReactivate;
}

void CArtDetectorBase::UpdateWork()
{
	UpdateAf				();

	if (m_ui)
		m_ui->update			();
}

// TODO: don't run this check repeatedly, use event-driven approach instead (check detector at all relevant events)
void CArtDetectorBase::UpdateVisibility()//Подгонка нахождения детектора в левой руке под различные события худа
{
	attachable_hud_item* i0 = g_player_hud->attached_item(0);
	if (i0 != nullptr
		&& HudItemData()
		&& m_pCurrentInventory->CurrentDetector() != NULL
		&& !IsItemStateCompatible(i0->m_parent_hud_item))
	{
		HideDetectorInstantly(true);
	}
	else if (m_bNeedActivation
			&& m_pCurrentInventory->m_slots[DETECTOR_SLOT].CanBeActivated())
	{
		auto hudItem = i0 != nullptr ? i0->m_parent_hud_item : nullptr;
		if (hudItem == nullptr || (!hudItem->IsHidden() && !hudItem->IsHiding() && IsItemStateCompatible(hudItem)))
		{
			ShowDetector(false);
		}
	}
}


bool CArtDetectorBase::install_upgrade_impl(LPCSTR section, bool test)
{
	bool result = inherited::install_upgrade_impl(section, test);
	
	//Msg("Detecting radius before %f", fdetect_radius);
	result |= process_if_exists(section, "detect_radius", &CInifile::r_float, fdetect_radius, test);
	//Msg("Detecting radius after %f", fdetect_radius);
	result |= process_if_exists(section, "overall_range_to_check", &CInifile::r_float, foverallrangetocheck, test);

	result |= process_if_exists(section, "test_range_to_check", &CInifile::r_float, fortestrangetocheck, test);
	result |= process_if_exists_set(section, "for_test", &CInifile::r_bool, for_test, test);

	result |= process_if_exists_set(section, "reaction_sound_off", &CInifile::r_bool, reaction_sound_off, test);
	result |= process_if_exists_set(section, "sounds", &CInifile::r_string, af_sounds_section, test);

	LPCSTR str;
	bool result2 = process_if_exists_set(section, "af_sound_if_no_artefact_sound", &CInifile::r_string, str, test);
	if (result2 && !test)
	{
		detect_snd.create(pSettings->r_string(section, "af_sound_if_no_artefact_sound"), st_Effect, SOUND_TYPE_ITEM);
	}

	result |= result2;

	//Для проверки
	//for (int it = 0; it < af_types.size(); ++it)
	//{
	//	Msg("Afs After Upgrade af_types%i == %s, it is marked true", it, af_types[it].c_str());
	//}

	// Подгрузим список всех артов, список прописан в линии af_to_detect_list. Пока будет так, так как по другому сделать не хватает опыта
	xr_vector<shared_str>			af_total_list;

	LPCSTR	S = READ_IF_EXISTS(pSettings, r_string, detector_section, "af_to_detect_list", "null");

	if (S && S[0])
	{
		string128		artefact;
		int				count = _GetItemCount(S);
		for (int it = 0; it < count; ++it)
		{
			_GetItem(S, it, artefact);
			af_total_list.push_back(artefact);
		}
	}

	// Отсеим только те арты, которые построчно прописаны в конфиге апгрейда и добавим их в список видимых для детектора af_types

	for (u16 it = 0; it < af_total_list.size(); ++it)
	{
		u8 af_line_true_false = 0;
		//Для проверки

		//Msg("Afs processing af_total_list %i == %s", it, af_total_list[it].c_str());

		result2 = process_if_exists_set(section, af_total_list[it].c_str(), &CInifile::r_u8, af_line_true_false, test);

		if (af_line_true_false == 1){
			af_types.push_back(af_total_list[it]);
			//Msg("found true %s", af_total_list[it].c_str());
		}
	}

	///Для проверки
	//for (int it = 0; it < af_types.size(); ++it)
	//{
	//	Msg("Afs After Upgrade af_types%i == %s, it was marked true", it, af_types[it].c_str());
	//}

	result |= result2;

	return result;
}
