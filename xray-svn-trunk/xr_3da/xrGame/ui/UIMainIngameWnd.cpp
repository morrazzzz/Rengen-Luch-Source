#include "stdafx.h"
#include <functional>

#include "UIMainIngameWnd.h"
#include "UIMessagesWindow.h"
#include "UIInventoryUtilities.h"
#include "UIScrollView.h"
#include "UIXmlInit.h"
#include "UIPdaMsgListItem.h"
#include "../UIZoneMap.h"

#include <dinput.h>
#include "../actor.h"
#include "../HUDManager.h"
#include "../PDA.h"
#include "../character_info.h"
#include "../inventory.h"
#include "../UIGameSP.h"
#include "../weaponmagazined.h"
#include "../Grenade.h"

#include "../game_cl_base.h"
#include "../level.h"

#include "../torch.h"

#include "../ActorState.h"
#include "../alife_registry_wrappers.h"
#include "../actorcondition.h"
#include "../clsid_game.h"
#include "../mounted_turret.h"
#include "../game_news.h"

#include "../attachable_item.h"
#include "../../xr_input.h"
#include "UICellItemFactory.h"

using namespace InventoryUtilities;

#define		DEFAULT_MAP_SCALE			1.f
#define		SHOW_INFO_SPEED				0.5f

CUIMainIngameWnd::CUIMainIngameWnd()
{
	m_pActor					= NULL;
	m_pWeapon					= NULL;
	m_pGrenade					= NULL;
	m_pItem						= NULL;
	UIZoneMap					= xr_new <CUIZoneMap>();
	m_pPickUpItem				= NULL;

	uiPickUpItemIconNew_		= nullptr;
	fuzzyShowInfo_				= 0.f;

	showMiniMapCond_			= CondNotSet;

	useCircleTextureFrame		= 0;
}

#include "UIProgressShape.h"
extern CUIProgressShape* g_MissileForceShape;

CUIMainIngameWnd::~CUIMainIngameWnd()
{
	DestroyFlashingIcons		();
	xr_delete					(UIZoneMap);
	HUD_SOUND_ITEM::DestroySound		(m_contactSnd);
	xr_delete					(g_MissileForceShape);
	xr_delete					(uiPickUpItemIconNew_);
}

