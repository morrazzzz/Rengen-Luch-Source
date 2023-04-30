#include "pch_script.h"
#include "actor.h"
#include "ActorFlags.h"
#include "inventory.h"
#include "xrserver_objects_alife_monsters.h"
#include "xrServer.h"
#include "CameraFirstEye.h"
#include "ActorEffector.h"
#include "../../xrphysics/iPHWorld.h"
#include "../../xrphysics/actorcameracollision.h"
#include "level.h"
#include "infoportion.h"
#include "alife_registry_wrappers.h"
#include "../Include/xrRender/Kinematics.h"
#include "client_spawn_manager.h"
#include "CharacterPhysicsSupport.h"
#include "UIGameCustom.h"
#include "actoranimdefs.h"
#include "map_manager.h"
#include "HUDManager.h"
#include "gamepersistent.h"
#include "holder_custom.h"
#include "actor_memory.h"
#include "actor_statistic_mgr.h"
#include "../xr_collide_form.h"
#include "uigamesp.h"
#include "ui/UIInventoryWnd.h"
#include "level.h"
#include "GameTaskManager.h"

#ifdef DEBUG
#	include "debug_renderer.h"
#	include "..\xr_collide_form.h"
#endif

int	g_cl_InterpolationType			= 0;
u32	g_cl_InterpolationMaxPoints		= 0;

CActor* g_actor						= NULL;

CActor*	Actor()	
{	
	VERIFY(g_actor);

	return(g_actor); 
};

void CActor::ExportDataToServer(NET_Packet& P) // export to server
{
	//CSE_ALifeCreatureAbstract
	u8					flags = 0;
	P.w_float			(GetfHealth());
	P.w_u32				(Level().timeServer());
	P.w_u8				(flags);
	Fvector				p = Position();
	P.w_vec3			(p);//Position());

	P.w_float			(angle_normalize(r_model_yaw));
	P.w_float			(angle_normalize(unaffected_r_torso.yaw));
	P.w_float			(angle_normalize(unaffected_r_torso.pitch));
	P.w_float			(angle_normalize(unaffected_r_torso.roll));

	P.w_u8				(u8(g_Team()));
	P.w_u8				(u8(g_Squad()));
	P.w_u8				(u8(g_Group()));

	//CSE_ALifeCreatureActor
	u16 ms	= (u16)(mstate_real & 0x0000ffff);

	P.w_u16				(u16(ms));
	P.w_sdir			(NET_SavedAccel);

	Fvector				v = character_physics_support()->movement()->GetVelocity();
	P.w_sdir			(v);
	P.w_float			(g_Radiation());

	P.w_u8				(u8(inventory().GetActiveSlot()));

	u16 NumItems		= PHGetSyncItemsNumber();

	if (!g_Alive()) NumItems = 0;
	
	P.w_u16				(NumItems);

	if (!NumItems)
		return;

	if (g_Alive())
	{
		SPHNetState	State;

		CPHSynchronize* pSyncObj = NULL;
		pSyncObj = PHGetSyncItem(0);
		pSyncObj->get_State(State);

		P.w_u8					( State.enabled );

		P.w_vec3				( State.angular_vel);
		P.w_vec3				( State.linear_vel);

		P.w_vec3				( State.force);
		P.w_vec3				( State.torque);

		P.w_vec3				( State.position);

		P.w_float				( State.quaternion.x );
		P.w_float				( State.quaternion.y );
		P.w_float				( State.quaternion.z );
		P.w_float				( State.quaternion.w );
	}
};

