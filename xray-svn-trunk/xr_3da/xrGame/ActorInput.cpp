#include "stdafx.h"
#include <dinput.h>
#include "pch_script.h"
#include "Actor.h"
#include "Torch.h"
#include "../CameraBase.h"
#ifdef DEBUG
#include "PHDebug.h"
#endif
#include "Car.h"
#include "HudManager.h"
#include "UIGameSP.h"
#include "inventory.h"
#include "level.h"
#include "xr_level_controller.h"
#include "UsableScriptObject.h"
#include "clsid_game.h"
#include "actorcondition.h"
#include "actor_input_handler.h"
#include "string_table.h"
#include "UI/UIStatic.h"
#include "CharacterPhysicsSupport.h"
#include "InventoryBox.h"
#include "WeaponMagazined.h"
#include "game_object_space.h"
#include "script_callback_ex.h"
#include "script_game_object.h"
#include "../xr_input.h"
#include "player_hud.h"

#include "UIGameCustom.h"
#include "UI/UIInventoryWnd.h"
#include "ui\UIDragDropReferenceList.h"
#include "HudItem.h"
#include "weaponmiscs.h"
#include "GameConstants.h"

bool g_bAutoClearCrouch = true;
u32  g_bAutoApplySprint = 0;

extern int hud_adj_mode;
extern Flags32 psHoldZoom;

