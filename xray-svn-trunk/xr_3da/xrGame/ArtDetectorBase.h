#pragma once

#include "hud_item_object.h"
#include "ai_sounds.h"
//#include "script_export_space.h"

class CUIArtefactDetectorBase;

class CArtDetectorBase : public CHudItemObject
{
	typedef	CHudItemObject	inherited;
protected:
	CUIArtefactDetectorBase*			m_ui;
	bool			m_bFastAnimMode;
	bool			m_bNeedActivation;
public:
	CArtDetectorBase();
	virtual			~CArtDetectorBase();

	virtual void 	LoadCfg(LPCSTR section);

	virtual BOOL 	SpawnAndImportSOData(CSE_Abstract* data_containing_so);
	virtual void 	AfterAttachToParent		();
	virtual void 	BeforeDetachFromParent	(bool just_before_destroy);

	virtual void 	ScheduledUpdate(u32 dt);
	virtual void 	UpdateCL();

	bool 	IsWorking();

	virtual void	OnActiveItem();
	virtual void	OnHiddenItem();
	virtual void	OnStateSwitch(u32 S);
	virtual void	OnAnimationEnd(u32 state);
	virtual	void	UpdateXForm();
			void	OnMoveToSlot() override {};
			void	OnMoveToRuck() override {};

	// TODO: do we really need this "fast mode"?
	void			ToggleDetector(bool bFastMode);
	void			HideDetector(bool bFastMode, bool needToReactivate = false);
	void			HideDetectorInstantly(bool needToReactivate = false);
	void			ShowDetector(bool bFastMode);

	float							fdetect_radius;
	float							foverallrangetocheck;
	float							fortestrangetocheck;
	BOOL							for_test;
	BOOL							reaction_sound_off;
	xr_vector<shared_str>			af_types;
	LPCSTR	 						af_sounds_section;
	LPCSTR							closestart;
	LPCSTR							detector_section;
protected:
	void 			TurnDetectorInternal(bool b);

	void			UpdateVisibility();
	void			SwitchToBolt();
	bool			IsItemStateCompatible(CHudItem*);

	virtual void	UpdateWork();
	virtual void 	UpdateAf()				{};
	virtual void 	CreateUI()				{};

	bool			m_bWorking;
public:
	float							freqq;
	float							snd_timetime;
	float							cur_periodperiod;
	u8								feel_touch_update_delay;
	LPCSTR				detect_sndsnd_line;

	ref_sound			detect_snd;

	virtual bool			install_upgrade_impl(LPCSTR section, bool test);

	//DECLARE_SCRIPT_REGISTER_FUNCTION
};
//add_to_type_list(CArtDetectorBase)
//#undef script_type_list
//#define script_type_list save_type_list(CArtDetectorBase)