BOOL CActor::SpawnAndImportSOData(CSE_Abstract* data_containing_so)
{
	m_holder_id				= ALife::_OBJECT_ID(-1);
	m_feel_touch_characters = 0;
	m_snd_noise				= 0.0f;

	m_sndShockEffector = NULL;
	m_ScriptCameraDirection	= NULL;

	if (m_pPhysicsShell)
	{
		m_pPhysicsShell->Deactivate();
		xr_delete(m_pPhysicsShell);
	};

	//force actor to be local on server client
	CSE_Abstract* e	= (CSE_Abstract*)(data_containing_so);
	CSE_ALifeCreatureActor* E = smart_cast<CSE_ALifeCreatureActor*>(e);	

	E->s_flags.set(M_SPAWN_OBJECT_LOCAL, TRUE);
	
	if(	TRUE == E->s_flags.test(M_SPAWN_OBJECT_LOCAL) && TRUE == E->s_flags.is(M_SPAWN_OBJECT_ASPLAYER))
		g_actor = this;

	VERIFY(m_pActorEffector == NULL);

	m_pActorEffector			= xr_new <CActorCameraManager>();

	// motions
	m_bAnimTorsoPlayed			= false;
	m_current_legs_blend		= 0;
	m_current_jump_blend		= 0;
	m_current_legs.invalidate	();
	m_current_torso.invalidate	();
	m_current_head.invalidate	();

	// инициализация реестров, используемых актером
	encyclopedia_registry->registry().init(ID());
	game_news_registry->registry().init(ID());

	if (!CInventoryOwner::SpawnAndImportSOData(data_containing_so))
		return FALSE;

	if (!inherited::SpawnAndImportSOData(data_containing_so))
		return FALSE;

	CSE_ALifeTraderAbstract* pTA = smart_cast<CSE_ALifeTraderAbstract*>(e);

	set_money(pTA->m_dwMoney, false);

	ROS()->force_mode(IRender_ObjectSpecific::TRACE_ALL);

	mstate_wishful = 0;
	mstate_wishful = E->mstate&(mcCrouch | mcAccel);
	mstate_old = mstate_real = mstate_wishful;
	set_state_box(mstate_real);
	m_pPhysics_support->in_NetSpawn(e);

	if(m_bOutBorder)character_physics_support()->movement()->setOutBorder();

	r_torso_tgt_roll		= 0;

	r_model_yaw				= E->o_torso.yaw;
	r_torso.yaw				= E->o_torso.yaw;
	r_torso.pitch			= E->o_torso.pitch;
	r_torso.roll			= 0.0f;

	unaffected_r_torso.yaw	= r_torso.yaw;
	unaffected_r_torso.pitch= r_torso.pitch;
	unaffected_r_torso.roll	= r_torso.roll;

	cam_Set					(eacFirstEye);

	cam_Active()->Set		(-E->o_torso.yaw, E->o_torso.pitch, 0);

	m_bJumpKeyPressed		= FALSE;

	NET_SavedAccel.set		(0, 0, 0);

	setEnabled				(E->s_flags.is(M_SPAWN_OBJECT_LOCAL));

	Engine.Sheduler.Register(this, TRUE);

	m_hit_slowmo				= 0.f;

	OnChangeVisual();
	//----------------------------------
	m_bAllowDeathRemove = false;

	processing_activate();

#ifdef DEBUG
	LastPosS.clear();
	LastPosH.clear();
	LastPosL.clear();
#endif

	SetDefaultVisualOutfit(VisualName());
	ChangeVisual(m_DefaultVisualOutfit_legs);
	m_bFirstEye = true;

	smart_cast<IKinematics*>(Visual())->CalculateBones();

	inventory().SetPrevActiveSlot(NO_ACTIVE_SLOT);

	m_States.empty();

	if (!g_Alive())
	{
		mstate_wishful	&=		~mcAnyMove;
		mstate_real		&=		~mcAnyMove;
		IKinematicsAnimated* K= smart_cast<IKinematicsAnimated*>(Visual());
		K->PlayCycle("death_init");

		//остановить звук тяжелого дыхания
		m_HeavyBreathSnd.stop();
	}
	
	typedef CClientSpawnManager::CALLBACK_TYPE	CALLBACK_TYPE;

	CALLBACK_TYPE	callback;
	callback.bind	(this, &CActor::on_requested_spawn);

	m_holder_id = E->m_holderID;

	if (E->m_holderID != ALife::_OBJECT_ID(-1))
		Level().client_spawn_manager().add_spawn_callback(E->m_holderID, ID(), callback);

	m_iLastHitterID = u16(-1);
	m_iLastHittingWeaponID = u16(-1);
	m_s16LastHittedElement = -1;
	m_bWasHitted = false;

	Level().MapManager().AddMapLocation("actor_location", ID());
	Level().MapManager().AddMapLocation("actor_location_p", ID());

	Level().GameTaskManager().initialize(ID());

	m_statistic_manager = xr_new <CActorStatisticMgr>();

	spatial.s_type |=STYPE_REACTTOSOUND;
	psHUD_Flags.set(HUD_WEAPON_RT,TRUE);
	
	return TRUE;
}

