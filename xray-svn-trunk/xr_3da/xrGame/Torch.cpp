#include "stdafx.h"
#include "torch.h"
#include "entity.h"
#include "actor.h"
#include "../LightAnimLibrary.h"
#include "xrserver_objects_alife_items.h"
#include "ai_sounds.h"
#include "HUDManager.h"
#include "level.h"
#include "../../Include/xrRender/Kinematics.h"
#include "../camerabase.h"
#include "inventory.h"
#include "UIGameCustom.h"
#include "actorEffector.h"
#include "../xr_collide_form.h"
#include "CustomOutfit.h"

static const float		TIME_2_HIDE					= 5.f;
static const float		TORCH_INERTION_CLAMP		= PI_DIV_6;
static const float		TORCH_INERTION_SPEED_MAX	= 7.5f;
static const float		TORCH_INERTION_SPEED_MIN	= 0.5f;
static		 Fvector	TORCH_OFFSET				= {-0.2f,+0.1f,-0.3f};
static const Fvector	OMNI_OFFSET					= {-0.2f,+0.1f,-0.1f};
static const float		OPTIMIZATION_DISTANCE		= 100.f;

static bool stalker_use_dynamic_lights	= false;

ENGINE_API int g_current_renderer;

extern BOOL	gameObjectLightShadows_;

CTorch::CTorch(void)
{
	light_render				= ::Render->light_create();
	light_render->set_type		(IRender_Light::SPOT);
	light_render->set_shadow	(gameObjectLightShadows_ == TRUE ? true : false);
	light_render->set_use_static_occ(false);
	
	light_omni					= ::Render->light_create();
	
	light_omni->set_type		(IRender_Light::POINT);
	light_omni->set_shadow		(false);
	light_omni->set_use_static_occ(false);
		
	m_switched_on				= false;
	glow_render					= ::Render->glow_create();
	lanim						= 0;
	time2hide					= 0;
	fBrightness					= 1.f;

	m_RangeMax					= 20.f;
	m_RangeCurve				= 20.f;

	m_prev_hp.set				(0,0);
	m_delta_h					= 0;

#pragma todo("lets skip AlifeItem stuff")
	m_battery_state = 0.f;
	fUnchanreRate = 0.f;
}

CTorch::~CTorch(void) 
{
	light_render.destroy	();
	light_omni.destroy	();
	glow_render.destroy		();
	HUD_SOUND_ITEM::DestroySound	(m_FlashlightSwitchOnSnd);
	HUD_SOUND_ITEM::DestroySound(m_FlashlightSwitchOffSnd);
	sndBreaking.destroy();
}

inline bool CTorch::can_use_dynamic_lights	()
{
	if (!H_Parent())
		return				(true);

	CInventoryOwner			*owner = smart_cast<CInventoryOwner*>(H_Parent());
	if (!owner)
		return				(true);

	return					(owner->can_use_dynamic_lights());
}

void CTorch::LoadCfg(LPCSTR section) 
{
	inherited::LoadCfg		(section);
	light_trace_bone		= pSettings->r_string(section,"light_trace_bone");
	HUD_SOUND_ITEM::LoadSound(section, "snd_flashlight_switch_on", m_FlashlightSwitchOnSnd, 0, SOUND_TYPE_ITEM_USING);
	HUD_SOUND_ITEM::LoadSound(section, "snd_flashlight_switch_off", m_FlashlightSwitchOffSnd, 0, SOUND_TYPE_ITEM_USING);
	m_battery_duration		= pSettings->r_float(section, "battery_duration");

	if(pSettings->line_exist(section, "break_sound"))
		sndBreaking.create(pSettings->r_string(section, "break_sound"),st_Effect,sg_SourceType);

	Settings_from_ltx = READ_IF_EXISTS(pSettings, r_u8, section, "settings_from_ltx", 0);

	//tatarinrafa: Добавил возможность прописать это в конфиге
	if (Settings_from_ltx == 1)
	{
		LoadLightSettings(pSettings, section);
	}

	//Volumetric light
	volumetric				= !!READ_IF_EXISTS(pSettings, r_bool, section, "light_volumetric", FALSE);

	volumetric_distance		= READ_IF_EXISTS(pSettings, r_float, section, "volumetric_distance", 0.80f);
	volumetric_intensity	= READ_IF_EXISTS(pSettings, r_float, section, "volumetric_intensity", 0.40f);
	volumetric_quality		= READ_IF_EXISTS(pSettings, r_float, section, "volumetric_quality", 1.0f);

	fUnchanreRate			= READ_IF_EXISTS(pSettings, r_float, section, "uncharge_rate", 20.f);
	//lets give some more realistics to batteries. Now found tourches will have 70 to 100 precent battery status 
	float rondo = ::Random.randF(0.7f, 1.0f);
	m_battery_state = m_battery_duration * rondo;
}

