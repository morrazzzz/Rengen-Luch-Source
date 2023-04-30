#pragma once

#include "inventory_item_object.h"
//#include "night_vision_effector.h"
#include "hudsound.h"
#include "script_export_space.h"

class CLAItem;
class CNightVisionEffector;
class CMonsterEffector;

class CTorch : public CInventoryItemObject {
private:
    typedef	CInventoryItemObject	inherited;

protected:
	float			fBrightness;
	CLAItem*		lanim;
	float			time2hide;

	u16				guid_bone;
	shared_str		light_trace_bone;

	float			m_delta_h;
	Fvector2		m_prev_hp;
	bool			m_switched_on;
	ref_light		light_render;
	ref_light		light_omni;
	ref_glow		glow_render;
	Fvector			m_focus;
	// lost alpha start
	float			m_battery_duration;
	float			m_battery_state;
	float			fUnchanreRate;
	bool			m_actor_item;
private:
	inline	bool	can_use_dynamic_lights	();

public:
					CTorch					();
	virtual			~CTorch					();

	virtual void	LoadCfg				(LPCSTR section);
	virtual BOOL	SpawnAndImportSOData(CSE_Abstract* data_containing_so);
	virtual void	DestroyClientObj	();
	virtual void	ExportDataToServer	(NET_Packet& P);				// export to server

	virtual void	save					(NET_Packet &output_packet);
	virtual void	load					(IReader &input_packet);
	
	virtual void	AfterAttachToParent		();
	virtual void	BeforeDetachFromParent	(bool just_before_destroy);

	virtual void	UpdateCL			();
	
	virtual void	renderable_Render	(IRenderBuffer& render_buffer);
			
			void	UpdateBattery		(void);
			void	Switch				();
			void	Switch				(bool light_on);
			void	Broke				();

			void	Recharge(void);
			float	GetBatteryStatus(void) const;
			void	SetBatteryStatus(float val);
			bool	IsSwitchedOn(void) const; 
			float	GetBatteryLifetime(void) const;

	virtual bool	can_be_attached		() const;

	// volumetric light
	bool				volumetric;

	float				volumetric_distance;
	float				volumetric_intensity;
	float				volumetric_quality;
			bool	torch_active			() const;
	virtual	void	enable					(bool value);
 
public:

	HUD_SOUND_COLLECTION	m_sounds;
	ref_sound				sndBreaking;

	HUD_SOUND_ITEM				m_FlashlightSwitchOffSnd;
	HUD_SOUND_ITEM				m_FlashlightSwitchOnSnd;

	float					m_RangeMax;
	// float					m_RangeMin;
	float					m_RangeCurve;

	enum EStats
	{
		eTorchActive				= (1<<0),
		eAttached					= (1<<1)
	};

	u8						Settings_from_ltx;

			void	LoadLightSettings	(CInifile* ini, LPCSTR section);

public:

	virtual bool			use_parent_ai_locations	() const
	{
		return				(!H_Parent());
	}
	virtual void	create_physic_shell		();
	virtual void	activate_physic_shell	();
	virtual void	setup_physic_shell		();

	virtual void	afterDetach				();

	DECLARE_SCRIPT_REGISTER_FUNCTION
};

add_to_type_list(CTorch)
#undef script_type_list
#define script_type_list save_type_list(CTorch)