void CUIMainIngameWnd::Init()
{
	CUIXml						uiXml;

	string128		XmlName;
	xr_sprintf		(XmlName, "maingame_%d.xml", ui_hud_type);

	uiXml.Load					(CONFIG_PATH, UI_PATH, XmlName);
	
	CUIXmlInit					xml_init;
	CUIWindow::SetWndRect		(Frect().set(0,0, UI_BASE_WIDTH, UI_BASE_HEIGHT));

	Enable(false);


	AttachChild					(&UIStaticHealth);
	xml_init.InitStatic			(uiXml, "static_health", 0, &UIStaticHealth);

	AttachChild					(&UIWeaponBack);
	xml_init.InitStatic			(uiXml, "static_weapon", 0, &UIWeaponBack);

	UIWeaponBack.AttachChild	(&UIWeaponSignAmmo);
	xml_init.InitStatic			(uiXml, "static_ammo", 0, &UIWeaponSignAmmo);
	UIWeaponSignAmmo.SetElipsis	(CUIStatic::eepEnd, 2);

	UIWeaponBack.AttachChild	(&UIWeaponIcon);
	xml_init.InitStatic			(uiXml, "static_wpn_icon", 0, &UIWeaponIcon);

	UIWeaponBack.AttachChild	(&UIWeaponFiremode);
	xml_init.InitStatic			(uiXml, "static_wpn_firemode", 0, &UIWeaponFiremode);

	UIWeaponIcon.SetShader		(GetEquipmentIconsShader());
	UIWeaponIcon_rect			= UIWeaponIcon.GetWndRect();

	AttachChild(&UIStaticTime);
	xml_init.InitStatic(uiXml, "static_time", 0, &UIStaticTime);
	AttachChild(&UIStaticDate);
	xml_init.InitStatic(uiXml, "static_date", 0, &UIStaticDate);
	AttachChild(&UIStaticTorch);
	xml_init.InitStatic(uiXml, "static_flashlight", 0, &UIStaticTorch);
	AttachChild(&UIStaticTurret);
	xml_init.InitStatic(uiXml, "static_turret", 0, &UIStaticTurret);
	AttachChild(&UIActorStateIcons);
	UIActorStateIcons.Init();

	//---------------------------------------------------------

	UIWeaponIcon.Enable			(false);

	UIZoneMap->Init				();
	UIZoneMap->SetScale			(DEFAULT_MAP_SCALE);

	xml_init.InitStatic					(uiXml, "static_pda_online", 0, &UIPdaOnline);
	UIZoneMap->Background().AttachChild	(&UIPdaOnline);

	showMiniMapCond_ = (EDifficultyShowCondition)xml_init.ParseDiffCond(uiXml.Read("show_minimap_cond", 0, ""));

	//Полоса прогресса здоровья
	UIStaticHealth.AttachChild	(&UIHealthBar);
	xml_init.InitProgressBar	(uiXml, "progress_bar_health", 0, &UIHealthBar);

	UIStaticTorch.AttachChild(&UIFlashlightBar);
	xml_init.InitProgressBar(uiXml, "progress_bar_flashlight", 0, &UIFlashlightBar);
	UIFlashlightBar.Show(false);
	UIStaticTorch.Show(false);

	UIStaticTurret.AttachChild(&UITurretBar);
	xml_init.InitProgressBar(uiXml, "progress_bar_turret", 0, &UITurretBar);
	UITurretBar.Show(false);
	UIStaticTurret.Show(false);

	// Подсказки, которые возникают при наведении прицела на объект
	AttachChild					(&UIStaticQuickHelp);
	xml_init.InitTextWnd			(uiXml, "quick_info", 0, &UIStaticQuickHelp);

	uiXml.SetLocalRoot			(uiXml.GetRoot());

	m_UIIcons					= xr_new <CUIScrollView>(); m_UIIcons->SetAutoDelete(true);
	xml_init.InitScrollView		(uiXml, "icons_scroll_view", 0, m_UIIcons);
	AttachChild					(m_UIIcons);

	// Загружаем иконки 
	xml_init.InitStatic			(uiXml, "invincible_static", 0, &UIInvincibleIcon);
	UIInvincibleIcon.Show		(false);

	AttachChild(&useCircle);
	xml_init.InitStatic(uiXml, "use_circle_static", 0, &useCircle);

	uiXml.SetLocalRoot(uiXml.NavigateToNode("use_circle_static", 0));

	LPCSTR nodevalue = uiXml.Read("use_circle_static_texture_name", 0, "ui\\ui_progress_circle");
	useCircleRectX = uiXml.ReadFlt("frame_rect_x", 0, 40.f);
	useCircleRectY = uiXml.ReadFlt("frame_rect_y", 0, 40.f);
	useCircleFrames = uiXml.ReadInt("frames_cnt", 0, 0);

	useCircle.InitTextureEx(nodevalue);

	uiXml.SetLocalRoot(uiXml.GetRoot());

	shared_str warningStrings[7] = 
	{	
		"jammed",
		"radiation",
		"wounds",
		"starvation",
		"thirst",
		"fatigue",
		"invincible"
	};

	// Загружаем пороговые значения для индикаторов
	EWarningIcons j = ewiWeaponJammed;
	while (j < ewiInvincible)
	{
		// Читаем данные порогов для каждого индикатора
		shared_str cfgRecord = pSettings->r_string("la_main_ingame_indicators_thresholds", *warningStrings[static_cast<int>(j) - 1]);
		u32 count = _GetItemCount(*cfgRecord);

		R_ASSERT3(count==3, "Item count in parameter '%s' in [la_main_ingame_indicators_thresholds] should be 3.", *warningStrings[static_cast<int>(j) - 1]);

		char	singleThreshold[8];
		float	f = 0;
		for (u32 k = 0; k < count; ++k)
		{
			_GetItem(*cfgRecord, k, singleThreshold);
			sscanf(singleThreshold, "%f", &f);

			m_Thresholds[j].push_back(f);
		}

		j = static_cast<EWarningIcons>(j + 1);
	}


	// Flashing icons initialize
	uiXml.SetLocalRoot						(uiXml.NavigateToNode("flashing_icons"));
	InitFlashingIcons						(&uiXml);

	uiXml.SetLocalRoot						(uiXml.GetRoot());
	
	AttachChild								(&UICarPanel);
	xml_init.InitWindow						(uiXml, "car_panel", 0, &UICarPanel);
	Frect wndrect = UICarPanel.GetWndRect();
	UICarPanel.Init							(wndrect.x1, wndrect.y1, wndrect.width(), wndrect.height());

	AttachChild								(&UIMotionIcon);
	UIMotionIcon.Init						();

	HUD_SOUND_ITEM::LoadSound("maingame_ui", "snd_new_contact", m_contactSnd, 0, SOUND_TYPE_IDLE);
}