void CActor::IR_OnKeyboardPress(int cmd)
{
	if (CAttachableItem::m_dbgItem || hud_adj_mode && pInput->iGetAsyncKeyState(DIK_LSHIFT))
		return;

	callback(GameObject::eOnButtonPress)(lua_game_object(), cmd);

	if (IsTalking())
		return;

	if (m_input_external_handler && !m_input_external_handler->authorized(cmd))
		return;

	switch (cmd)
	{
	case kWPN_FIRE:
	{
		BreakSprint();
	}break;
	default:
	{
	}break;
	}

	if (!g_Alive())
		return;

	if (m_holder && kUSE != cmd)
	{
		m_holder->OnKeyboardPress(cmd);

		if (m_holder->allowWeapon() && inventory().InputAction(cmd, CMD_START))
			return;

		return;
	}
	else
	{
		if (cmd == kWPN_ZOOM&&!psHoldZoom.test(1))
		{
			if (inventory().ActiveItem())
			{
				CWeaponMagazined* pWM = smart_cast<CWeaponMagazined*>(inventory().ActiveItem());
				if (pWM)
				{
					if (IsZoomAimingMode())
					{
						if (inventory().InputAction(cmd, CMD_STOP))
							return;
					}
					else
					{
						if (inventory().InputAction(cmd, CMD_START))
							return;
					}
				}
				else
				{
					if (inventory().InputAction(cmd, CMD_START))
						return;
				}
			}
			else
			{
				if (inventory().InputAction(cmd, CMD_START))
					return;
			}
		}
		else
		{
			if (inventory().InputAction(cmd, CMD_START))
				return;
		}
	}

	switch(cmd){
	case kJUMP:		
		{
			if (!inventoryLimitsMoving_)
				mstate_wishful |= mcJump;
		}break;
	case kCROUCH_TOGGLE:
		{
			if (inventoryLimitsMoving_)
				break;

			g_bAutoClearCrouch = !g_bAutoClearCrouch;

			if (!g_bAutoClearCrouch)
				mstate_wishful |= mcCrouch;

		}break;
	case kSPRINT_TOGGLE:	
		{
			if (inventoryLimitsMoving_)
				break;

			CWeapon* W = smart_cast<CWeapon*>(inventory().ActiveItem());

			if (IsReloadingWeapon())
			{
				if (trySprintCounter_ == 0) // don't interrupt reloading on first key press and skip sprint request
				{
					trySprintCounter_++;

					return;
				}
				else if (trySprintCounter_ >= 1) // break reloading, if player insist(presses two or more times) and do sprint
				{
					W->StopAllSounds(DONT_STOP_SOUNDS);
					W->SwitchState(CHUDState::EHudStates::eIdle);
				}
			}

			trySprintCounter_ = 0;

			if (mstate_wishful & mcSprint)
			{
				mstate_wishful &= ~mcSprint;
			}
			else
			{
				g_bAutoClearCrouch = true;
				g_bAutoApplySprint = 1;
				mstate_wishful |= mcSprint;
			}
		}break;

	case kCAM_1:
	{
		cam_Set(eacFirstEye);
		CArtDetectorBase* detectortoshow = inventory().CurrentDetector();

		if (detectortoshow)
			g_player_hud->attach_item(detectortoshow); 
	}break; //Для востановления детектора в руке

	case kCAM_2:	cam_Set(eacLookAt);					break;
	case kCAM_3:	cam_Set(eacFreeLook);				break;
	case kNIGHT_VISION: SwitchNightVision();			break;
	case kALIVE_DETECTOR: SwitchAliveDetector();		break;

	case kTORCH:{ 
		if (!m_current_torch)
		{
			if (inventory().ItemFromSlot(TORCH_SLOT))
			{
				CTorch* torch = smart_cast<CTorch*>(inventory().ItemFromSlot(TORCH_SLOT));

				if (torch)		
				{
					m_current_torch = torch;
					m_current_torch->Switch();
				}
			}
		}
		else 
		{
			if (inventory().ItemFromSlot(TORCH_SLOT))
			{
				CTorch* torch = smart_cast<CTorch*>(inventory().ItemFromSlot(TORCH_SLOT));

				if (torch)
				{
					m_current_torch = torch;
					m_current_torch->Switch();
				}
				else
					m_current_torch = nullptr;

			}
			else
				m_current_torch = nullptr;
		}
		} break;
		
	case kWPN_1:	
	case kWPN_2:	
	case kWPN_3:	
	case kWPN_3b:	
	case kWPN_4:	
	case kWPN_5:	
	case kWPN_6:	
	case kWPN_RELOAD:			
		break;
	/*
	case kFLARE:{
			PIItem fl_active = inventory().ItemFromSlot(FLARE_SLOT);
			if(fl_active)
			{
				CFlare* fl			= smart_cast<CFlare*>(fl_active);
				fl->DropFlare		();
				return				;
			}

			PIItem fli = inventory().Get(CLSID_DEVICE_FLARE, true);
			if(!fli)			return;

			CFlare* fl			= smart_cast<CFlare*>(fli);

			if(inventory().Slot(fl))
				fl->ActivateFlare	();
		}break;
	*/
	case kUSE:
		// drop captured or dragging object
		if (character_physics_support()->movement()->PHCapture())
		{
			character_physics_support()->movement()->PHReleaseObject();

			break;
		}

		// drag or capture object
		if (Level().IR_GetKeyState(DIK_LSHIFT) && (!m_pUsableObject || m_pUsableObject->nonscript_usable()))
		{
			collide::rq_result& RQ = HUD().GetCurrentRayQuery();
			CPhysicsShellHolder* object = smart_cast<CPhysicsShellHolder*>(RQ.O);
			u16 element = BI_NONE;

			if (object)
				element = (u16)RQ.element;

			if (object)
			{
				bool b_allow = !!pSettings->line_exist("ph_capture_visuals", object->VisualName());

				if (m_pPersonWeLookingAt && !m_pPersonWeLookingAt->inventory().CanBeDragged())
					b_allow = false;

				if (b_allow && !character_physics_support()->movement()->PHCapture())
				{
					character_physics_support()->movement()->PHCaptureObject(object, element);

					return;
				}
			}
		}

		break;
	case kDROP:
		b_DropActivated			= TRUE;
		f_DropPower				= 0;
		break;
	case kNEXT_SLOT:
		{
			OnNextWeaponSlot();
		}break;
	case kPREV_SLOT:
		{
			OnPrevWeaponSlot();
		}break;

	case kUSE_BANDAGE:
	case kUSE_MEDKIT:
		{
			PIItem itm = inventory().GetItemByClassID((cmd == kUSE_BANDAGE) ? CLSID_IITEM_BANDAGE : CLSID_IITEM_MEDKIT, inventory().allContainer_);
			if (itm)
			{
				bool used = inventory().Eat(itm);
				if (used)
				{
					SDrawStaticStruct* HudMessage = CurrentGameUI()->AddCustomStatic("inv_hud_message", true);
					HudMessage->m_endTime = EngineTime() + 3.0f; // 3sec

					string1024 str;
					xr_sprintf(str, "%s : %s", *CStringTable().translate("st_item_used"), itm->Name());
					HudMessage->wnd()->TextItemControl()->SetText(str);
				}
			}
		}break;

	case kQUICK_USE_1:
	case kQUICK_USE_2:
	case kQUICK_USE_3:
	case kQUICK_USE_4:
	{
		const shared_str& item_name = quickUseSlotsContents_[cmd - kQUICK_USE_1];
		if (item_name.size())
		{
			PIItem itm = inventory().GetItemBySect(item_name.c_str(), inventory().allContainer_);

			if (itm)
			{
				bool used = inventory().Eat(itm);
				if (used)
				{
					SDrawStaticStruct* HudMessage = CurrentGameUI()->AddCustomStatic("inv_hud_message", true);
					HudMessage->m_endTime = EngineTime() + 3.0f; // 3sec

					string1024 str;
					xr_sprintf(str, "%s : %s", *CStringTable().translate("st_item_used"), itm->Name());
					HudMessage->wnd()->TextItemControl()->SetText(str);


					CurrentGameUI()->m_InventoryMenu->quickUseSlots_->ReloadReferences();
				}
			}
		}
	}break;
	}
}