void CTorch::LoadLightSettings(CInifile* ini, LPCSTR section)
{
	bool isR2 = !!psDeviceFlags.test(rsR2) || !!psDeviceFlags.test(rsR3) || !!psDeviceFlags.test(rsR4);

	IKinematics* K = smart_cast<IKinematics*>(Visual());
	lanim = LALib.FindItem(READ_IF_EXISTS(ini, r_string, section, "color_animator", "nill"));
	guid_bone = K->LL_BoneID(ini->r_string(section, "guide_bone"));
	VERIFY(guid_bone != BI_NONE);

	Fcolor clr = ini->r_fcolor(section, (isR2) ? "color_r2" : "color");
	fBrightness = clr.intensity();
	m_RangeMax = ini->r_float(section, (isR2) ? "range_max_r2" : "range_max");
	// m_RangeMin = ini->r_float(section, (isR2) ? "range_min_r2" : "range_min");
	m_RangeCurve = ini->r_float(section, "range_curve");

	light_render->set_color(clr);
	light_render->set_range(m_RangeMax);

	Fcolor clr_o = ini->r_fcolor(section, (isR2) ? "omni_color_r2" : "omni_color");
	float range_o = ini->r_float(section, (isR2) ? "omni_range_r2" : "omni_range");
	light_omni->set_color(clr_o);
	light_omni->set_range(range_o);

	light_render->set_cone(deg2rad(ini->r_float(section, "spot_angle")));
	light_render->set_texture(ini->r_string(section, "spot_texture"));

	glow_render->set_texture(ini->r_string(section, "glow_texture"));
	glow_render->set_color(clr);
	glow_render->set_radius(ini->r_float(section, "glow_radius"));

	if (volumetric)
	{
		light_render->set_volumetric(true);
		light_render->set_volumetric_distance(volumetric_distance);
		light_render->set_volumetric_intensity(volumetric_intensity);
		light_render->set_volumetric_quality(volumetric_quality);
	}
}

void CTorch::Switch()
{
	bool bActive			= !m_switched_on;
	Switch					(bActive);
}

void CTorch::Broke()
{
	if (m_switched_on) 
	{
		sndBreaking.play_at_pos(0, Position(), false);
		Switch(false);
	}
}

void CTorch::SetBatteryStatus(float val)
{
	m_battery_state = val;
	float condition = 1.f * m_battery_state / m_battery_duration;
	SetCondition(condition);
}

void CTorch::Switch	(bool light_on)
{
	CActor *pA = smart_cast<CActor *>(H_Parent());
	m_actor_item = (pA) ? true : false;
	if (light_on && m_battery_state < 0 && m_actor_item)
	{
		light_on = false;

		SDrawStaticStruct* s	= HUD().GetGameUI()->AddCustomStatic("torch_battery_low", true);
		s->m_endTime = EngineTime() + 3.0f;// 3sec
	}
	m_switched_on			= light_on;
	if (can_use_dynamic_lights())
	{
		light_render->set_active(light_on);
		if(!pA)light_omni->set_active(light_on);
	}
	glow_render->set_active					(light_on);

	if (*light_trace_bone) 
	{
		IKinematics* pVisual				= smart_cast<IKinematics*>(Visual()); VERIFY(pVisual);
		u16 bi								= pVisual->LL_BoneID(light_trace_bone);

		pVisual->LL_SetBoneVisible			(bi,	light_on,	TRUE);
		pVisual->CalculateBones				();

		pVisual->LL_SetBoneVisible			(bi,	light_on,	TRUE); //hack
	}
	
	if(m_actor_item)
	{
		HUD_SOUND_ITEM::PlaySound(m_switched_on ? m_FlashlightSwitchOnSnd : m_FlashlightSwitchOffSnd, pA->Position(), pA, true, false);
	}
}
bool CTorch::torch_active					() const
{
	return (m_switched_on);
}