void CActor::DestroyClientObj()
{
	inherited::DestroyClientObj();

	if (m_holder_id != ALife::_OBJECT_ID(-1))
		Level().client_spawn_manager().remove_spawn_callback(m_holder_id, ID());

	delete_data				(m_statistic_manager);
	
	Level().MapManager().OnObjectDestroyNotify(ID());

	CInventoryOwner::DestroyClientObj();

	cam_UnsetLadder();	
	character_physics_support()->movement()->DestroyCharacter();

	if(m_pPhysicsShell)
	{
		m_pPhysicsShell->Deactivate();
		xr_delete<CPhysicsShell>(m_pPhysicsShell);
	};

	m_pPhysics_support->in_NetDestroy();

	xr_delete		(m_sndShockEffector);
	xr_delete		(m_ScriptCameraDirection);
	xr_delete		(m_pActorEffector);

	pCamBobbing		= NULL;
	
#ifdef DEBUG	
	LastPosS.clear();
	LastPosH.clear();
	LastPosL.clear();
#endif

	processing_deactivate();

	m_holder = NULL;
	m_holderID = u16(-1);
	
	SetDefaultVisualOutfit(NULL);
	SetDefaultVisualOutfit_legs(NULL);

	if(g_actor == this)
		g_actor = NULL;

	Engine.Sheduler.Unregister(this);

	if (actor_camera_shell &&
		actor_camera_shell->get_ElementByStoreOrder(0)->PhysicsRefObject()
		==
		this
		)
		destroy_physics_shell(actor_camera_shell);
}

void CActor::RemoveLinksToCLObj(CObject* O)
{
 	VERIFY(O);

	CGameObject* GO = smart_cast<CGameObject*>(O);

	if(GO && m_pObjectWeLookingAt == GO)
	{
		m_pObjectWeLookingAt=NULL;
	}

	CHolderCustom* HC = smart_cast<CHolderCustom*>(GO);

	if (HC && HC == m_pHolderWeLookingAt)
	{
		m_pHolderWeLookingAt = NULL;
	}

	if (HC&&HC == m_holder)
	{
		m_holder->detach_Actor();
		m_holder = NULL;
	}

	inherited::RemoveLinksToCLObj(O);

	memory().remove_links(O);
	m_pPhysics_support->in_NetRelcase(O);

	HUD().RemoveLinksToCLObj(O);
}

BOOL CActor::NeedDataExport()	// relevant for export to server
{ 
	return true;
};

void CActor::SetCallbacks()
{
	IKinematics* V		= smart_cast<IKinematics*>(Visual());

	VERIFY(V);

	u16 spine0_bone		= V->LL_BoneID("bip01_spine");
	u16 spine1_bone		= V->LL_BoneID("bip01_spine1");
	u16 shoulder_bone	= V->LL_BoneID("bip01_spine2");
	u16 head_bone		= V->LL_BoneID("bip01_head");
	V->LL_GetBoneInstance(u16(spine0_bone)).set_callback	(bctCustom,Spin0Callback, this);
	V->LL_GetBoneInstance(u16(spine1_bone)).set_callback	(bctCustom,Spin1Callback, this);
	V->LL_GetBoneInstance(u16(shoulder_bone)).set_callback	(bctCustom,ShoulderCallback, this);
	V->LL_GetBoneInstance(u16(head_bone)).set_callback		(bctCustom,HeadCallback, this);
}
void CActor::ResetCallbacks()
{
	IKinematics* V		= smart_cast<IKinematics*>(Visual());

	VERIFY(V);

	u16 spine0_bone		= V->LL_BoneID("bip01_spine");
	u16 spine1_bone		= V->LL_BoneID("bip01_spine1");
	u16 shoulder_bone	= V->LL_BoneID("bip01_spine2");
	u16 head_bone		= V->LL_BoneID("bip01_head");

	V->LL_GetBoneInstance(u16(spine0_bone)).reset_callback	();
	V->LL_GetBoneInstance(u16(spine1_bone)).reset_callback	();
	V->LL_GetBoneInstance(u16(shoulder_bone)).reset_callback();
	V->LL_GetBoneInstance(u16(head_bone)).reset_callback	();
}

