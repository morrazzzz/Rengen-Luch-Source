#include "stdafx.h"
#include "igame_level.h"

#include "xr_object.h"
#include "../xrcdb/xr_area.h"
#include "render.h"
#include "xrLevel.h"
#include "../Include/xrRender/RenderVisual.h"
#include "../Include/xrRender/Kinematics.h"

#include "x_ray.h"
#include "GameFont.h"

#include "mp_logging.h"
#include "xr_collide_form.h"

#pragma warning(push)
#pragma warning(disable:4995)
#include <intrin.h>
#pragma warning(pop)

#pragma intrinsic(_InterlockedCompareExchange)

inline void CObjectList::o_updatable(CObject* O)
{
	protectUpdatables_.Enter(); // In case, the object is added, while updating other objects, we should not corrupt list

	Objects& upd_list = GetUpdatablesList();
	
	R_ASSERT(O);
	R_ASSERT(O->SectionName());

	if (O->IsInUpdateList())
	{
		auto o_itter = std::find(upd_list.begin(), upd_list.end(), O);

		R_ASSERT(o_itter != upd_list.end()); // wtf check

		if (o_itter != upd_list.end()) // exit, if in the list already
		{
			protectUpdatables_.Leave();

			return;
		}
	}

	O->SetIsInUpdateList(TRUE);

	VERIFY(std::find(upd_list.begin(), upd_list.end(), O) == upd_list.end());

	upd_list.push_back(O);

	protectUpdatables_.Leave();
}

void CObject::MakeMeUpdatable()
{
	if (!processing_enabled())
		return;

	g_pGameLevel->Objects.o_updatable(this);
}

void CObject::SetObjectName(shared_str N)
{
	R_ASSERT(!engineState.test(FRAME_RENDERING));

	NameObject = N;
}

void CObject::SetSectionName(shared_str N)
{
	R_ASSERT(!engineState.test(FRAME_RENDERING));

	NameSection = N;
}

void CObject::SetVisualName(shared_str N)
{
	// check if equal
	if (*N && *NameVisual)
		if (N == NameVisual)
			return;

	R_ASSERT(!engineState.test(FRAME_RENDERING));

	// replace model
	if (*N && N[0])
	{
		IRenderVisual* old_v = renderable.visual;

		NameVisual = N;
		renderable.visual = Render->model_Create(*N);

		IKinematics* old_k = old_v ? old_v->dcast_PKinematics() : NULL;
		IKinematics* new_k = renderable.visual->dcast_PKinematics();

		if (old_k && new_k)
		{
			new_k->SetUpdateCallback(old_k->GetUpdateCallback());
			new_k->SetUpdateCallbackParam(old_k->GetUpdateCallbackParam());
		}

		::Render->model_Delete(old_v);
	}
	else
	{
		::Render->model_Delete(renderable.visual);
		NameVisual = 0;
	}

	OnChangeVisual();
}

// flagging
void CObject::processing_activate()
{
	VERIFY3(255 != Props.bActiveCounter, "Invalid sequence of processing enable/disable calls: overflow", *ObjectName());
#pragma todo("replace with bool, to avoid bugs")
	Props.bActiveCounter++;

	if (0 == (Props.bActiveCounter - 1))
		g_pGameLevel->Objects.o_activate(this);
}
void CObject::processing_deactivate()
{
	VERIFY3(0 != Props.bActiveCounter, "Invalid sequence of processing enable/disable calls: underflow", *ObjectName());

	Props.bActiveCounter--;

	if (0 == Props.bActiveCounter)
		g_pGameLevel->Objects.o_sleep(this);
}

void CObject::setEnabled(BOOL _enabled)
{
	//R_ASSERT(!engineState.test(FRAME_RENDERING));

	if (_enabled)
	{
		Props.bEnabled = 1;

		if (collidable.model)
			spatial.s_type |= STYPE_COLLIDEABLE;
	}
	else
	{
		Props.bEnabled = 0;
		spatial.s_type &= ~STYPE_COLLIDEABLE;
	}
}