void CActor::SwitchNightVision()
{
	auto active_wpn = dynamic_cast<CWeapon*>(inventory().ActiveItem());

	if (active_wpn && active_wpn->IsZoomed())
	{
		if (active_wpn->IsScopeNvInstalled())
		{
			active_wpn->SwitchNightVision();
			return;
		}
	}
	else
		SwitchDeviceNightVision(); // no nv device while zooming to stimmulate player for buying scope with nv
}

void CActor::SwitchNightVision(bool val)
{
	auto active_wpn = dynamic_cast<CWeapon*>(inventory().ActiveItem());

	if (active_wpn && active_wpn->IsZoomed())
	{
		if (active_wpn->IsScopeNvInstalled())
		{
			active_wpn->SwitchNightVision(val);

			return;
		}
	}
	else
		SwitchDeviceNightVision(val); // no nv device while zooming to stimmulate player for buying scope with nv
}

void CActor::SwitchDeviceNightVision()
{
	if (!m_nv_handler)
		m_nv_handler = xr_new<CNightVisionActor>(this);

	m_nv_handler->SwitchNightVision();
}

void CActor::SwitchDeviceNightVision(bool val)
{
	if (!m_nv_handler)
		m_nv_handler = xr_new<CNightVisionActor>(this);

	m_nv_handler->SwitchNightVision(val);
}

bool CActor::GetActorAliveDetectorState() const
{ 
	auto helmet = dynamic_cast<COutfitBase*>(inventory().ItemFromSlot(HELMET_SLOT));
	auto outfit = dynamic_cast<COutfitBase*>(GetCurrentOutfit());

	return (outfit && outfit->GetAliveDetectorState()) ||
		   (helmet && helmet->GetAliveDetectorState());
}

void CActor::SwitchAliveDetector()
{
	auto active_wpn = dynamic_cast<CWeapon*>(inventory().ActiveItem());

	if (active_wpn && active_wpn->IsZoomed())
	{
		if (active_wpn->IsScopeAliveDetectorInstalled())
		{
			active_wpn->SwitchAliveDetector();

			return;
		}
	}

	SwitchActorAliveDetector();
}

void CActor::SwitchActorAliveDetector()
{
	auto helmet = dynamic_cast<COutfitBase*>(inventory().ItemFromSlot(HELMET_SLOT));
	auto outfit = dynamic_cast<COutfitBase*>(GetCurrentOutfit());

	if (helmet)
		(m_alive_detector_device = helmet)->SwitchAliveDetector();
	else
	if (outfit)
		(m_alive_detector_device = outfit)->SwitchAliveDetector();

}