void CActor::OnChangeVisual()
{
	{
		CPhysicsShell* tmp_shell=PPhysicsShell();
		PPhysicsShell()=NULL;
		inherited::OnChangeVisual();
		PPhysicsShell()=tmp_shell;
		tmp_shell=NULL;
	}
	
	IKinematicsAnimated* V	= smart_cast<IKinematicsAnimated*>(Visual());

	if (V)
	{
		CStepManager::reload(SectionName().c_str());

		SetCallbacks();
		m_anims->Create(V);
		m_vehicle_anims->Create(V);

		CDamageManager::reload(*SectionName(),"damage",pSettings);

		m_head				= smart_cast<IKinematics*>(Visual())->LL_BoneID("bip01_head");
		m_eye_left			= smart_cast<IKinematics*>(Visual())->LL_BoneID("eye_left");
		m_eye_right			= smart_cast<IKinematics*>(Visual())->LL_BoneID("eye_right");
		m_r_hand			= smart_cast<IKinematics*>(Visual())->LL_BoneID(pSettings->r_string(*SectionName(), "weapon_bone0"));
		m_l_finger1			= smart_cast<IKinematics*>(Visual())->LL_BoneID(pSettings->r_string(*SectionName(), "weapon_bone1"));
		m_r_finger2			= smart_cast<IKinematics*>(Visual())->LL_BoneID(pSettings->r_string(*SectionName(), "weapon_bone2"));

		m_neck				= smart_cast<IKinematics*>(Visual())->LL_BoneID("bip01_neck");
		m_l_clavicle		= smart_cast<IKinematics*>(Visual())->LL_BoneID("bip01_l_clavicle");
		m_r_clavicle		= smart_cast<IKinematics*>(Visual())->LL_BoneID("bip01_r_clavicle");
		m_spine2			= smart_cast<IKinematics*>(Visual())->LL_BoneID("bip01_spine2");
		m_spine1			= smart_cast<IKinematics*>(Visual())->LL_BoneID("bip01_spine1");
		m_spine				= smart_cast<IKinematics*>(Visual())->LL_BoneID("bip01_spine");

		reattach_items();
		m_pPhysics_support->in_ChangeVisual();
		SetCallbacks();

		m_current_head.invalidate	();
		m_current_legs.invalidate	();
		m_current_torso.invalidate	();
		m_current_legs_blend		= NULL;
		m_current_torso_blend		= NULL;
		m_current_jump_blend		= NULL;
	}
};

void CActor::ChangeVisual(shared_str NewVisual)
{
	if (!NewVisual.size())
		return;

	if (VisualName().size() )
	{
		if (VisualName() == NewVisual)
			return;
	}

	SetVisualName(NewVisual);

	g_SetAnimation(mstate_real);

	Visual()->dcast_PKinematics()->CalculateBones_Invalidate();
	Visual()->dcast_PKinematics()->CalculateBones(BonesCalcType::force_recalc);

	CStepManager::reload(*SectionName());
};

void ACTOR_DEFS::net_update::lerp(ACTOR_DEFS::net_update& A, ACTOR_DEFS::net_update& B, float f)
{
}

void CActor::save(NET_Packet &output_packet)
{
	inherited::save(output_packet);
	CInventoryOwner::save(output_packet);
	output_packet.w_u8(u8(m_bOutBorder));
	output_packet.w_float(float(inventory().GetMaxWeight())); // for skills

	output_packet.w_stringZ(quickUseSlotsContents_[0]);
	output_packet.w_stringZ(quickUseSlotsContents_[1]);
	output_packet.w_stringZ(quickUseSlotsContents_[2]);
	output_packet.w_stringZ(quickUseSlotsContents_[3]);

	// save PNV status
	if (m_nv_handler && m_nv_handler->GetNightVisionStatus())
		output_packet.w_u8(1);
	else
		output_packet.w_u8(0);

	// save crouch state
	if (Actor()->MovingState()&mcCrouch)
		output_packet.w_u8(1);
	else
		output_packet.w_u8(0);

	CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(CurrentGameUI());
	output_packet.w_u32(pGameSP->m_InventoryMenu->GetCategoryFilter());
}

void CActor::load(IReader &input_packet)
{
	inherited::load(input_packet);
	CInventoryOwner::load(input_packet);
	m_bOutBorder=!!(input_packet.r_u8());
	float fRMaxWeight = input_packet.r_float();

	inventory().SetMaxWeight(fRMaxWeight);

	input_packet.r_stringZ(quickUseSlotsContents_[0], sizeof(quickUseSlotsContents_[0]));
	input_packet.r_stringZ(quickUseSlotsContents_[1], sizeof(quickUseSlotsContents_[1]));
	input_packet.r_stringZ(quickUseSlotsContents_[2], sizeof(quickUseSlotsContents_[2]));
	input_packet.r_stringZ(quickUseSlotsContents_[3], sizeof(quickUseSlotsContents_[3]));

	needActivateNV_ = !!input_packet.r_u8();

	needActivateCrouchState_ = !!input_packet.r_u8();

	CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(CurrentGameUI());
	pGameSP->m_InventoryMenu->SetCategoryFilter(input_packet.r_u32());

	GamePersistent().afterGameLoadedStuff_.push_back(fastdelegate::FastDelegate0<>(this, &CActor::AfterLoad));
}

void CActor::net_Save(NET_Packet& P)
{
	inherited::net_Save	(P);
	m_pPhysics_support->in_NetSave(P);
	P.w_u16(m_holderID);
}

BOOL CActor::net_SaveRelevant()
{
	return TRUE;
}

void CActor::AfterLoad()
{
	if (needActivateNV_)
	{
		if (!m_nv_handler)
			m_nv_handler = xr_new<CNightVisionActor>(this);

		m_nv_handler->SwitchNightVision(true, false);
	}

	if (needActivateCrouchState_)
		mstate_wishful |= mcCrouch;
}