void CObject::setVisible(BOOL _visible)
{
	//R_ASSERT(!engineState.test(FRAME_RENDERING));

	if (_visible)
	{
		// Parent should control object visibility itself (??????)
		Props.bVisible = 1;

		if (renderable.visual)
			spatial.s_type |= STYPE_RENDERABLE;
	}
	else
	{
		Props.bVisible = 0;
		spatial.s_type &= ~STYPE_RENDERABLE;
	}
}

void CObject::Center(Fvector& C) const
{
	VERIFY2(renderable.visual, *ObjectName());

	renderable.xform.transform_tiny(C, renderable.visual->getVisData().sphere.P);
}

float CObject::Radius() const
{
	VERIFY2(renderable.visual, *ObjectName());
	
	return renderable.visual->getVisData().sphere.R;
}

const Fbox&	CObject::BoundingBox() const
{
	VERIFY2(renderable.visual, *ObjectName());

	return renderable.visual->getVisData().box;
}

CObject::CObject() : ISpatial(g_SpatialSpace)
{
	// Transform
	Props.storage = 0;

	Parent = NULL;

	NameObject = NULL;
	NameSection = NULL;
	NameVisual = NULL;

	Props.is_in_upd_list = FALSE;

#ifdef DEBUG
	dbg_update_shedule = u32(-1) / 2;
	dbg_update_cl = u32(-1) / 2;
#endif
}

CObject::~CObject()
{
	R_ASSERT(!engineState.test(FRAME_PROCESING));
	R_ASSERT2(GetCurrentThreadId() == Core.mainThreadID, "Objects must be destructed only within main thread");

	SetVisualName(0);

	fastdelegate::RemoveFastDelegate(g_pGameLevel->Objects.frameEndObjectCalls, fastdelegate::FastDelegate0<>(this, &CObject::FrameEndCL));
	
	g_pGameLevel->Objects.RemoveFromUpdList(this);
}

void CObject::LoadCfg(LPCSTR section)
{
	R_ASSERT(!engineState.test(FRAME_PROCESING));

	// Name
	R_ASSERT(section);
	SetObjectName(section);
	SetSectionName(section);

	// Visual and light-track
	if (pSettings->line_exist(section, "visual"))
	{
		string_path tmp;

		xr_strcpy(tmp, pSettings->r_string(section, "visual"));

		if (strext(tmp))
			*strext(tmp) = 0;

		xr_strlwr(tmp);

		SetVisualName(tmp);
	}

	setVisible(false);
}

BOOL CObject::SpawnAndImportSOData(CSE_Abstract* data_containing_so)
{
	R_ASSERT(!engineState.test(FRAME_PROCESING));

	PositionStack.clear();

	VERIFY(_valid(renderable.xform));

	if (!Visual() && pSettings->line_exist(SectionName(), "visual"))
		SetVisualName(pSettings->r_string(SectionName(), "visual"));

	if (!collidable.model) 
	{
		if (pSettings->line_exist(SectionName(), "cform"))
		{
			VERIFY3(*NameVisual, "Model isn't assigned for object, but cform requisted", *ObjectName());

			collidable.model = xr_new <CCF_Skeleton>(this);
		}
	}

	R_ASSERT(spatial.space);
	spatial_register();

	if (register_schedule())
		shedule_register();

	// reinitialize flags
	processing_activate();
	setDestroy(false);

	MakeMeUpdatable();

	return TRUE;
}

void CObject::DestroyClientObj()
{
	VERIFY(getDestroy());
	R_ASSERT(!engineState.test(FRAME_PROCESING));

	xr_delete(collidable.model);

	if (register_schedule())
		shedule_unregister();

	spatial_unregister();
}

const float base_spu_epsP = 0.05f;
const float base_spu_epsR = 0.05f;