void CUIMainIngameWnd::Draw()
{
	if(!m_pActor) return;

	UIMotionIcon.SetNoise		((s16)(0xffff&iFloor(m_pActor->m_snd_noise*100.0f)));
	CUIWindow::Draw				();

	if (showMiniMapCond_ != CondNotSet)
	{
		if (showMiniMapCond_ != CondNever && g_SingleGameDifficulty < showMiniMapCond_)
		{
			UIZoneMap->Render();
		}
	}
	else
		UIZoneMap->Render();

	RenderQuickInfos			();

	if (uiPickUpItemIconNew_ && fuzzyShowInfo_ > 0.5f)
		uiPickUpItemIconNew_->Draw();
}

void CUIMainIngameWnd::SetAmmoIcon (const shared_str& sect_name)
{
	if ( !sect_name.size() )
	{
		UIWeaponIcon.Show			(false);
		return;
	};

	UIWeaponIcon.Show			(true);
	//properties used by inventory menu
	Frect texture_rect;
	texture_rect.x1					= pSettings->r_float( sect_name,  "inv_grid_x")		*INV_GRID_WIDTH;
	texture_rect.y1					= pSettings->r_float( sect_name,  "inv_grid_y")		*INV_GRID_HEIGHT;
	texture_rect.x2					= pSettings->r_float( sect_name, "inv_grid_width")	*INV_GRID_WIDTH;
	texture_rect.y2					= pSettings->r_float( sect_name, "inv_grid_height")	*INV_GRID_HEIGHT;
	texture_rect.rb.add				(texture_rect.lt);

	UIWeaponIcon.GetUIStaticItem().SetTextureRect(texture_rect);
	UIWeaponIcon.SetStretchTexture(true);

	// now perform only width scale for ammo, which (W)size >2
	// all others ammo (1x1, 1x2) will be not scaled (original picture)
	float w = ((texture_rect.width() < 2.f*INV_GRID_WIDTH)?0.5f:1.f)*UIWeaponIcon_rect.width();
	float h = UIWeaponIcon_rect.height();//1 cell

	float x = UIWeaponIcon_rect.x1;
	if (texture_rect.width() < 2.f*INV_GRID_WIDTH)
		x += ( UIWeaponIcon_rect.width() - w) / 2.0f;

	UIWeaponIcon.SetWndPos	(Fvector2().set(x, UIWeaponIcon_rect.y1));
	
	UIWeaponIcon.SetWidth	(w);
	UIWeaponIcon.SetHeight	(h);
};

