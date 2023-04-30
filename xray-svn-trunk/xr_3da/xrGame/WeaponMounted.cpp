#include "stdafx.h"
#pragma hdrstop

#include "WeaponMounted.h"
#include "xrServer_Objects_ALife.h"
#include "camerafirsteye.h"
#include "actor.h"
#include "weaponammo.h"


#include "actoreffector.h"
#include "effectorshot.h"
#include "ai_sounds.h"
#include "level.h"
#include "xr_level_controller.h"
#include "../Include/xrRender/Kinematics.h"
#include "weaponmiscs.h"

extern int sndMaxShotSounds_;

//----------------------------------------------------------------------------------------

void CWeaponMounted::BoneCallbackX(CBoneInstance *B)
{
	CWeaponMounted	*P = static_cast<CWeaponMounted*>(B->callback_param());

	if (P->Owner()){
		Fmatrix rX;		rX.rotateX		(P->camera->pitch+P->m_dAngle.y);
		B->mTransform.mulB_43(rX);
	}
}

void CWeaponMounted::BoneCallbackY(CBoneInstance *B)
{
	CWeaponMounted	*P = static_cast<CWeaponMounted*>(B->callback_param());

	if (P->Owner()){
		Fmatrix rY;		rY.rotateY		(P->camera->yaw+P->m_dAngle.x);
		B->mTransform.mulB_43(rY);
	}
}
//----------------------------------------------------------------------------------------

CWeaponMounted::CWeaponMounted()
{
	camera		= xr_new <CCameraFirstEye>(this, CCameraBase::flRelativeLink|CCameraBase::flPositionRigid|CCameraBase::flDirectionRigid);
	camera->LoadCfg("mounted_weapon_cam");
}

CWeaponMounted::~CWeaponMounted()
{
	xr_delete(camera);
}

void	CWeaponMounted::LoadCfg(LPCSTR section)
{
	inherited::LoadCfg(section);
	CShootingObject::LoadCfg	(section);

	HUD_SOUND_ITEM::LoadSound(section, "snd_shoot", sndShot, (u8)sndMaxShotSounds_, SOUND_TYPE_WEAPON_SHOOTING);

	//��� ������������ ��������
	m_sAmmoType = pSettings->r_string(section, "ammo_class");
	m_CurrentAmmo.Load(*m_sAmmoType, 0);

	//������������� ������ �� ����� ������
	camMaxAngle			= pSettings->r_float		(section,"cam_max_angle"	); 
	camMaxAngle			= deg2rad					(camMaxAngle);
	camRelaxSpeed		= pSettings->r_float		(section,"cam_relax_speed"	); 
	camRelaxSpeed		= deg2rad					(camRelaxSpeed);

	camRecoil.camMaxAngleVert = camMaxAngle;
	camRecoil.camRelaxSpeed = camRelaxSpeed;
	camRecoil.camMaxAngleHorz = 0.25f;
	camRecoil.camStepAngleHorz = 0.01f;
	camRecoil.camDispersionFrac = 0.7f;
}

BOOL CWeaponMounted::SpawnAndImportSOData(CSE_Abstract* data_containing_so)
{
	CSE_Abstract			*e	= (CSE_Abstract*)(data_containing_so);
	CSE_ALifeMountedWeapon	*mw	= smart_cast<CSE_ALifeMountedWeapon*>(e);
	R_ASSERT				(mw);

	if (!inherited::SpawnAndImportSOData(data_containing_so))
		return			(FALSE);

	R_ASSERT				(Visual() && smart_cast<IKinematics*>(Visual()));

	IKinematics* K			= smart_cast<IKinematics*>(Visual());
	CInifile* pUserData		= K->LL_UserData(); 

	R_ASSERT3				(pUserData,"Empty MountedWeapon user data!",mw->get_visual());

	fire_bone				= K->LL_BoneID	(pUserData->r_string("mounted_weapon_definition","fire_bone"));
	actor_bone				= K->LL_BoneID	(pUserData->r_string("mounted_weapon_definition","actor_bone"));
	rotate_x_bone			= K->LL_BoneID	(pUserData->r_string("mounted_weapon_definition","rotate_x_bone"));
	rotate_y_bone			= K->LL_BoneID	(pUserData->r_string("mounted_weapon_definition","rotate_y_bone"));
	camera_bone				= K->LL_BoneID	(pUserData->r_string("mounted_weapon_definition","camera_bone"));

	CBoneData& bdX			= K->LL_GetData(rotate_x_bone); VERIFY(bdX.IK_data.type==jtJoint);
	camera->lim_pitch.set	(bdX.IK_data.limits[0].limit.x,bdX.IK_data.limits[0].limit.y);
	CBoneData& bdY			= K->LL_GetData(rotate_y_bone); VERIFY(bdY.IK_data.type==jtJoint);
	camera->lim_yaw.set		(bdY.IK_data.limits[1].limit.x,bdY.IK_data.limits[1].limit.y);

	U16Vec fixed_bones;
	fixed_bones.push_back	(K->LL_GetBoneRoot());
	PPhysicsShell()			= P_build_Shell(this,false,fixed_bones);
	K						->CalculateBones_Invalidate();
	K						->CalculateBones(BonesCalcType::force_recalc);

	CShootingObject::Light_Create();

	setVisible	(TRUE);
	setEnabled	(TRUE);



	return TRUE;
}