void CActor::SwitchActorAliveDetector(bool state)
{
	auto helmet = dynamic_cast<COutfitBase*>(inventory().ItemFromSlot(HELMET_SLOT));
	auto outfit = dynamic_cast<COutfitBase*>(GetCurrentOutfit());

	if (helmet)
		(m_alive_detector_device = helmet)->SwitchAliveDetector(state);
	else
		if (outfit)
			(m_alive_detector_device = outfit)->SwitchAliveDetector(state);

}

void CActor::IR_OnMouseWheel(int direction)
{
	if (hud_adj_mode && g_player_hud)
	{
		g_player_hud->tune(Ivector().set(0, 0, direction));
		return;
	}

	if (CAttachableItem::m_dbgItem)
	{
		return;
	}

	if (inventory().InputAction((direction > 0) ? kWPN_ZOOM_DEC : kWPN_ZOOM_INC, CMD_START))
		return;


	if (direction > 0)
	{
		if (eacLookAt == cam_active)
			for (int i = 0; i < 10; ++i)
				cam_Active()->Move(kCAM_ZOOM_IN);
		else
			OnNextWeaponSlot();
	}
	else
	{
		if (eacLookAt == cam_active)
			for (int i = 0; i < 10; ++i)
				cam_Active()->Move(kCAM_ZOOM_OUT);
		else
			OnPrevWeaponSlot();
	}
}

void CActor::IR_OnKeyboardRelease(int cmd)
{
	if (CAttachableItem::m_dbgItem || hud_adj_mode && pInput->iGetAsyncKeyState(DIK_LSHIFT))
		return;

	callback(GameObject::eOnButtonRelease)(lua_game_object(), cmd);

	if (m_input_external_handler && !m_input_external_handler->authorized(cmd))	return;

	if (g_Alive())
	{
		if (cmd == kUSE)
		{
			needUseKeyRelease = false;
			timeUseAccum = 0.f;
			pickUpLongInProgress = false;
		}

		if (m_holder)
		{
			m_holder->OnKeyboardRelease(cmd);

			if (m_holder->allowWeapon() && inventory().InputAction(cmd, CMD_STOP))
				return;

			return;
		}
		else{
			if (cmd == kWPN_ZOOM&&!psHoldZoom.test(1))
			{
				if (inventory().ActiveItem())
				{
					CWeaponMagazined* pWM = smart_cast<CWeaponMagazined*>(inventory().ActiveItem());

					if (!pWM || (pWM&&!pWM->IsZoomEnabled()))
					{
						if (inventory().InputAction(cmd, CMD_STOP))
							return;
					}
				}
				else
				{
					if (inventory().InputAction(cmd, CMD_STOP))
						return;
				}
			}
			else
			{
				if (inventory().InputAction(cmd, CMD_STOP))
					return;
			}
		}

		switch(cmd)
		{
		case kJUMP:		mstate_wishful &=~mcJump;		break;
		case kDROP:		g_PerformDrop();				break;

		case kCROUCH:
			if (!inventoryLimitsMoving_)
				g_bAutoClearCrouch = true;
			break;
		}
	}
}

