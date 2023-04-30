//----------------------------------------------------
// file: CustomObject.cpp
//----------------------------------------------------

#include "stdafx.h"
#pragma hdrstop

#include "customobject.h"
#include "../ECore/Editor/ui_main.h"
#include "UI_LevelMain.h"
#include "../../xr_3da/xrRender/D3DUtils.h"
#include "motion.h"
#include "ESceneCustomOTools.h"
#include "Scene.h"

#define CUSTOMOBJECT_CHUNK_PARAMS 		0xF900
#define CUSTOMOBJECT_CHUNK_LOCK	 		0xF902
#define CUSTOMOBJECT_CHUNK_TRANSFORM	0xF903
#define CUSTOMOBJECT_CHUNK_GROUP		0xF904
#define CUSTOMOBJECT_CHUNK_MOTION		0xF905
#define CUSTOMOBJECT_CHUNK_FLAGS		0xF906
#define CUSTOMOBJECT_CHUNK_NAME			0xF907
#define CUSTOMOBJECT_CHUNK_MOTION_PARAM	0xF908
//----------------------------------------------------

CCustomObject::CCustomObject(LPVOID data, LPCSTR name)
{
	save_id				= 0;
    ClassID 			= OBJCLASS_DUMMY;
    ParentTools			= 0;
    if (name) 	FName 	= name;
    m_CO_Flags.assign	(0);
    m_RT_Flags.assign	(flRT_Valid|flRT_Visible);
    m_pOwnerObject		= 0;
    ResetTransform		();
    m_RT_Flags.set		(flRT_UpdateTransform,TRUE);
    m_Motion			= NULL;
    m_MotionParams 		= NULL;

    FPosition.set		(0,0,0);
    FScale.set			(1,1,1);
    FRotation.set		(0,0,0);

	AxisManager::SetOwningObject(this);

	Scene->totalCObjectsInstances_ += 1; // Add 1 to scene custom objects total
}

CCustomObject::CCustomObject(CCustomObject* source)
{
    ClassID 	= source->ClassID;
    Name		= source->Name;
    m_CO_Flags.assign	(0);
    m_RT_Flags.assign	(flRT_Valid|flRT_Visible);
    m_pOwnerObject	= 0;
    m_Motion		= NULL;
    m_MotionParams 	= NULL;

	Scene->totalCObjectsInstances_ += 1; // Add 1 to scene custom objects total
}
CCustomObject::~CCustomObject()
{
	xr_delete				(m_Motion);
    xr_delete				(m_MotionParams);

	Scene->totalCObjectsInstances_ -= 1; // Subs 1 from scene custom objects total
	VERIFY2(Scene->totalCObjectsInstances_ >= 0, make_string("totalCObjectsInstances_ = %i", Scene->totalCObjectsInstances_));
}

void CCustomObject::OnUpdateTransform()
{
	m_RT_Flags.set			(flRT_UpdateTransform,FALSE);
    // update transform matrix
	FTransformR.setXYZi		(-PRotation.x, -PRotation.y, -PRotation.z);

	FTransformS.scale		(PScale);
	FTransformP.translate	(PPosition);
	FTransformRP.mul		(FTransformP,FTransformR);
	FTransform.mul			(FTransformRP,FTransformS);
    FITransformRP.invert	(FTransformRP);
    FITransform.invert		(FTransform);

    if (Motionable()&&Visible()&&Selected()&&m_CO_Flags.is(flAutoKey)) AnimationCreateKey(m_MotionParams->Frame());
}

void CCustomObject::Select( int flag )
{
    if (m_RT_Flags.is(flRT_Visible)&&(!!m_CO_Flags.is(flSelected)!=flag))
	{ 
        m_CO_Flags.set		(flSelected,(flag==-1)?(m_CO_Flags.is(flSelected)?FALSE:TRUE):flag);
        UI->RedrawScene		();
        ExecCommand			(COMMAND_UPDATE_PROPERTIES);
	    ParentTools->OnSelected(this);
    }
}

void CCustomObject::Show( BOOL flag )
{
	m_RT_Flags.set	   	(flRT_Visible,flag);

    if (!m_RT_Flags.is(flRT_Visible)) 
    	m_CO_Flags.set(flSelected, FALSE);
        
    UI->RedrawScene();
};

void CCustomObject::Lock( BOOL flag )
{
	m_CO_Flags.set(flCO_Locked,flag);
}

BOOL   CCustomObject::Editable() const 
{
	BOOL b1 = m_CO_Flags.is(flObjectInGroup);
    BOOL b2 = m_CO_Flags.is(flObjectInGroupUnique);
	return !b1 || (b1&&b2);
}

