#pragma once

#include "iinputreceiver.h"
#include "xr_object_list.h"
#include "ParticleList.h"
#include "../xrcdb/xr_area.h"

class ENGINE_API CCameraManager;
class ENGINE_API CCursor;
class ENGINE_API CCustomHUD;
class ENGINE_API ISpatial;

namespace Feel { class ENGINE_API Sound; }

class ENGINE_API IGame_Level : 
	public DLL_Pure,
	public IInputReceiver,
	public pureRender,
	public pureFrame,
	public pureFrameBegin,
	public pureFrameEnd,
	public IEventReceiver
{
protected:
	// Network interface
	CObject*					pCurrentEntity;
	CObject*					pCurrentViewEntity;
   
	// Static sounds
	xr_vector<ref_sound>		Sounds_Random;
	u32							Sounds_Random_dwNextTime;
	BOOL						Sounds_Random_Enabled;

	CCameraManager*				m_pCameras;

	// temporary
	xr_vector<ISpatial*>		snd_ER;

public:

	CObjectList					Objects;
	CObjectSpace				ObjectSpace;

	CParticleList				Particles;

	CCameraManager&				Cameras			()				{ return *m_pCameras; };

	BOOL						bReady;

	CInifile*					pLevel;

	fastdelegate::FastDelegate1<float>		lastApplyCamera;
	float									lastApplyCameraVPNear;

public:	// deferred sound events

	struct	_esound_delegate
	{
		Feel::Sound*			dest;
		ref_sound_data_ptr		source;
		float					power;
	};
	xr_vector<_esound_delegate>	snd_Events;

public:
	// Main, global functions
	IGame_Level					();
	virtual ~IGame_Level		();

	virtual shared_str			name					() const = 0;

	virtual BOOL				Gameloading				(LPCSTR op_server)						= 0;
	virtual void				net_Load				(LPCSTR name)							= 0;
	virtual void				net_Save				(LPCSTR name)							= 0;
	virtual void				net_Stop				();
	virtual void				net_Update				()										= 0;

	virtual BOOL				Load					(u32 dwNum);
	virtual BOOL				Load_GameSpecific_Before()										{ return TRUE; };		// before object loading
	virtual BOOL				Load_GameSpecific_After	()										{ return TRUE; };		// after object loading
	virtual void				Load_GameSpecific_CFORM	(CDB::TRI* T, u32 count)				= 0;
	virtual bool				LoadTextures			();

	virtual void RenderTracers() = 0; //tracers 


	virtual void				OnFrame					();
	virtual void				OnRender				();
	virtual void				OnFrameBegin			();
	virtual void				OnFrameEnd				();

			void				RenderViewPort			(u32 viewport);

	virtual void				ApplyCamera				();

	// Main interface
	CObject*					CurrentEntity			() const							{ return pCurrentEntity; }
	CObject*					CurrentViewEntity		() const							{ return pCurrentViewEntity; }
	void						SetEntity				(CObject* O);
	void						SetViewEntity			(CObject* O);
	
	void						SoundEvent_Register		(ref_sound_data_ptr S, float range);
	void						SoundEvent_Dispatch		();
	void                        SoundEvent_OnDestDestroy(Feel::Sound*);

	void						LL_CheckTextures		();

	void						PlayRandomAmbientSnd	();

	void						DestroyEnvironment		();
	void						CreateEnvironment		();
	virtual void				PlayEnvironmentEffects	();
	virtual u64					GetTimeForEnv			() = 0;
	virtual float				GetTimeFactorForEnv		() = 0;
};

extern ENGINE_API IGame_Level* g_pGameLevel;

template <typename _class_type>
	void relcase_register	(_class_type *self, void (xr_stdcall _class_type::* function_to_bind)(CObject*))
	{
		g_pGameLevel->Objects.relcase_register	(
			CObjectList::RELCASE_CALLBACK (
				self,
				function_to_bind)
		);
	}

template <typename _class_type>
	void relcase_unregister	(_class_type *self, void (xr_stdcall _class_type::* function_to_bind)(CObject*))
	{
		g_pGameLevel->Objects.relcase_unregister	(
			CObjectList::RELCASE_CALLBACK (
				self,
				function_to_bind)
		);
	}