void CActor::SetHitInfo(CObject* who, CObject* weapon, s16 element, Fvector Pos, Fvector Dir)
{
	m_iLastHitterID = (who!= NULL) ? who->ID() : u16(-1);
	m_iLastHittingWeaponID = (weapon != NULL) ? weapon->ID() : u16(-1);
	m_s16LastHittedElement = element;
	m_fLastHealth = GetfHealth();
	m_bWasHitted = true;
	m_vLastHitDir = Dir;
	m_vLastHitPos = Pos;
};




#ifdef DEBUG

static void w_vec_q8(NET_Packet& P,const Fvector& vec,const Fvector& min,const Fvector& max)
{
	P.w_float_q8(vec.x,min.x,max.x);
	P.w_float_q8(vec.y,min.y,max.y);
	P.w_float_q8(vec.z,min.z,max.z);
}
static void r_vec_q8(NET_Packet& P,Fvector& vec,const Fvector& min,const Fvector& max)
{
	P.r_float_q8(vec.x,min.x,max.x);
	P.r_float_q8(vec.y,min.y,max.y);
	P.r_float_q8(vec.z,min.z,max.z);

	clamp(vec.x,min.x,max.x);
	clamp(vec.y,min.y,max.y);
	clamp(vec.z,min.z,max.z);
}
static void w_qt_q8(NET_Packet& P,const Fquaternion& q)
{
	P.w_float_q8(q.x,-1.f,1.f);
	P.w_float_q8(q.y,-1.f,1.f);
	P.w_float_q8(q.z,-1.f,1.f);
	P.w_float_q8(q.w,-1.f,1.f);

}
static void r_qt_q8(NET_Packet& P,Fquaternion& q)
{
	P.r_float_q8(q.x,-1.f,1.f);
	P.r_float_q8(q.y,-1.f,1.f);
	P.r_float_q8(q.z,-1.f,1.f);
	P.r_float_q8(q.w,-1.f,1.f);

	clamp(q.x,-1.f,1.f);
	clamp(q.y,-1.f,1.f);
	clamp(q.z,-1.f,1.f);
	clamp(q.w,-1.f,1.f);
}

static void	UpdateLimits (Fvector &p, Fvector& min, Fvector& max)
{
	if(p.x<min.x)min.x=p.x;
	if(p.y<min.y)min.y=p.y;
	if(p.z<min.z)min.z=p.z;

	if(p.x>max.x)max.x=p.x;
	if(p.y>max.y)max.y=p.y;
	if(p.z>max.z)max.z=p.z;

	for (int k=0; k<3; k++)
	{
		if (p[k]<min[k] || p[k]>max[k])
		{
			R_ASSERT2(0, "Fuck");
			UpdateLimits(p, min, max);
		}
	}
};

#define F_MAX         3.402823466e+38F

extern	Flags32	dbg_net_Draw_Flags;
void dbg_draw_piramid (Fvector pos, Fvector dir, float size, float xdir, u32 color)
{
	
	Fvector p0, p1, p2, p3, p4;
	p0.set(size, size, 0.0f);
	p1.set(-size, size, 0.0f);
	p2.set(-size, -size, 0.0f);
	p3.set(size, -size, 0.0f);
	p4.set(0, 0, size*4);
	
	bool Double = false;
	Fmatrix t; t.identity();
	if (_valid(dir) && dir.square_magnitude()>0.01f)
	{		
		t.k.normalize	(dir);
		Fvector::generate_orthonormal_basis(t.k, t.j, t.i);		
	}
	else
	{
		t.rotateY(xdir);		
		Double = true;
	}
	t.c.set(pos);

//	Level().debug_renderer().draw_line(t, p0, p1, color);
//	Level().debug_renderer().draw_line(t, p1, p2, color);
//	Level().debug_renderer().draw_line(t, p2, p3, color);
//	Level().debug_renderer().draw_line(t, p3, p0, color);

//	Level().debug_renderer().draw_line(t, p0, p4, color);
//	Level().debug_renderer().draw_line(t, p1, p4, color);
//	Level().debug_renderer().draw_line(t, p2, p4, color);
//	Level().debug_renderer().draw_line(t, p3, p4, color);
	
	if (!Double)
	{
		DRender->dbg_DrawTRI(t, p0, p1, p4, color);
		DRender->dbg_DrawTRI(t, p1, p2, p4, color);
		DRender->dbg_DrawTRI(t, p2, p3, p4, color);
		DRender->dbg_DrawTRI(t, p3, p0, p4, color);
	}
	else
	{
//		Fmatrix scale;
//		scale.scale(0.8f, 0.8f, 0.8f);
//		t.mulA_44(scale);
//		t.c.set(pos);

		Level().debug_renderer().draw_line(t, p0, p1, color);
		Level().debug_renderer().draw_line(t, p1, p2, color);
		Level().debug_renderer().draw_line(t, p2, p3, color);
		Level().debug_renderer().draw_line(t, p3, p0, color);

		Level().debug_renderer().draw_line(t, p0, p4, color);
		Level().debug_renderer().draw_line(t, p1, p4, color);
		Level().debug_renderer().draw_line(t, p2, p4, color);
		Level().debug_renderer().draw_line(t, p3, p4, color);
	};	
};