void CActor::IR_OnKeyboardHold(int cmd)
{
	if (CAttachableItem::m_dbgItem)
		return;

	if (hud_adj_mode && pInput->iGetAsyncKeyState(DIK_LSHIFT) && g_player_hud)
	{
		bool bIsRot = (hud_adj_mode == 2 || hud_adj_mode == 4);

		if (pInput->iGetAsyncKeyState(bIsRot ? DIK_LEFT  : DIK_UP))
			g_player_hud->tune(Ivector().set(0, -1, 0));
		if (pInput->iGetAsyncKeyState(bIsRot ? DIK_RIGHT : DIK_DOWN))
			g_player_hud->tune(Ivector().set(0, 1, 0));
		if (pInput->iGetAsyncKeyState(bIsRot ? DIK_UP    : DIK_LEFT))
			g_player_hud->tune(Ivector().set(-1, 0, 0));
		if (pInput->iGetAsyncKeyState(bIsRot ? DIK_DOWN  : DIK_RIGHT))
			g_player_hud->tune(Ivector().set(1, 0, 0));
		if (pInput->iGetAsyncKeyState(DIK_PRIOR))
			g_player_hud->tune(Ivector().set(0, 0, 1));
		if (pInput->iGetAsyncKeyState(DIK_NEXT))
			g_player_hud->tune(Ivector().set(0, 0, -1));
		if (pInput->iGetAsyncKeyState(DIK_DELETE))
			g_player_hud->tune(Ivector().set(0, 0, 0));
		return;
	}

	if (!g_Alive())
		return;

	callback(GameObject::eOnButtonHold)(lua_game_object(), cmd);

	if (m_input_external_handler && !m_input_external_handler->authorized(cmd))
		return;

	if (IsTalking())
		return;

	if(m_holder)
	{
		m_holder->OnKeyboardHold(cmd);
		return;
	}

	float LookFactor = GetLookFactor();
	switch (cmd)
	{
	case kUP:
	case kDOWN:
		cam_Active()->Move((cmd == kUP) ? kDOWN : kUP, 0, LookFactor);			break;
	case kCAM_ZOOM_IN:
	case kCAM_ZOOM_OUT:
		cam_Active()->Move(cmd);												break;
	case kLEFT:
	case kRIGHT:
		if (eacFreeLook != cam_active) cam_Active()->Move(cmd, 0, LookFactor);	break;

	case kACCEL:	mstate_wishful |= mcAccel;									break;
	case kL_STRAFE:
	{
		mstate_wishful |= mcLStrafe;

		if (inventoryLimitsMoving_)
			mstate_wishful |= mcAccel;
	}break;
	case kR_STRAFE:
	{
		mstate_wishful |= mcRStrafe;

		if (inventoryLimitsMoving_)
			mstate_wishful |= mcAccel;
	}break;
	case kL_LOOKOUT:mstate_wishful |= mcLLookout;								break;
	case kR_LOOKOUT:mstate_wishful |= mcRLookout;								break;
	case kFWD:
	{
		mstate_wishful |= mcFwd;

		if (inventoryLimitsMoving_)
			mstate_wishful |= mcAccel;
	}break;
	case kBACK:
	{
		mstate_wishful |= mcBack;

		if (inventoryLimitsMoving_)
			mstate_wishful |= mcAccel;
	}break;
	case kCROUCH:	mstate_wishful |= mcCrouch;									break;
	case kUSE:
	{
		if (character_physics_support()->movement()->PHCapture())
			break;

		NearItemsUpdate(); // Draw pickable items names

		ActorPrepareUse();

	}break;
	}
}

void CActor::IR_OnMouseMove(int dx, int dy)
{
	if (hud_adj_mode && g_player_hud)
	{
		g_player_hud->tune(Ivector().set(dx, dy, 0));
		return;
	}

	if (CAttachableItem::m_dbgItem)
	{
		return;
	}

	PIItem iitem = inventory().ActiveItem();
	if (iitem && iitem->cast_hud_item())
		iitem->cast_hud_item()->ResetSubStateTime();

	if (m_holder)
	{
		m_holder->OnMouseMove(dx, dy);

		return;
	}

	float LookFactor = GetLookFactor();

	CCameraBase* C = cameras[cam_active];
	float scale = (C->f_fov / camFov)*psMouseSens * psMouseSensScale / 50.f / LookFactor;

	if (dx)
	{
		float d = float(dx)*scale;
		cam_Active()->Move((d<0) ? kLEFT : kRIGHT, _abs(d));
	}
	if (dy)
	{
		float d = ((psMouseInvert.test(1)) ? -1 : 1)*float(dy)*scale*3.f / 4.f;
		cam_Active()->Move((d>0) ? kUP : kDOWN, _abs(d));
	}
}