void CUIMainIngameWnd::Update()
{
	m_pActor = smart_cast<CActor*>(Level().CurrentViewEntity());
	if (!m_pActor) 
	{
		m_pItem					= NULL;
		m_pWeapon				= NULL;
		m_pGrenade				= NULL;
		CUIWindow::Update		();
		return;
	}
	BOOL Device_dwFrame = CurrentFrame() % 30;
	if( !Device_dwFrame)
	{
			string256				text_str;
			CPda* _pda	= m_pActor->GetPDA();
			u32 _cn		= 0;
			if(_pda && 0!= (_cn=_pda->ActiveContactsNum()) )
			{
				xr_sprintf(text_str, "%d", _cn);

				if (!psHUD_Flags.is(HUD_NPC_COUNTER))
				{
					UIPdaOnline.TextItemControl()->SetText("");
				}
				else
				{
					UIPdaOnline.TextItemControl()->SetText(text_str);
				}
			}
			else
			{
				UIPdaOnline.TextItemControl()->SetText("");
			}
	};

	if (!(CurrentFrame() % 5))
	{

		if (!(CurrentFrame() % 30))
		{
			BOOL b_God = GodMode();
			if(b_God)
				SetWarningIconColor	(ewiInvincible,0xffffffff);
			else
				SetWarningIconColor	(ewiInvincible,0x00ffffff);
		}

		UpdateActiveItemInfo				();

		EWarningIcons i					= ewiWeaponJammed;

		while (i < ewiInvincible)
		{
			float value = 0.f;
			EActorState NewState = eJammedInactive;
			EActorState	InactiveState = eJammedInactive;

			switch (i)
			{
			case ewiWeaponJammed:
				if (m_pWeapon)
					value = 1 - m_pWeapon->GetConditionToShow();
				NewState = InactiveState = eJammedInactive;
				break;
			case ewiRadiation:
				value = m_pActor->conditions().GetRadiation();
				NewState = InactiveState = eRadiationInactive;
				break;
			case ewiWound:
				value = m_pActor->conditions().BleedingSpeed();
				NewState = InactiveState = eBleedingInactive;
				break;
			case ewiStarvation:
				value = 1 - m_pActor->conditions().GetSatiety();
				NewState = InactiveState = eHungerInactive;
				break;
			case ewiThirst:
				value = 1 - m_pActor->conditions().GetThirsty();
				NewState = InactiveState = eThirstInactive;
				break;
			case ewiPsyHealth:
				value = 1 - m_pActor->conditions().GetPsyHealth();
				NewState = InactiveState = ePsyHealthInactive;
				break;
			default:
				R_ASSERT(!"Unknown type of warning icon");
			}

			xr_vector<float>::reverse_iterator	rit;

			// Сначала проверяем на точное соответсвие
			rit  = std::find(m_Thresholds[i].rbegin(), m_Thresholds[i].rend(), value);

			// Если его нет, то берем последнее меньшее значение ()
			if (rit == m_Thresholds[i].rend())
				rit = std::find_if(m_Thresholds[i].rbegin(), m_Thresholds[i].rend(), std::bind2nd(std::less<float>(), value));

			// Минимальное и максимальное значения границы
			float min = m_Thresholds[i].front();
			float max = m_Thresholds[i].back();

			if (rit != m_Thresholds[i].rend())
			{
				float v = *rit;

				if (fsimilar(min, v))
					NewState = EActorState(InactiveState + 3); // green
				else if (fsimilar(max, v))
					NewState = EActorState(InactiveState + 1); // red
				else
					NewState = EActorState(InactiveState + 2); // yellow
			}

			for (u8 j = 0; j < 4; j++)
			{
				EActorState Icon = EActorState(InactiveState + j);
				if (Icon == NewState)
					m_pActor->SetIconState(Icon, true);
				else
					m_pActor->SetIconState(Icon, false);
			}

			i = (EWarningIcons)(i + 1);
		}
	}

	// health&armor
	UIHealthBar.SetProgressPos		(m_pActor->GetfHealth()*100.0f);
	UIMotionIcon.SetPower			(m_pActor->conditions().GetPower()*100.0f);

	if (psHUD_Flags.test(HUD_SHOW_CLOCK))
	{
		if (!UIStaticTime.IsShown())
			UIStaticTime.Show(true);
		if (!UIStaticDate.IsShown())
			UIStaticDate.Show(true);
		UIStaticTime.TextItemControl()->SetText(*GetGameTimeAsString(etpTimeToMinutes));
		UIStaticDate.TextItemControl()->SetText(*GetGameDateAsString(edpDateToDay));
	}
	else
	{
		if (UIStaticTime.IsShown())
			UIStaticTime.Show(false);
		if (UIStaticDate.IsShown())
			UIStaticDate.Show(false);
	}

	CTorch *flashlight = m_pActor->GetCurrentTorch();
	if (flashlight)
	{
		if (flashlight->IsSwitchedOn())
		{
			if (!UIStaticTorch.IsShown())
				UIStaticTorch.Show(true);
			if (!UIFlashlightBar.IsShown())
				UIFlashlightBar.Show(true);
			UIFlashlightBar.SetProgressPos(100.0f * ((flashlight->GetBatteryStatus()) / (flashlight->GetBatteryLifetime())));
		}
		else
		{
			if (UIStaticTorch.IsShown())
				UIStaticTorch.Show(false);
			if (UIFlashlightBar.IsShown())
				UIFlashlightBar.Show(false);
		}
	} else {
		if (UIStaticTorch.IsShown())
			UIStaticTorch.Show(false);
		if (UIFlashlightBar.IsShown())
			UIFlashlightBar.Show(false);
	}

	if (m_pActor->UsingTurret())
	{
		if (!UIStaticTurret.IsShown())
			UIStaticTurret.Show(true);
		if (!UITurretBar.IsShown())
			UITurretBar.Show(true);
		UITurretBar.SetProgressPos(100.0f * ((float)m_pActor->GetTurretTemp()) / MAX_FIRE_TEMP);
	}
	else
	{
		UIStaticTurret.Show(false);
		UITurretBar.Show(false);
	}
	UIZoneMap->UpdateRadar			(Device.vCameraPosition);
	float h, p;
	Device.vCameraDirection.getHP	(h,p);
	UIZoneMap->SetHeading			(-h);
	UIActorStateIcons.SetIcons		(m_pActor->GetIconsState());

	CUIWindow::Update				();

	fuzzyShowInfo_ += SHOW_INFO_SPEED * TimeDelta();

	if (uiPickUpItemIconNew_ && fuzzyShowInfo_ > 0.9f) // makes additional icons and text show with a delay(portions, upgrade icons, ammo count and other)
		uiPickUpItemIconNew_->Update();

	// Update use circle
	if (Actor()->timeUseAccum > 0.f)
	{
		useCircle.Show(true);

		float needed_time = Actor()->currentUsable.useDelay;
		float delta = Actor()->timeUseAccum / needed_time;
		u8 target_circle_frame = u8(useCircleFrames * delta);

		Frect rect;
		rect.set(useCircleRectX * target_circle_frame, 0, useCircleRectX * (target_circle_frame + 1), useCircleRectY);

		useCircle.SetTextureRect(rect);
	}
	else
	{
		useCircle.Show(false);
	}
}