#include "level_debug.h"

void	CActor::OnRender_Network()
{
	DRender->OnFrameEnd();

	//-----------------------------------------------------------------------------------------------------
	float size = 0.2f;
	
//	dbg_draw_piramid(Position(), m_PhysicMovementControl->GetVelocity(), size/2, -r_model_yaw, color_rgba(255, 255, 255, 255));
	//-----------------------------------------------------------------------------------------------------
	if (g_Alive())
	{
		if (dbg_net_Draw_Flags.test(dbg_draw_autopickupbox))
		{
			Fvector bc; bc.add(Position(), m_AutoPickUp_AABB_Offset);
			Fvector bd = m_AutoPickUp_AABB;

			Level().debug_renderer().draw_aabb			(bc, bd.x, bd.y, bd.z, color_rgba(0, 255, 0, 255));
		};
		
		IKinematics* V		= smart_cast<IKinematics*>(Visual());
		if (dbg_net_Draw_Flags.test(dbg_draw_actor_alive) && V)
		{
			if (this != Level().CurrentViewEntity() || cam_active != eacFirstEye)
			{
				/*
				u16 BoneCount = V->LL_BoneCount();
				for (u16 i=0; i<BoneCount; i++)
				{
					Fobb BoneOBB = V->LL_GetBox(i);
					Fmatrix BoneMatrix; BoneOBB.xform_get(BoneMatrix);
					Fmatrix BoneMatrixRes; BoneMatrixRes.mul(V->LL_GetTransform(i), BoneMatrix);
					BoneMatrix.mul(XFORM(), BoneMatrixRes);
					Level().debug_renderer().draw_obb(BoneMatrix, BoneOBB.m_halfsize, color_rgba(0, 255, 0, 255));
				};
				*/
				CCF_Skeleton* Skeleton = smart_cast<CCF_Skeleton*>(collidable.model);
				if (Skeleton){
					Skeleton->_dbg_refresh();

					const CCF_Skeleton::ElementVec& Elements = Skeleton->_GetElements();
					for (CCF_Skeleton::ElementVec::const_iterator I=Elements.begin(); I!=Elements.end(); I++){
						if (!I->valid())		continue;
						switch (I->type){
							case SBoneShape::stBox:{
								Fmatrix M;
								M.invert			(I->b_IM);
								Fvector h_size		= I->b_hsize;
								Level().debug_renderer().draw_obb	(M, h_size, color_rgba(0, 255, 0, 255));
							}break;
							case SBoneShape::stCylinder:{
								Fmatrix M;
								M.c.set				(I->c_cylinder.m_center);
								M.k.set				(I->c_cylinder.m_direction);
								Fvector				h_size;
								h_size.set			(I->c_cylinder.m_radius,I->c_cylinder.m_radius,I->c_cylinder.m_height*0.5f);
								Fvector::generate_orthonormal_basis(M.k,M.j,M.i);
								Level().debug_renderer().draw_obb	(M, h_size, color_rgba(0, 127, 255, 255));
							}break;
							case SBoneShape::stSphere:{
								Fmatrix				l_ball;
								l_ball.scale		(I->s_sphere.R, I->s_sphere.R, I->s_sphere.R);
								l_ball.translate_add(I->s_sphere.P);
								Level().debug_renderer().draw_ellipse(l_ball, color_rgba(0, 255, 0, 255));
							}break;
						};
					};					
				}
			};
		};

		if (!(dbg_net_Draw_Flags.is_any((dbg_draw_actor_dead)))) return;
		
		dbg_draw_piramid(Position(), character_physics_support()->movement()->GetVelocity(), size, -r_model_yaw, color_rgba(128, 255, 128, 255));
		dbg_draw_piramid(IStart.Pos, IStart.Vel, size, -IStart.o_model, color_rgba(255, 0, 0, 255));
//		Fvector tmp, tmp1; tmp1.set(0, .1f, 0);
//		dbg_draw_piramid(tmp.add(IStartT.Pos, tmp1), IStartT.Vel, size, -IStartT.o_model, color_rgba(155, 0, 0, 155));
		dbg_draw_piramid(IRec.Pos, IRec.Vel, size, -IRec.o_model, color_rgba(0, 0, 255, 255));
//		dbg_draw_piramid(tmp.add(IRecT.Pos, tmp1), IRecT.Vel, size, -IRecT.o_model, color_rgba(0, 0, 155, 155));
		dbg_draw_piramid(IEnd.Pos, IEnd.Vel, size, -IEnd.o_model, color_rgba(0, 255, 0, 255));
//		dbg_draw_piramid(tmp.add(IEndT.Pos, tmp1), IEndT.Vel, size, -IEndT.o_model, color_rgba(0, 155, 0, 155));
		dbg_draw_piramid(NET_Last.p_pos, NET_Last.p_velocity, size*3/4, -NET_Last.o_model, color_rgba(255, 255, 255, 255));
		
		Fmatrix MS, MH, ML, *pM = NULL;
		ML.translate(0, 0.2f, 0);
		MS.translate(0, 0.2f, 0);
		MH.translate(0, 0.2f, 0);

		Fvector point0S, point1S, point0H, point1H, point0L, point1L, *ppoint0 = NULL, *ppoint1 = NULL;
		Fvector tS, tH;
		u32	cColor = 0, sColor = 0;
		VIS_POSITION*	pLastPos = NULL;

		switch (g_cl_InterpolationType)
		{
		case 0: ppoint0 = &point0L; ppoint1 = &point1L; cColor = color_rgba(0, 255, 0, 255); sColor = color_rgba(128, 255, 128, 255); pM = &ML; pLastPos = &LastPosL; break;
		case 1: ppoint0 = &point0S; ppoint1 = &point1S; cColor = color_rgba(0, 0, 255, 255); sColor = color_rgba(128, 128, 255, 255); pM = &MS; pLastPos = &LastPosS; break;
		case 2: ppoint0 = &point0H; ppoint1 = &point1H; cColor = color_rgba(255, 0, 0, 255); sColor = color_rgba(255, 128, 128, 255); pM = &MH; pLastPos = &LastPosH; break;
		}

		//drawing path trajectory
		float c = 0;
		for (int i=0; i<11; i++)
		{
			c = float(i) * 0.1f;
			for (u32 k=0; k<3; k++)
			{
				point1S[k] = c*(c*(c*SCoeff[k][0]+SCoeff[k][1])+SCoeff[k][2])+SCoeff[k][3];
				point1H[k] = c*(c*(c*HCoeff[k][0]+HCoeff[k][1])+HCoeff[k][2])+HCoeff[k][3];
				point1L[k] = IStart.Pos[k] + c*(IEnd.Pos[k]-IStart.Pos[k]);
			};
			if (i!=0)
			{
				Level().debug_renderer().draw_line(*pM, *ppoint0, *ppoint1, cColor);
			};
			point0S.set(point1S);
			point0H.set(point1H);
			point0L.set(point1L);
		};

		//drawing speed vectors
		for (int i=0; i<2; i++)
		{
			c = float(i);
			for (u32 k=0; k<3; k++)
			{
				point1S[k] = c*(c*(c*SCoeff[k][0]+SCoeff[k][1])+SCoeff[k][2])+SCoeff[k][3];
				point1H[k] = c*(c*(c*HCoeff[k][0]+HCoeff[k][1])+HCoeff[k][2])+HCoeff[k][3];

				tS[k] = (c*c*SCoeff[k][0]*3+c*SCoeff[k][1]*2+SCoeff[k][2])/3; // сокрость из формулы в 3 раза превышает скорость при расчете коэффициентов !!!!
				tH[k] = (c*c*HCoeff[k][0]*3+c*HCoeff[k][1]*2+HCoeff[k][2]); 
			};

			point0S.add(tS, point1S);
			point0H.add(tH, point1H);

			if (g_cl_InterpolationType > 0)
			{
				Level().debug_renderer().draw_line(*pM, *ppoint0, *ppoint1, sColor);
			}
		}

		//draw interpolation history curve
		if (!pLastPos->empty())
		{
			Fvector Pos1, Pos2;
			VIS_POSITION_it It = pLastPos->begin();
			Pos1 = *It;
			for (; It != pLastPos->end(); It++)
			{
				Pos2 = *It;

				Level().debug_renderer().draw_line	(*pM, Pos1, Pos2, cColor);
				Level().debug_renderer().draw_aabb	(Pos2, size/5, size/5, size/5, sColor);
				Pos1 = *It;
			};
		};

		Fvector PH, PS;
		PH.set(IPosH); PH.y += 1;
		PS.set(IPosS); PS.y += 1;
//		Level().debug_renderer().draw_aabb			(PS, size, size, size, color_rgba(128, 128, 255, 255));
//		Level().debug_renderer().draw_aabb			(PH, size, size, size, color_rgba(255, 128, 128, 255));
		/////////////////////////////////////////////////////////////////////////////////
	}
	else
	{
		if (!(dbg_net_Draw_Flags.is_any((dbg_draw_actor_dead)))) return;

		IKinematics* V		= smart_cast<IKinematics*>(Visual());
		if (dbg_net_Draw_Flags.test(dbg_draw_actor_alive) && V)
		{
			u16 BoneCount = V->LL_BoneCount();
			for (u16 i=0; i<BoneCount; i++)
			{
				Fobb BoneOBB = V->LL_GetBox(i);
				Fmatrix BoneMatrix; BoneOBB.xform_get(BoneMatrix);
				Fmatrix BoneMatrixRes; BoneMatrixRes.mul(V->LL_GetTransform(i), BoneMatrix);
				BoneMatrix.mul(XFORM(), BoneMatrixRes);
				Level().debug_renderer().draw_obb(BoneMatrix, BoneOBB.m_halfsize, color_rgba(0, 255, 0, 255));
			};
		};

		if (!m_States.empty())
		{
			u32 NumBones = m_States.size();
			for (u32 i=0; i<NumBones; i++)
			{
				SPHNetState state = m_States[i];			

				Fvector half_dim;
				half_dim.x = 0.2f;
				half_dim.y = 0.1f;
				half_dim.z = 0.1f;

				u32 Color = color_rgba(255, 0, 0, 255);

				Fmatrix M;
				
				M = Fidentity;
				M.rotation(state.quaternion);
				M.translate_add(state.position);
				Level().debug_renderer().draw_obb				(M, half_dim, Color);

				if (!PHGetSyncItem(u16(i))) continue;
				PHGetSyncItem(u16(i))->get_State(state);

				Color = color_rgba(0, 255, 0, 255);
				M = Fidentity;
				M.rotation(state.quaternion);
				M.translate_add(state.position);
				Level().debug_renderer().draw_obb				(M, half_dim, Color);
			};
		}
		else
		{
			if (!g_Alive() && PHGetSyncItemsNumber() > 2)
			{
				u16 NumBones = PHGetSyncItemsNumber();
				for (u16 i=0; i<NumBones; i++)
				{
					SPHNetState state;// = m_States[i];
					PHGetSyncItem(i)->get_State(state);

					Fmatrix M;
					M = Fidentity;
					M.rotation(state.quaternion);
					M.translate_add(state.position);

					Fvector half_dim;
					half_dim.x = 0.2f;
					half_dim.y = 0.1f;
					half_dim.z = 0.1f;

					u32 Color = color_rgba(0, 255, 0, 255);
					Level().debug_renderer().draw_obb				(M, half_dim, Color);
				};
				//-----------------------------------------------------------------
				Fvector min,max;

				min.set(F_MAX,F_MAX,F_MAX);
				max.set(-F_MAX,-F_MAX,-F_MAX);
				/////////////////////////////////////
				for(u16 i=0;i<NumBones;i++)
				{
					SPHNetState state;
					PHGetSyncItem(i)->get_State(state);

					Fvector& p=state.position;
					UpdateLimits (p, min, max);

					Fvector px =state.linear_vel;
					px.div(10.0f);
					px.add(state.position);
					UpdateLimits (px, min, max);
				};

				NET_Packet PX;
				for(u16 i=0;i<NumBones;i++)
				{
					SPHNetState state;
					PHGetSyncItem(i)->get_State(state);

					PX.B.count = 0;
					w_vec_q8(PX,state.position,min,max);
					w_qt_q8(PX,state.quaternion);
//					w_vec_q8(PX,state.linear_vel,min,max);

					PX.r_pos = 0;
					r_vec_q8(PX,state.position,min,max);
					r_qt_q8(PX,state.quaternion);
//					r_vec_q8(PX,state.linear_vel,min,max);
					//===============================================
					Fmatrix M;
					M = Fidentity;
					M.rotation(state.quaternion);
					M.translate_add(state.position);

					Fvector half_dim;
					half_dim.x = 0.2f;
					half_dim.y = 0.1f;
					half_dim.z = 0.1f;

					u32 Color = color_rgba(255, 0, 0, 255);
					Level().debug_renderer().draw_obb				(M, half_dim, Color);
				};	
				Fvector LC, LS;
				LC.add(min, max); LC.div(2.0f);
				LS.sub(max, min); LS.div(2.0f);

				Level().debug_renderer().draw_aabb			(LC, LS.x, LS.y, LS.z, color_rgba(255, 128, 128, 255));
				//-----------------------------------------------------------------
			};
		}
	}
};

#endif