bool CActor::use_Holder(CHolderCustom* holder)
{
	if(m_holder)
	{
		bool b = false;
		CGameObject* holderGO = smart_cast<CGameObject*>(m_holder);
		
		if(smart_cast<CCar*>(holderGO))
			b = use_Vehicle(0);
		else
			if (holderGO->CLS_ID==CLSID_OBJECT_W_MOUNTED ||
				holderGO->CLS_ID==CLSID_OBJECT_W_STATMGUN ||
				holderGO->CLS_ID==CLSID_OBJECT_W_TURRET)
					b = use_MountedWeapon(0);

		return b;
	}
	else
	{
		bool b = false;

		CGameObject* holderGO = smart_cast<CGameObject*>(holder);

		if (smart_cast<CCar*>(holder))
			b = use_Vehicle(holder);

		if (holderGO->CLS_ID==CLSID_OBJECT_W_MOUNTED ||
			holderGO->CLS_ID==CLSID_OBJECT_W_STATMGUN ||
			holderGO->CLS_ID==CLSID_OBJECT_W_TURRET)
				b = use_MountedWeapon(holder);
		
		if (b)//used succesfully
		{
			// switch off torch...
			CAttachableItem* I = CAttachmentOwner::attachedItem(CLSID_DEVICE_TORCH);
			if (I)
			{
				CTorch* torch = smart_cast<CTorch*>(I);

				if (torch)
					torch->Switch(false);
			}
		}

		return b;
	}
}

void CActor::ActorPrepareUse()
{
	currentUsable = GetUseObject();

	if (currentUsable.gameObject != usageTarget) // if usage target changed - rewind
		timeUseAccum = 0.f;

	if (!needUseKeyRelease && currentUsable.gameObject)
	{
		timeUseAccum += 1.f * TimeDelta();

		if (timeUseAccum > currentUsable.useDelay)
		{
			timeUseAccum = 0.f;
			needUseKeyRelease = true;

			ActorUse();
		}
	}

	usageTarget = currentUsable.gameObject;
}

void CActor::ActorUse()
{
	if (m_holder)
	{
		use_Holder(NULL);

		return;
	}

	if (m_pUsableObject)
	{
		m_pUsableObject->use(this);
	}

	if (!m_pUsableObject || m_pUsableObject->nonscript_usable())
	{
		if (m_pInvBoxWeLookingAt)
		{
			CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(CurrentGameUI());

			if (pGameSP)
			{
				if (!m_pInvBoxWeLookingAt->closed())
				{
					if (m_pInvBoxWeLookingAt->IsSafe)
					{
						luabind::functor<void> lua_func;
						R_ASSERT2(ai().script_engine().functor("ui_lock.start_lock_ui", lua_func), "Can't find ui_lock.start_lock_ui");
						lua_func(m_pInvBoxWeLookingAt->SafeCode, m_pInvBoxWeLookingAt->lua_game_object());

						pGameSP->StoredInvBox = m_pInvBoxWeLookingAt;
					}
					else
					{
						pGameSP->StartStashUI(this, m_pInvBoxWeLookingAt);
					}
				}
			}

			return;
		}

		// подбирание предмета
		if (inventory().m_pTarget && inventory().m_pTarget->Useful() && !pickUpLongInProgress && !Level().m_feel_deny.is_object_denied(smart_cast<CGameObject*>(inventory().m_pTarget)))
		{
			NET_Packet P;
			u_EventGen(P, GE_OWNERSHIP_TAKE, ID());
			P.w_u16(inventory().m_pTarget->object().ID());
			u_EventSend(P);

			if (pickUpItems.size() > 1)
			{
				timeUseAccum = 0.f;
				needUseKeyRelease = false;

				pickUpLongInProgress = true;
			}

			return;
		}

		// подбирание группы предметов при долгом зажатии
		if (pickUpLongInProgress && pickUpItems.size())
		{
			for (u32 i = 0; i < pickUpItems.size(); ++i)
			{
				NET_Packet P;
				u_EventGen(P, GE_OWNERSHIP_TAKE, ID());
				P.w_u16(pickUpItems[i]->object().ID());
				u_EventSend(P);
			}

			pickUpLongInProgress = false;

			return;
		}

		if (m_pPersonWeLookingAt)
		{
			CEntityAlive* pEntityAliveWeLookingAt =	smart_cast<CEntityAlive*>(m_pPersonWeLookingAt);

			if (pEntityAliveWeLookingAt)
			{
				if (pEntityAliveWeLookingAt->g_Alive()) // talk
				{
					TryToTalk();

					return;
				}
				else if (!Level().IR_GetKeyState(DIK_LSHIFT) && !m_pPersonWeLookingAt->deadbody_closed_status()) //обыск трупа
				{
					CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(CurrentGameUI());

					if (pGameSP)
					{
						luabind::functor<bool> lua_bool;

						R_ASSERT2(ai().script_engine().functor("bind_monster.is_no_monster_inv", lua_bool), "Can't find bind_monster.is_no_monster_inv");

						bool do_no_inv_looting = lua_bool(m_pPersonWeLookingAt->cast_game_object()->lua_game_object());

						if (do_no_inv_looting && m_pPersonWeLookingAt->IsNoInvWnd()) // если у сущности нет инвентаря и геймдизайнер так хочет
						{
							luabind::functor<void> lua_function;

							R_ASSERT2(ai().script_engine().functor<void>("bind_monster.loot_no_inventory", lua_function), make_string("Can't find function bind_monster.loot_no_inventory"));

							lua_function(m_pPersonWeLookingAt->cast_game_object()->lua_game_object());

							return;
						}
						else
						{
							pGameSP->StartStashUI(this, m_pPersonWeLookingAt);

							return;
						}
					}
				}
			}
		}

		collide::rq_result& RQ = HUD().GetCurrentRayQuery();
		CPhysicsShellHolder* object = smart_cast<CPhysicsShellHolder*>(RQ.O);
		u16 element = BI_NONE;

		if (object)
			element = (u16)RQ.element;

		if (object && smart_cast<CHolderCustom*>(object))
		{
			use_Holder(smart_cast<CHolderCustom*>(object));

			return;
		}
	}
}