void CObject::spatial_update(float eps_P, float eps_R)
{
	R_ASSERT(!engineState.test(FRAME_PROCESING));

	BOOL bUpdate = FALSE;

	if (PositionStack.empty())
	{
		// Empty
		bUpdate = TRUE;
		PositionStack.push_back(SavedPosition());
		PositionStack.back().dwTime = EngineTimeU();
		PositionStack.back().vPosition = Position();
	}
	else
	{
		if (PositionStack.back().vPosition.similar(Position(), eps_P))
		{
			// Just update time
			PositionStack.back().dwTime = EngineTimeU();
		}
		else
		{
			// Register _new_ record
			bUpdate = TRUE;

			if (PositionStack.size() < 4)
			{
				PositionStack.push_back(SavedPosition());
			}
			else
			{
				PositionStack[0] = PositionStack[1];
				PositionStack[1] = PositionStack[2];
				PositionStack[2] = PositionStack[3];
			}

			PositionStack.back().dwTime = EngineTimeU();
			PositionStack.back().vPosition = Position();
		}
	}

	if (bUpdate)
	{
		spatial_move();
	}
	else
	{
		if (spatial.node_ptr)
		{
			// Object registered!
			if (!fsimilar(Radius(), spatial.sphere.R, eps_R))
				spatial_move();
			else
			{
				Fvector C;
				Center(C);

				if (!C.similar(spatial.sphere.P, eps_P))
					spatial_move();
			}

			// else nothing to do :_)
		}
	}
}

// Updates
void CObject::UpdateCL()
{
#ifdef MEASURE_UPDATES
	CTimer measure_updatecl; measure_updatecl.Start();
#endif


	// consistency check
#ifdef DEBUG
	VERIFY2(_valid(renderable.xform), *ObjectName());

	if (CurrentFrame() == dbg_update_cl)
		Debug.fatal(DEBUG_INFO, "'UpdateCL' called twice per frame for %s", *ObjectName());

	dbg_update_cl = CurrentFrame();

	if (Parent && spatial.node_ptr)
		Debug.fatal(DEBUG_INFO, "Object %s has parent but is still registered inside spatial DB", *ObjectName());

	if ((!collidable.model) && (spatial.s_type&STYPE_COLLIDEABLE))
		Debug.fatal(DEBUG_INFO, "Object %s registered as 'collidable' but has no collidable model", *ObjectName());
#endif

	g_pGameLevel->Objects.frameEndObjectCalls.push_back(fastdelegate::FastDelegate0<>(this, &CObject::FrameEndCL));

#pragma todo("This is not mistake, but doubtsome, since if object does not get its first UpdateCL call, than these hacks are not working")

	// Self assigning for next UpdateCL call
	if (Parent == g_pGameLevel->CurrentViewEntity())
		MakeMeUpdatable();

	else if (AlwaysInUpdateList())
		MakeMeUpdatable();
	else
	{
		float dist = Device.vCameraPosition.distance_to_sqr(Position());

		if (dist < FORCE_UPDATE_CL_RADIUS * FORCE_UPDATE_CL_RADIUS)
			MakeMeUpdatable();
		else if ((Visual() && Visual()->getVisData().nextHomTestframe + 2 > CurrentFrame()) && (dist < FORCE_UPDATE_CL_RADIUS2 * FORCE_UPDATE_CL_RADIUS2))
			MakeMeUpdatable();
	}


#ifdef MEASURE_UPDATES
	Device.Statistic->updateCL_Object_ += measure_updatecl.GetElapsed_sec();
#endif
}

void CObject::FrameEndCL()
{
	spatial_update(base_spu_epsP * 5, base_spu_epsR * 5);
}

void CObject::ScheduledUpdate(u32 T)
{
#ifdef MEASURE_UPDATES
	CTimer measure_sc_update; measure_sc_update.Start();
#endif

	// consistency check
	// Msg						("-SUB-:[%x][%s] CObject::ScheduledUpdate",dynamic_cast<void*>(this),*ObjectName());

	ISheduled::ScheduledUpdate(T);

	// Always send me in the list of "UpdateCL calls" on shedule-update 
	// Makes sure that UpdateCL called at least with freq of shedule-update
	MakeMeUpdatable();

	if (Visual())
	{
		IKinematics* K = Visual()->dcast_PKinematics();

		if (K)
			K->NeedToCalcBones();
	}

#ifdef MEASURE_UPDATES
	Device.Statistic->scheduler_Object_ += measure_sc_update.GetElapsed_sec();
#endif
}

