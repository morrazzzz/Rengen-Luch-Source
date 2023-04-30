#pragma once
#include "missile.h"
#include "explosive.h"
#include "../feel_touch.h"

#define SND_RIC_COUNT 5

class CGrenade :
	public CMissile,
	public CExplosive
{
	typedef CMissile		inherited;
public:
							CGrenade							();
	virtual					~CGrenade							();


	virtual void			LoadCfg								(LPCSTR section);
	
	virtual BOOL 			SpawnAndImportSOData				(CSE_Abstract* data_containing_so);
	virtual void 			DestroyClientObj					();
	virtual void 			RemoveLinksToCLObj					(CObject* O );

	virtual void 			BeforeDetachFromParent				(bool just_before_destroy);
	virtual void 			AfterDetachFromParent				();
	virtual void			BeforeAttachToParent				()				{inherited::BeforeAttachToParent();};
	virtual void 			AfterAttachToParent					();
	
	virtual void 			UpdateCL							();
	
	virtual void 			OnEvent								(NET_Packet& P, u16 type);
	
	virtual void			DiscardState						();
	virtual bool			DropGrenade							();			//in this case if grenade state is eReady, it should Throw
	
	virtual void 			OnAnimationEnd						(u32 state);

	virtual void 			Throw();
	virtual void 			Destroy();

	
	virtual bool			Action								(u16 cmd, u32 flags);
	virtual bool			Useful								() const;
	virtual void			State								(u32 state);

	virtual	void			Hit									(SHit* pHDS);

			void			PutNextToSlot						();

	virtual void			DeactivateItem						();
	virtual bool			GetBriefInfo						(II_BriefInfo& info);

	virtual void			SendHiddenItem						();	//same as OnHiddenItem but for client... (sends message to a server)...
protected:
	ALife::_TIME_ID			m_dwGrenadeRemoveTime;
	ALife::_TIME_ID			m_dwGrenadeIndependencyTime;
protected:
	ESoundTypes				m_eSoundCheckout;
private:
	float					m_grenade_detonation_threshold_hit;
	bool					m_thrown;

	CGrenade*				GetNextGrenadeType();
	CGrenade*				nextGrenade_;

	void					RemoveSelfFromSlot();
protected:
	virtual	void			UpdateXForm							()		{ CMissile::UpdateXForm(); };
public:

	virtual BOOL			UsedAI_Locations					();
	virtual CExplosive		*cast_explosive						()	{return this;}
	virtual CMissile		*cast_missile						()	{return this;}
	virtual CHudItem		*cast_hud_item						()	{return this;}
	virtual CGameObject		*cast_game_object					()	{return this;}
	virtual IDamageSource	*cast_IDamageSource					()	{return CExplosive::cast_IDamageSource();}

	typedef					fastdelegate::FastDelegate< void (CGrenade*) >	destroy_callback;
	void					set_destroy_callback				(destroy_callback callback) 
																{ m_destroy_callback = callback; }
private:
	destroy_callback		m_destroy_callback;
};