UsableObject CActor::GetUseObject()
{
	UsableObject usable;

	if (!m_pUsableObject || m_pUsableObject->nonscript_usable())
	{
		if (inventory().m_pTarget && inventory().m_pTarget->Useful() && !pickUpLongInProgress && !Level().m_feel_deny.is_object_denied(smart_cast<CGameObject*>(inventory().m_pTarget)))
		{
			// item
			usable.gameObject = inventory().m_pTarget->cast_game_object();
			usable.useDelay = GameConstants::GetUseTimePickUp();
		}
		else if (pickUpLongInProgress && pickUpItems.size())
		{
			// item group
			usable.gameObject = Actor(); // just dull
			usable.useDelay = GameConstants::GetUseTimePickUpLong();
		}
		else if (m_pInvBoxWeLookingAt) // stash search
		{
			usable.gameObject = m_pObjectWeLookingAt;
			usable.useDelay = GameConstants::GetUseTimeStash();
		}
		else if (m_pPersonWeLookingAt)
		{
			CEntityAlive* entity_we_look_at = smart_cast<CEntityAlive*>(m_pPersonWeLookingAt);

			if (entity_we_look_at && entity_we_look_at->g_Alive()) // talk
			{
				usable.gameObject = m_pObjectWeLookingAt;
				usable.useDelay = GameConstants::GetUseTimeTalk();
			}
			else if (!Level().IR_GetKeyState(DIK_LSHIFT)) // body search
			{
				usable.gameObject = m_pObjectWeLookingAt;
				usable.useDelay = GameConstants::GetUseTimeBodySearch();
			}
		}
		else
		{
			collide::rq_result& RQ = HUD().GetCurrentRayQuery();
			CPhysicsShellHolder* object = smart_cast<CPhysicsShellHolder*>(RQ.O);
			u16 element = BI_NONE;

			if (object)
				element = (u16)RQ.element;

			if (object && smart_cast<CHolderCustom*>(object)) // turrets and other holders
			{
				usable.gameObject = object->cast_game_object();
				usable.useDelay = GameConstants::GetUseTimeHolder();
			}
		}
	}
	else if (m_pUsableObject) // doors, handles, etc..
	{
		usable.gameObject = m_pObjectWeLookingAt;
		usable.useDelay = GameConstants::GetUseTimeScriptUsable();
	}

	return usable;
}