bool CCustomObject::Load(IReader& F)
{
    R_ASSERT(F.find_chunk(CUSTOMOBJECT_CHUNK_FLAGS));
    {
        m_CO_Flags.assign(F.r_u32());
    	
        R_ASSERT(F.find_chunk(CUSTOMOBJECT_CHUNK_NAME));
        F.r_stringZ		(FName);
    }

	if(F.find_chunk(CUSTOMOBJECT_CHUNK_PARAMS))
    {	
        m_RT_Flags.assign(F.r_u32());
    }
    
	if(F.find_chunk(CUSTOMOBJECT_CHUNK_TRANSFORM))
    {
        F.r_fvector3(FPosition);
        F.r_fvector3(FRotation);
    VERIFY(_valid(FRotation));
        F.r_fvector3(FScale);
    }

    // object motion
    if (F.find_chunk(CUSTOMOBJECT_CHUNK_MOTION))
    {
        m_Motion 	= xr_new<COMotion>();
        if (!m_Motion->Load(F)){
            ELog.Msg(mtError,"CustomObject: '%s' - motion has different version. Load failed.",Name);
            xr_delete(m_Motion);
        }
        m_MotionParams = xr_new<SAnimParams>();
	    m_MotionParams->Set(m_Motion);
        AnimationUpdate(m_MotionParams->Frame(), true);
    }

    if (F.find_chunk(CUSTOMOBJECT_CHUNK_MOTION_PARAM)){
    	m_MotionParams->t_current = F.r_float();
        AnimationUpdate(m_MotionParams->Frame(), true);
    }

//	UpdateTransform	(true); // ����� ��� ��������, ����� ������������ ����
	UpdateTransform	();
//	m_bUpdateTransform = TRUE;

	return true;
}

void CCustomObject::Save(IWriter& F)
{
	F.open_chunk	(CUSTOMOBJECT_CHUNK_FLAGS);
	F.w_u32			(m_CO_Flags.get());
	F.close_chunk	();

	F.open_chunk	(CUSTOMOBJECT_CHUNK_NAME);
	F.w_stringZ		(FName);
	F.close_chunk	();
	
	F.open_chunk	(CUSTOMOBJECT_CHUNK_PARAMS);
	F.w_u32			(m_RT_Flags.get());
	F.close_chunk	();

	F.open_chunk	(CUSTOMOBJECT_CHUNK_TRANSFORM);
    F.w_fvector3 	(FPosition);
    F.w_fvector3 	(FRotation);
    F.w_fvector3 	(FScale);
	F.close_chunk	();

    // object motion
    if (m_CO_Flags.is(flMotion)){
    	VERIFY			(m_Motion);
		F.open_chunk	(CUSTOMOBJECT_CHUNK_MOTION);
		m_Motion->Save	(F);
		F.close_chunk	();

        F.open_chunk	(CUSTOMOBJECT_CHUNK_MOTION_PARAM);
        F.w_float		(m_MotionParams->t_current);
        F.close_chunk	();
    }
}
//----------------------------------------------------

void CCustomObject::OnFrame()
{
	if (m_Motion)
		AnimationOnFrame();

	if (m_RT_Flags.is(flRT_UpdateTransform))
		OnUpdateTransform();

	if(m_CO_Flags.test(flObjectInGroup) && m_pOwnerObject==NULL)
		m_CO_Flags.set(flObjectInGroup, FALSE);

	AxisManager::MovementAxisUpdate();
}

bool CCustomObject::IsRender()
{
	Fbox bb;
	GetBox(bb);

	bool res = ::Render->occ_visible(bb) || (Selected() && m_CO_Flags.is_any(flRenderAnyWayIfSelected | flMotion));

	return res;
}

void CCustomObject::RenderRoot(int priority, bool strictB2F)
{
	if(FParentTools->IsVisible())
		Render(priority, strictB2F);
}

void CCustomObject::Render(int priority, bool strictB2F)
{
	if ((1 == priority) && (false == strictB2F))
	{
        if (m_Motion && Visible() && Selected())
            AnimationDrawPath();
    }

	if ((priority == 3) && (!strictB2F))
	{
		AxisManager::DrawAxis();
	}
}

bool CCustomObject::RaySelect(int flag, const Fvector& start, const Fvector& dir, bool bRayTest){
	float dist = UI->ZFar();
	if ((bRayTest&&RayPick(dist,start,dir))||!bRayTest){
		Select(flag);
        return true;
    }
    return false;
};

bool CCustomObject::FrustumSelect(int flag, const CFrustum& frustum){
	if (FrustumPick(frustum)){
    	Select(flag);
        return true;
    }
	return false;
};

bool CCustomObject::GetSummaryInfo(SSceneSummary* inf)
{
	Fbox bb; 
    if (GetBox(bb)){
        inf->bbox.modify(bb.min);
        inf->bbox.modify(bb.max);
    }
    return true;
}

void CCustomObject::OnSynchronize()
{
	OnFrame		();
}