bool CUIMainIngameWnd::OnKeyboardPress(int dik)
{
	bool flag = false;

	if (CAttachableItem::m_dbgItem){
		static float rot_d = deg2rad(0.5f);
		static float mov_d = 0.01f;
		bool shift = !!pInput->iGetAsyncKeyState(DIK_LSHIFT);
		flag = true;
		switch (dik)
		{
			// Shift +x
		case DIK_Q:
			if (shift)	CAttachableItem::rot_dx(rot_d);
			else		CAttachableItem::mov_dx(rot_d);
			break;
			// Shift -x
		case DIK_E:
			if (shift)	CAttachableItem::rot_dx(-rot_d);
			else		CAttachableItem::mov_dx(-rot_d);
			break;
			// Shift +z
		case DIK_A:
			if (shift)	CAttachableItem::rot_dy(rot_d);
			else		CAttachableItem::mov_dy(rot_d);
			break;
			// Shift -z
		case DIK_D:
			if (shift)	CAttachableItem::rot_dy(-rot_d);
			else		CAttachableItem::mov_dy(-rot_d);
			break;
			// Shift +y
		case DIK_S:
			if (shift)	CAttachableItem::rot_dz(rot_d);
			else		CAttachableItem::mov_dz(rot_d);
			break;
			// Shift -y
		case DIK_W:
			if (shift)	CAttachableItem::rot_dz(-rot_d);
			else		CAttachableItem::mov_dz(-rot_d);
			break;

		case DIK_SUBTRACT:
			if (shift)	rot_d -= deg2rad(0.01f);
			else		mov_d -= 0.001f;
			Msg("rotation delta=[%f]; moving delta=[%f]", rot_d, mov_d);
			break;
		case DIK_ADD:
			if (shift)	rot_d += deg2rad(0.01f);
			else		mov_d += 0.001f;
			Msg("rotation delta=[%f]; moving delta=[%f]", rot_d, mov_d);
			break;

		case DIK_P:
			Msg("LTX section [%s]", *CAttachableItem::m_dbgItem->item().object().SectionName());
			Msg("attach_angle_offset [%f,%f,%f]", VPUSH(CAttachableItem::get_angle_offset()));
			Msg("attach_position_offset [%f,%f,%f]", VPUSH(CAttachableItem::get_pos_offset()));
			break;
		default:
			flag = false;
			break;
		}
		if (flag)return true;;
	}

	if(Level().IR_GetKeyState(DIK_LSHIFT) || Level().IR_GetKeyState(DIK_RSHIFT))
	{
		switch(dik)
		{
		case DIK_NUMPADMINUS:
			UIZoneMap->ZoomOut();
			return true;
			break;
		case DIK_NUMPADPLUS:
			UIZoneMap->ZoomIn();
			return true;
			break;
		}
	}
	else
	{
		switch(dik)
		{
		case DIK_NUMPADMINUS:
			//.HideAll();
			CurrentGameUI()->ShowGameIndicators(IB_DEVELOPER, false);
			CurrentGameUI()->ShowHUDEffects(HUDEFFB_SCRIPT_1, false);
			CurrentGameUI()->ShowCrosshair(CB_DEVELOPER, false);
			return true;
			break;
		case DIK_NUMPADPLUS:
			//.ShowAll();
			CurrentGameUI()->ShowGameIndicators(IB_DEVELOPER, true);
			CurrentGameUI()->ShowHUDEffects(HUDEFFB_SCRIPT_1, true);
			CurrentGameUI()->ShowCrosshair(CB_DEVELOPER, true);
			return true;
			break;
		}
	}
	return false;
}