BOOL CActor::HUDview()const
{
	return IsFocused() && (cam_active == eacFirstEye || (cam_active == eacLookAt && m_holder && m_holder->HUDView()));
}

static	u32 SlotsToCheck [] = {
		KNIFE_SLOT		,		// 0
		PISTOL_SLOT		,		// 1
		RIFLE_SLOT		,		// 2
		RIFLE_2_SLOT	,		// 14
		GRENADE_SLOT	,		// 3
		APPARATUS_SLOT	,		// 4
		BOLT_SLOT		,		// 5
		ARTEFACT_SLOT	,		// 10
};

void CActor::OnNextWeaponSlot()
{
	u32 ActiveSlot = inventory().GetActiveSlot();

	if (ActiveSlot == NO_ACTIVE_SLOT)
		ActiveSlot = inventory().GetPrevActiveSlot();

	if (ActiveSlot == NO_ACTIVE_SLOT)
		ActiveSlot = KNIFE_SLOT;

	u32 NumSlotsToCheck = sizeof(SlotsToCheck) / sizeof(u32);

	u32 CurSlot = 0;
	for (; CurSlot<NumSlotsToCheck; CurSlot++)
	{
		if (SlotsToCheck[CurSlot] == ActiveSlot) break;
	};

	if (CurSlot >= NumSlotsToCheck)
		return;

	for (u32 i = CurSlot + 1; i<NumSlotsToCheck; i++)
	{
		auto slotItm = inventory().ItemFromSlot((TSlotId)SlotsToCheck[i]);

		if (slotItm)
		{
			if (SlotsToCheck[i] == ARTEFACT_SLOT)
			{
				IR_OnKeyboardPress(kARTEFACT);
			}
			else
				IR_OnKeyboardPress(kWPN_1 + i);

			return;
		}
	}
};

void CActor::OnPrevWeaponSlot()
{
	u32 ActiveSlot = inventory().GetActiveSlot();

	if (ActiveSlot == NO_ACTIVE_SLOT)
		ActiveSlot = inventory().GetPrevActiveSlot();

	if (ActiveSlot == NO_ACTIVE_SLOT)
		ActiveSlot = KNIFE_SLOT;

	u32 NumSlotsToCheck = sizeof(SlotsToCheck) / sizeof(u32);

	u32 CurSlot = 0;
	for (; CurSlot<NumSlotsToCheck; CurSlot++)
	{
		if (SlotsToCheck[CurSlot] == ActiveSlot) break;
	};

	if (CurSlot >= NumSlotsToCheck)
		return;

	for (s32 i = s32(CurSlot - 1); i >= 0; i--)
	{
		if (inventory().ItemFromSlot((TSlotId)SlotsToCheck[i]))
		{
			if (SlotsToCheck[i] == ARTEFACT_SLOT)
			{
				IR_OnKeyboardPress(kARTEFACT);
			}
			else
				IR_OnKeyboardPress(kWPN_1 + i);

			return;
		}
	}
}

float CActor::GetLookFactor()
{
	if (m_input_external_handler)
		return m_input_external_handler->mouse_scale_factor();

	float factor = 1.f;

	PIItem pItem = inventory().ActiveItem();

	if (pItem)
		factor *= pItem->GetControlInertionFactor();

	VERIFY(!fis_zero(factor));

	return factor;
}

void CActor::set_input_external_handler(CActorInputHandler *handler)
{
	// clear state
	if (handler)
		mstate_wishful = 0;

	// release fire button
	if (handler)
		IR_OnKeyboardRelease(kWPN_FIRE);

	// set handler
	m_input_external_handler = handler;
}

void CActor::g_PerformDrop()
{
	b_DropActivated = FALSE;

	PIItem pItem = inventory().ActiveItem();

	if (!pItem)
		return;

	if (pItem->IsQuestItem())
		return;

	u32 s = inventory().GetActiveSlot();
	if (inventory().m_slots[s].m_bPersistent)	return;

	pItem->SetDropManual(TRUE);
}

CPHDestroyable*	CActor::ph_destroyable()
{
	return smart_cast<CPHDestroyable*>(character_physics_support());
}