BOOL CTorch::SpawnAndImportSOData(CSE_Abstract* data_containing_so) 
{
	CSE_Abstract			*e	= (CSE_Abstract*)(data_containing_so);
	CSE_ALifeItemTorch		*torch	= smart_cast<CSE_ALifeItemTorch*>(e);
	R_ASSERT				(torch);
	SetVisualName			(torch->get_visual());

	R_ASSERT				(!CFORM());
	R_ASSERT				(smart_cast<IKinematics*>(Visual()));
	collidable.model		= xr_new <CCF_Skeleton>(this);

	if (!inherited::SpawnAndImportSOData(data_containing_so))
		return(FALSE);
	
	//tatarinrafa: Добавил возможность прописать это в конфиге
	if (Settings_from_ltx == 0)
	{
		IKinematics* K = smart_cast<IKinematics*>(Visual());
		CInifile* pUserData = K->LL_UserData();
		R_ASSERT3(pUserData, "Empty Torch user data!", torch->get_visual());

		LoadLightSettings(pUserData, "torch_definition");
	}

	//включить/выключить фонарик
	Switch					(torch->m_active);
	VERIFY					(!torch->m_active || (torch->ID_Parent != 0xffff));

	m_delta_h				= PI_DIV_2-atan((m_RangeMax*0.5f)/_abs(TORCH_OFFSET.x));

	return					(TRUE);
}

void CTorch::DestroyClientObj() 
{
	Switch					(false);

	inherited::DestroyClientObj();
}

void CTorch::AfterAttachToParent() 
{
	inherited::AfterAttachToParent	();
	m_focus.set						(Position());
}

void CTorch::BeforeDetachFromParent	(bool just_before_destroy) 
{
	inherited::BeforeDetachFromParent(just_before_destroy);
	
	time2hide						= TIME_2_HIDE;

	Switch						(false);
}

void CTorch::Recharge(void)
{
	m_battery_state = m_battery_duration;
	SetCondition(1.f);
}

void CTorch::UpdateBattery(void)
{
	if (m_switched_on)
	{
		float minus = fUnchanreRate * TimeDelta();

		m_battery_state -= minus;

		float condition = 1.f * m_battery_state / m_battery_duration;
		SetCondition(condition);

		float rangeCoef = atan(m_RangeCurve * m_battery_state / m_battery_duration) / PI_DIV_2;
		clamp(rangeCoef, 0.f, 1.f);
		float range = m_RangeMax * rangeCoef;

		light_render->set_range	(range);
		m_delta_h	= PI_DIV_2-atan((range*0.5f)/_abs(TORCH_OFFSET.x));

		if (m_battery_state < 0)
		{
			Switch(false);
			return;
		}
	}
}

void CTorch::UpdateCL() 
{
#ifdef MEASURE_UPDATES
	CTimer measure_updatecl; measure_updatecl.Start();
#endif
	

	inherited::UpdateCL();

	if (!m_switched_on)
		return;

	CBoneInstance &BI = smart_cast<IKinematics*>(Visual())->LL_GetBoneInstance(guid_bone);

	Fmatrix	M;

	if (H_Parent()) 
	{
		CActor*	actor = smart_cast<CActor*>(H_Parent());

		if (actor)
			smart_cast<IKinematics*>(H_Parent()->Visual())->CalculateBones_Invalidate();

		if (H_Parent()->XFORM().c.distance_to_sqr(Device.vCameraPosition)<_sqr(OPTIMIZATION_DISTANCE))
		{
			// near camera
			smart_cast<IKinematics*>(H_Parent()->Visual())->CalculateBones();

			M.mul_43(XFORM(),BI.mTransform);
		}
		else
		{
			// approximately the same
			M = H_Parent()->XFORM();
			H_Parent()->Center(M.c);

			M.c.y += H_Parent()->Radius() * 2.f / 3.f;
		}

		if (actor) 
		{
			if (actor->IsLookAt())
			{
				m_prev_hp.x		= angle_inertion_var(m_prev_hp.x,-actor->cam_Active()->yaw,TORCH_INERTION_SPEED_MIN,TORCH_INERTION_SPEED_MAX,TORCH_INERTION_CLAMP,TimeDelta());
				m_prev_hp.y		= angle_inertion_var(m_prev_hp.y,-actor->cam_Active()->pitch,TORCH_INERTION_SPEED_MIN,TORCH_INERTION_SPEED_MAX,TORCH_INERTION_CLAMP,TimeDelta());
			}
			else
			{
				m_prev_hp.x		= angle_inertion_var(m_prev_hp.x,-actor->cam_FirstEye()->yaw,TORCH_INERTION_SPEED_MIN,TORCH_INERTION_SPEED_MAX,TORCH_INERTION_CLAMP,TimeDelta());
				m_prev_hp.y		= angle_inertion_var(m_prev_hp.y,-actor->cam_FirstEye()->pitch,TORCH_INERTION_SPEED_MIN,TORCH_INERTION_SPEED_MAX,TORCH_INERTION_CLAMP,TimeDelta());
			}

			Fvector dir, right, up;	
			dir.setHP(m_prev_hp.x + m_delta_h, m_prev_hp.y);

			Fvector::generate_orthonormal_basis_normalized(dir, up, right);

			Fvector offset = M.c;

			offset.mad(M.i, TORCH_OFFSET.x);
			offset.mad(M.j, TORCH_OFFSET.y);
			offset.mad(M.k, TORCH_OFFSET.z);

			light_render->set_position(offset);

			offset = M.c; 
			offset.mad(M.i, OMNI_OFFSET.x);
			offset.mad(M.j, OMNI_OFFSET.y);
			offset.mad(M.k, OMNI_OFFSET.z);

			light_omni->set_position(offset);

			glow_render->set_position(M.c);

			light_render->set_rotation(dir, right);

			light_omni->set_rotation(dir, right);

			glow_render->set_direction(dir);

		}
		else 
		{
			if (can_use_dynamic_lights()) 
			{
				light_render->set_position	(M.c);
				light_render->set_rotation	(M.k,M.i);

				Fvector offset = M.c;

				offset.mad					(M.i, OMNI_OFFSET.x);
				offset.mad					(M.j, OMNI_OFFSET.y);
				offset.mad					(M.k, OMNI_OFFSET.z);

				light_omni->set_position	(M.c);
				light_omni->set_rotation	(M.k,M.i);
			}

			glow_render->set_position	(M.c);
			glow_render->set_direction	(M.k);
		}
	}
	else 
	{
		if (getVisible() && m_pPhysicsShell)
		{
			M.mul(XFORM(), BI.mTransform);

			m_switched_on = false;

			light_render->set_active(false);
			light_omni->set_active(false);
			glow_render->set_active	(false);
		}
	}

	if (!m_switched_on)
		return;

	// calc color animator
	if (!lanim)
		return;

	int	frame;
	// возвращает в формате BGR
	u32 clr = lanim->CalculateBGR(EngineTime(), frame);

	Fcolor fclr;
	fclr.set((float)color_get_B(clr), (float)color_get_G(clr), (float)color_get_R(clr), 1.f);

	fclr.mul_rgb(fBrightness / 255.f);

	if (can_use_dynamic_lights())
	{
		light_render->set_color(fclr);
		light_omni->set_color(fclr);
	}

	glow_render->set_color(fclr);

	
#ifdef MEASURE_UPDATES
	Device.Statistic->updateCL_Actor_ += measure_updatecl.GetElapsed_sec();
#endif
}