void CUIMainIngameWnd::RenderQuickInfos()
{
	if (!m_pActor)
		return;

	static CGameObject *pObject			= NULL;
	LPCSTR actor_action					= m_pActor->GetDefaultActionForObject();
	UIStaticQuickHelp.Show				(NULL!=actor_action);

	if(NULL!=actor_action){
		if(stricmp(actor_action,UIStaticQuickHelp.GetText()))
			UIStaticQuickHelp.SetTextST				(actor_action);
	}

	if (pObject!=m_pActor->ObjectWeLookingAt())
	{
		UIStaticQuickHelp.SetTextST				(actor_action?actor_action:" ");
		UIStaticQuickHelp.ResetColorAnimation		();
		pObject	= m_pActor->ObjectWeLookingAt	();
	}
}

void CUIMainIngameWnd::ReceiveNews(GAME_NEWS_DATA* news)
{
	VERIFY(news->texture_name.size());

	CurrentGameUI()->m_pMessagesWnd->AddIconedPdaMessage(*(news->texture_name), news->tex_rect, news->SingleLineText(), news->show_time);
}

void CUIMainIngameWnd::SetWarningIconColor(CUIStatic* s, const u32 cl)
{
	int bOn = (cl>>24);
	bool bIsShown = s->IsShown();

	if(bOn)
		s->SetTextureColor	(cl);

	if(bOn&&!bIsShown){
		m_UIIcons->AddWindow	(s, false);
		s->Show					(true);
	}

	if(!bOn&&bIsShown){
		m_UIIcons->RemoveWindow	(s);
		s->Show					(false);
	}
}

void CUIMainIngameWnd::SetWarningIconColor(EWarningIcons icon, const u32 cl)
{
	bool bMagicFlag = true;

	// Задаем цвет требуемой иконки
	switch(icon)
	{
	case ewiAll:
		bMagicFlag = false;
	case ewiInvincible:
		SetWarningIconColor		(&UIInvincibleIcon, cl);
		if (bMagicFlag) break;
		break;
	default:
		R_ASSERT(!"Unknown warning icon type");
	}
}

void CUIMainIngameWnd::TurnOffWarningIcon(EWarningIcons icon)
{
	SetWarningIconColor(icon, 0x00ffffff);
}


void CUIMainIngameWnd::SetFlashIconState_(EFlashingIcons type, bool enable)
{
	// Включаем анимацию требуемой иконки
	FlashingIcons_it icon = m_FlashingIcons.find(type);
	R_ASSERT2(icon != m_FlashingIcons.end(), "Flashing icon with this type not existed");
	icon->second->Show(enable);
}

#pragma todo("if hud will not include flashing icons, then delete")

void CUIMainIngameWnd::InitFlashingIcons(CUIXml* node)
{
	const char * const flashingIconNodeName = "flashing_icon";
	int staticsCount = node->GetNodesNum("", 0, flashingIconNodeName);

	CUIXmlInit xml_init;
	CUIStatic *pIcon = NULL;
	// Пробегаемся по всем нодам и инициализируем из них статики
	for (int i = 0; i < staticsCount; ++i)
	{
		pIcon = xr_new <CUIStatic>();
		xml_init.InitStatic(*node, flashingIconNodeName, i, pIcon);
		shared_str iconType = node->ReadAttrib(flashingIconNodeName, i, "type", "none");

		// Теперь запоминаем иконку и ее тип
		EFlashingIcons type = efiPdaTask;

		if (iconType == "pda")
			type = efiPdaTask;
		else if (iconType == "mail")
			type = efiMail;
		else if (iconType == "encyclopedia")
			type = efiEncyclopedia;
		else
			R_ASSERT(!"Unknown type of mainingame flashing icon");

		R_ASSERT2(m_FlashingIcons.find(type) == m_FlashingIcons.end(), "Flashing icon with this type already exists");

		CUIStatic* &val	= m_FlashingIcons[type];
		val			= pIcon;

		AttachChild(pIcon);
		pIcon->Show(false);
	}
}