void	CWeaponMounted::DestroyClientObj()
{
	CShootingObject::Light_Destroy();

	inherited::DestroyClientObj();
	
	xr_delete(m_pPhysicsShell);
}

void	CWeaponMounted::ExportDataToServer(NET_Packet& P)
{
	inherited::ExportDataToServer(P);
}

void CWeaponMounted::UpdateCL()
{
#ifdef MEASURE_UPDATES
	CTimer measure_updatecl; measure_updatecl.Start();
#endif

	
	inherited::UpdateCL	();

	if (Owner())
	{
		IKinematics* K = smart_cast<IKinematics*>(Visual());

		K->CalculateBones();

		// update fire pos & fire_dir
		fire_bone_xform = K->LL_GetTransform(fire_bone);
		fire_bone_xform.mulA_43			(XFORM());

		fire_pos.set(0, 0, 0);

		fire_bone_xform.transform_tiny(fire_pos);

		fire_dir.set(0, 0, 1); 

		fire_bone_xform.transform_dir(fire_dir);

		UpdateFire();

		if(OwnerActor() && OwnerActor()->IsMyCamera()) 
		{
			cam_Update(TimeDelta(), camFov);

			OwnerActor()->Cameras().UpdateFromCamera(Camera());
			OwnerActor()->Cameras().ApplyDevice(VIEWPORT_NEAR);
		}
	}
	
	
#ifdef MEASURE_UPDATES
	Device.Statistic->updateCL_VariousPhysics_ += measure_updatecl.GetElapsed_sec();
#endif
}

void CWeaponMounted::ScheduledUpdate(u32 dt)
{
	inherited::ScheduledUpdate(dt);
}

void	CWeaponMounted::renderable_Render(IRenderBuffer& render_buffer)
{
	//���������� ���������
#pragma todo("MT Warning: better to move this call to main thread. May be into updateCL?")
	RenderLight();

	inherited::renderable_Render	(render_buffer);
}

void	CWeaponMounted::OnMouseMove			(int dx, int dy)
{
	CCameraBase* C	= camera;
	float scale		= (C->f_fov / camFov)*psMouseSens * psMouseSensScale/50.f;
	if (dx){
		float d		= float(dx)*scale;
		C->Move		((d<0)?kLEFT:kRIGHT, _abs(d));
	}
	if (dy){
		float d		= ((psMouseInvert.test(1))?-1:1)*float(dy)*scale*3.f/4.f;
		C->Move		((d>0)?kUP:kDOWN, _abs(d));
	}
}
void	CWeaponMounted::OnKeyboardPress		(int dik)
{
	switch (dik)	
	{
	case kWPN_FIRE:					
		FireStart();
		break;
	};

}
void	CWeaponMounted::OnKeyboardRelease	(int dik)
{
	switch (dik)	
	{
	case kWPN_FIRE:
		FireEnd();
		break;
	};
}
void	CWeaponMounted::OnKeyboardHold		(int dik)
{

}

void	CWeaponMounted::cam_Update			(float dt, float fov)
{
	Fvector							P,Da;
	Da.set							(0,0,0);

	IKinematics* K					= smart_cast<IKinematics*>(Visual());
	K->CalculateBones_Invalidate	();
	K->CalculateBones				();
	const Fmatrix& C				= K->LL_GetTransform(camera_bone);
	XFORM().transform_tiny			(P,C.c);

	if(OwnerActor()){
		// rotate head
		OwnerActor()->Orientation().yaw			= -Camera()->yaw;
		OwnerActor()->Orientation().pitch		= -Camera()->pitch;
	}
	Camera()->Update							(P,Da);
	Level().Cameras().UpdateFromCamera			(Camera());
}