void CTorch::create_physic_shell()
{
	CPhysicsShellHolder::create_physic_shell();
}

void CTorch::activate_physic_shell()
{
	CPhysicsShellHolder::activate_physic_shell();
}

void CTorch::setup_physic_shell	()
{
	CPhysicsShellHolder::setup_physic_shell();
}

void CTorch::ExportDataToServer(NET_Packet& P)
{
	inherited::ExportDataToServer(P);

	BYTE F = 0;
	F |= (m_switched_on ? eTorchActive : 0);

	const CActor *pA = smart_cast<const CActor *>(H_Parent());
	if (pA)
	{
		if (pA->attached(this))
			F |= eAttached;
	}

	P.w_u8(F);
	P.w_u16(256);
}

void CTorch::save(NET_Packet &output_packet)
{
	inherited::save(output_packet);
	save_data(m_battery_state, output_packet);

}

void CTorch::load(IReader &input_packet)
{
	inherited::load(input_packet);
	load_data(m_battery_state, input_packet);
}

bool CTorch::can_be_attached		() const
{
//	if( !inherited::can_be_attached() ) return false;

	const CActor *pA = smart_cast<const CActor *>(H_Parent());
	if (pA) 
	{
//		if(pA->inventory().Get(ID(), false))
		//check slot and belt
		bool b = false;
		int slot = BaseSlot();
		if (slot != NO_ACTIVE_SLOT)
			b = (const CTorch*)smart_cast<CTorch*>(pA->inventory().m_slots[slot].m_pIItem) == this;
		b = b || pA->inventory().IsInBelt(const_cast<CTorch*>(this));
		if(b)
			return true;
		else
			return false;
	}
	return true;
}
void CTorch::afterDetach			()
{
	inherited::afterDetach	();
	Switch					(false);
}
void CTorch::enable(bool value)
{
	inherited::enable(value);

	if(!enabled() && m_switched_on)
		Switch				(false);

}

void CTorch::renderable_Render(IRenderBuffer& render_buffer)
{
	inherited::renderable_Render(render_buffer);
}

float CTorch::GetBatteryLifetime() const
{
	return m_battery_duration;
}

float CTorch::GetBatteryStatus() const
{
	return m_battery_state;
}

bool CTorch::IsSwitchedOn() const
{
	return m_switched_on;
}