void CUIMainIngameWnd::DestroyFlashingIcons()
{
	for (FlashingIcons_it it = m_FlashingIcons.begin(); it != m_FlashingIcons.end(); ++it)
	{
		DetachChild(it->second);
		xr_delete(it->second);
	}

	m_FlashingIcons.clear();
}

void CUIMainIngameWnd::UpdateFlashingIcons()
{
	for (FlashingIcons_it it = m_FlashingIcons.begin(); it != m_FlashingIcons.end(); ++it)
	{
		it->second->Update();
	}
}

void CUIMainIngameWnd::AnimateContacts(bool b_snd)
{
	UIPdaOnline.ResetXformAnimation	();

	if (CurrentGameUI()->GameIndicatorsShown())
	{
		if (b_snd && UIPdaOnline.GameDifficultyAllows() && psHUD_Flags.is(HUD_NPC_COUNTER))
			HUD_SOUND_ITEM::PlaySound	(m_contactSnd, Fvector().set(0,0,0), 0, true );
	}
}


void CUIMainIngameWnd::SetPickUpItem(CInventoryItem* PickUpItem)
{
	if (m_pPickUpItem != PickUpItem)
		xr_delete(uiPickUpItemIconNew_);

	if (m_pPickUpItem == PickUpItem)
		return;

	m_pPickUpItem = PickUpItem;

	if (!m_pPickUpItem || !m_pPickUpItem->IsPickUpVisible())
		return;

	uiPickUpItemIconNew_ = create_cell_item(m_pPickUpItem); // use inventory cell item class

	float x_size = m_pPickUpItem->GetGridWidth() * (UI().is_widescreen() ? 30.f : 40.f);
	float y_size = m_pPickUpItem->GetGridHeight() * 40.f;
	float x_position = (1024 / 2) - (x_size / 2);
	float y_position = 550;

	uiPickUpItemIconNew_->SetWndSize(x_size, y_size);
	uiPickUpItemIconNew_->SetWndPos(x_position, y_position);
	fuzzyShowInfo_ = 0.f;
};

void CUIMainIngameWnd::UpdateActiveItemInfo()
{
	PIItem	item		= m_pActor->inventory().ActiveItem();
	u32		active_slot	= m_pActor->inventory().GetActiveSlot();
	if(item) 
	{
		item->GetBriefInfo			(m_item_info);

		UIWeaponSignAmmo.Show		(true						);
		UIWeaponFiremode.Show		(true						);
		UIWeaponBack.TextItemControl()->SetText		(m_item_info.name.c_str			()	);
		UIWeaponFiremode.SetText	(item->GetCurrentFireModeStr());
		UIWeaponSignAmmo.TextItemControl()->SetText	(m_item_info.cur_ammo.c_str		()	);

		SetAmmoIcon					(m_item_info.icon.c_str	()	);

		//-------------------
		m_pWeapon = smart_cast<CWeapon*> (item);
		if (active_slot == BOLT_SLOT)
			HandleBolt();		
	}else
	{
		UIWeaponIcon.Show			(false);
		UIWeaponSignAmmo.Show		(false);
		UIWeaponFiremode.Show		(false);
		UIWeaponBack.TextItemControl()->SetText		("");
		m_pWeapon					= NULL;
	}
}

void CUIMainIngameWnd::HandleBolt()
{
	SetAmmoIcon("bolt");
}


void CUIMainIngameWnd::OnConnected()
{
	UIZoneMap->SetupCurrentMap		();
}

void CUIMainIngameWnd::reset_ui()
{
	m_pActor						= NULL;
	m_pWeapon						= NULL;
	m_pGrenade						= NULL;
	m_pItem							= NULL;
	m_pPickUpItem					= NULL;
	UIMotionIcon.ResetVisibility	();

	xr_delete(uiPickUpItemIconNew_);
}