void CObject::ScheduledFrameEnd()
{
	spatial_update(base_spu_epsP * 1, base_spu_epsR * 1);

	ISheduled::ScheduledFrameEnd();
}

void CObject::spatial_register_intern()
{
	Center(spatial.sphere.P);
	spatial.sphere.R = Radius();

	ISpatial::spatial_register_intern();
}

void CObject::spatial_unregister_intern()
{
	ISpatial::spatial_unregister_intern();
}

void CObject::spatial_move_intern()
{
	Center(spatial.sphere.P);
	spatial.sphere.R = Radius();

	ISpatial::spatial_move_intern();
}

CObject::SavedPosition CObject::ps_Element(u32 ID) const
{
	VERIFY(ID < ps_Size());

	return PositionStack[ID];
}

void CObject::renderable_Render(IRenderBuffer& render_buffer)
{
	// if we are rendered - than its a reason to update anyway
	MakeMeUpdatable();
}

CObject* CObject::H_SetParent(CObject* new_parent, bool just_before_destroy)
{
	if (new_parent == Parent)
		return new_parent;

	CObject* old_parent = Parent;

	VERIFY2((!new_parent) || (!old_parent), "Before set parent - execute H_SetParent(0)");

	if (!old_parent)
		BeforeAttachToParent();	// before attach
	else
		BeforeDetachFromParent(just_before_destroy); // before detach

	if (new_parent)
		spatial_unregister();
	else
		spatial_register();

	Parent = new_parent;
	if (!old_parent)
		AfterAttachToParent();	// after attach
	else
		AfterDetachFromParent(); // after detach

	MakeMeUpdatable();

	return old_parent;
}

void CObject::AfterAttachToParent()
{
}

void CObject::BeforeAttachToParent()
{
	setVisible(false);
}

void CObject::AfterDetachFromParent()
{
	setVisible(true);
}

void CObject::BeforeDetachFromParent(bool just_before_destroy)
{
}

void CObject::setDestroy(BOOL _destroy)
{
	if (_destroy == (BOOL)Props.bDestroy)
		return;

	Props.bDestroy = _destroy ? 1 : 0;

	if (_destroy)
	{
		g_pGameLevel->Objects.register_object_to_destroy(this);
#ifdef DEBUG
		extern BOOL debug_destroy;

		if (debug_destroy)
			Msg("cl setDestroy [%d][%d]", ID(), CurrentFrame());
#endif

#ifdef MP_LOGGING
		Msg("cl setDestroy [%d][%d]", ID(), CurrentFrame());
#endif
	}
	else
		VERIFY(!g_pGameLevel->Objects.registered_object_to_destroy(this));
}

Fvector CObject::get_new_local_point_on_mesh(u16& bone_id)
{
	bone_id = u16(-1);

	return Fvector().random_dir().mul(.7f);
}

Fvector CObject::get_last_local_point_on_mesh(Fvector const& local_point, u16 const bone_id)
{
	R_ASSERT2(CFORM(), make_string("invalid cform for '%s'", *NameObject));

	Fvector				result;

	// Fetch data
	Fmatrix				mE;
	const Fmatrix&		M = XFORM();
	const Fbox&			B = CFORM()->getBBox();

	// Build OBB + Ellipse and X-form point
	Fvector				c, r;
	Fmatrix				T, mR, mS;

	B.getcenter(c);
	B.getradius(r);
	T.translate(c);

	mR.mul_43(M, T);
	mS.scale(r);
	mE.mul_43(mR, mS);
	mE.transform_tiny(result, local_point);

	return result;
}