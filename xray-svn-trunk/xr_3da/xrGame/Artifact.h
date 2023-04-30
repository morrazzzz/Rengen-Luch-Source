#pragma once

#include "hud_item_object.h"
#include "hit_immunity.h"
#include "../xrphysics/PHUpdateObject.h"
#include "script_export_space.h"

class SArtefactActivation;

class CArtefact :	public CHudItemObject, 
					public CPHUpdateObject {
private:
	typedef			CHudItemObject	inherited;
public:
									CArtefact						();
	virtual							~CArtefact						();

	virtual void					LoadCfg							(LPCSTR section);
	
	virtual BOOL					SpawnAndImportSOData			(CSE_Abstract* data_containing_so);
	virtual void					DestroyClientObj				();
	virtual void					ExportDataToServer				(NET_Packet& P);

	virtual void					AfterAttachToParent				();
	virtual void					BeforeDetachFromParent			(bool just_before_destroy);
	
	virtual void					UpdateCL						();
	virtual void					ScheduledUpdate					(u32 dt);	
			void					UpdateWorkload					(u32 dt);

	
	virtual bool					CanTake							() const;

	//virtual void					renderable_Render				(IRenderBuffer& render_buffer);
	virtual BOOL					renderable_ShadowGenerate		()		{ return FALSE;	}
	virtual BOOL					renderable_ShadowReceive		()		{ return TRUE;	}
	virtual void					create_physic_shell();

	//for smart_cast
	virtual CArtefact*				cast_artefact						()		{return this;}

	ALife::_OBJECT_ID_u32			owningAnomalyID_; // for storing and knowing if artefact is owned by anomaly or is already taken by somebody

protected:
	virtual void					UpdateCLChild						()		{};

	u16								m_CarringBoneID;
	shared_str						m_sParticlesName;
protected:
	SArtefactActivation*			m_activationObj;
	//////////////////////////////////////////////////////////////////////////
	//	Lights
	//////////////////////////////////////////////////////////////////////////
	//флаг, что подсветка может быть включена
	bool							m_bLightsEnabled;

	//подсветка во время полета и работы двигателя
	ref_light						m_pTrailLight;
	Fcolor							m_TrailLightColor;
	float							m_fTrailLightRange;
protected:
	virtual void					UpdateLights();
	
public:
	virtual void					StartLights();
	virtual void					StopLights();
	void							ActivateArtefact					();
	bool							CanBeActivated						()				{return m_bCanSpawnZone;};// does artefact can spawn anomaly zone

	virtual void					PhDataUpdate						(dReal step);
	virtual void					PhTune								(dReal step)	{};

	bool							m_bCanSpawnZone;
	//tatarinrafa: коефициент радиуса обнаружения для детектора. радиус обнаружения = fdetect_radius * detect_radius_koef
	float							detect_radius_koef;

	LPCSTR							custom_detect_sound_string;
	ref_sound						custom_detect_sound;

	float							m_fHealthRestoreSpeed;
	float 							m_fRadiationRestoreSpeed;
	float 							m_fSatietyRestoreSpeed;
	float							m_fPowerRestoreSpeed;
	float							m_fBleedingRestoreSpeed;
	float							m_fPsyhealthRestoreSpeed;
	CHitImmunity 					m_ArtefactHitImmunities;

	//tatarinrafa added additional_inventory_weight to artefacts
	float							m_additional_weight;

	//tatarinrafa: added additional jump speed sprint speed walk speed
	float							m_additional_jump_speed;
	float							m_additional_run_coef;
	float							m_additional_sprint_koef;

protected:
public:
	enum EAFHudStates {
		eIdle		= 0,
		eShowing,
		eHiding,
		eHidden,
		eActivating,
	};
	virtual	void					PlayAnimIdle		();
	virtual void					MoveTo				(Fvector const & position);
public:
	virtual void					Hide				();
	virtual void					Show				();
	virtual	void					UpdateXForm			();
	virtual bool					Action				(u16 cmd, u32 flags);
	virtual void					onMovementChanged	(ACTOR_DEFS::EMoveCommand cmd);
	virtual void					OnStateSwitch		(u32 S);
	virtual void					OnAnimationEnd		(u32 state);
	virtual bool					IsHidden			()	const	{return GetState()==eHidden;}

	// optimization FAST/SLOW mode
public:						
	u32						o_render_frame				;
	BOOL					o_fastmode					;
	IC void					o_switch_2_fast				()	{
		if (o_fastmode)		return	;
		o_fastmode			= TRUE	;
		//processing_activate		();
	}
	IC void					o_switch_2_slow				()	{
		if (!o_fastmode)	return	;
		o_fastmode			= FALSE	;
		//processing_deactivate		();
	}

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CArtefact)
#undef script_type_list
#define script_type_list save_type_list(CArtefact)