bool	CWeaponMounted::Use					(const Fvector& pos,const Fvector& dir,const Fvector& foot_pos)
{
	return !Owner();
}
bool	CWeaponMounted::attach_Actor		(CGameObject* actor)
{
	m_dAngle.set(0.0f,0.0f);
	CHolderCustom::attach_Actor(actor);
	IKinematics* K		= smart_cast<IKinematics*>(Visual());
	// ������ ������ �� ���	
	// disable shell callback
	m_pPhysicsShell->EnabledCallbacks(FALSE);
	// enable actor rotate callback
	CBoneInstance& biX		= smart_cast<IKinematics*>(Visual())->LL_GetBoneInstance(rotate_x_bone);	
	biX.set_callback		(bctCustom,BoneCallbackX,this);
	CBoneInstance& biY		= smart_cast<IKinematics*>(Visual())->LL_GetBoneInstance(rotate_y_bone);	
	biY.set_callback		(bctCustom,BoneCallbackY,this);
	// set actor to mounted position
	const Fmatrix& A	= K->LL_GetTransform(actor_bone);
	Fvector ap;
	XFORM().transform_tiny	(ap,A.c);
	Fmatrix AP; AP.translate(ap);
	if(OwnerActor()) OwnerActor()->SetPhPosition	(AP);
	processing_activate		();
	return true;
}
void	CWeaponMounted::detach_Actor		()
{
	CHolderCustom::detach_Actor();
	// disable actor rotate callback
	CBoneInstance& biX		= smart_cast<IKinematics*>(Visual())->LL_GetBoneInstance(rotate_x_bone);	
	biX.reset_callback		();
	CBoneInstance& biY		= smart_cast<IKinematics*>(Visual())->LL_GetBoneInstance(rotate_y_bone);	
	biY.reset_callback		();
	// enable shell callback
	m_pPhysicsShell->EnabledCallbacks(TRUE);
	
	//��������� ��������
	FireEnd();

	processing_deactivate		();
}

Fvector	CWeaponMounted::ExitPosition		()
{
	return XFORM().c;
}

CCameraBase*	CWeaponMounted::Camera				()
{
	return camera;
}

#include "pch_script.h"
#include "script_callback_ex.h"
#include "script_game_object.h"

void CWeaponMounted::FireStart()
{
	if (Owner())
		Owner()->callback(GameObject::eActionTypeWeaponFire)(Owner()->lua_game_object(), lua_game_object());

	m_dAngle.set(0.0f,0.0f);
	CShootingObject::FireStart();
}

void CWeaponMounted::FireEnd()
{
	m_dAngle.set(0.0f,0.0f);
	CShootingObject::FireEnd();
	StopFlameParticles	();
	RemoveShotEffector ();
}


void CWeaponMounted::OnShot		()
{
	VERIFY(Owner());

	FireBullet(get_CurrentFirePoint(),fire_dir, 
		fireDispersionBase,
		m_CurrentAmmo, Owner()->ID(),ID(), SendHitAllowed(Owner()));

	StartShotParticles			();

	if(m_bLightShotEnabled) 
		Light_Start			();

	StartFlameParticles();
	StartSmokeParticles(fire_pos, zero_vel);
	OnShellDrop(fire_pos, zero_vel);

	bool b_hud_mode = (Level().CurrentEntity() == smart_cast<CObject*>(Owner()));
	HUD_SOUND_ITEM::PlaySound(sndShot, fire_pos, Owner(), b_hud_mode, false, u8(-1), Random.randF(MIN_RND_FREQ__SHOT, MAX_RND_FREQ__SHOT));

	//�������� �������� ��������
	AddShotEffector		();
	m_dAngle.set(	::Random.randF(-fireDispersionBase,fireDispersionBase),
					::Random.randF(-fireDispersionBase,fireDispersionBase));
}

void CWeaponMounted::UpdateFire()
{
	fShotTimeCounter -= TimeDelta();
	

	CShootingObject::UpdateFlameParticles();
	CShootingObject::UpdateLight();

	if(!IsWorking()){
		if(fShotTimeCounter <0) fShotTimeCounter = 0.f;
		return;
	}

	if(fShotTimeCounter <=0){
		OnShot();
		fShotTimeCounter += fOneShotTime;
	}else{
		angle_lerp		(m_dAngle.x,0.f,5.f,TimeDelta());
		angle_lerp		(m_dAngle.y,0.f,5.f,TimeDelta());
	}
}

const Fmatrix&	 CWeaponMounted::get_ParticlesXFORM	()
{
	return fire_bone_xform;
}

void CWeaponMounted::AddShotEffector				()
{
	if(OwnerActor())
	{
		CCameraShotEffector* S	= smart_cast<CCameraShotEffector*>(OwnerActor()->Cameras().GetCamEffector(eCEShot));

		if (!S)
			S = (CCameraShotEffector*)OwnerActor()->Cameras().AddCamEffector(xr_new <CCameraShotEffector>(camRecoil));

		R_ASSERT(S);
		S->Shot2(0.01f);
	}
}

void  CWeaponMounted::RemoveShotEffector	()
{
	if(OwnerActor())
		OwnerActor()->Cameras().RemoveCamEffector	(eCEShot);
}