//////////////////////////////////////////////////////////////////////
// ExplosiveItem.h: класс для вещи которая взрывается под 
//					действием различных хитов (канистры,
//					балоны с газом и т.д.)
//////////////////////////////////////////////////////////////////////

#pragma once

#include "Explosive.h"
#include "inventory_item_object.h"
#include "DelayedActionFuse.h"
class CExplosiveItem: 
			public CInventoryItemObject	,
			public CDelayedActionFuse	,
			public CExplosive
{
private:
	typedef CInventoryItemObject inherited;

public:
	CExplosiveItem(void);
	virtual ~CExplosiveItem(void);

	virtual void				LoadCfg					(LPCSTR section)							;

	virtual BOOL				SpawnAndImportSOData	(CSE_Abstract* data_containing_so)			{return CInventoryItemObject::SpawnAndImportSOData(data_containing_so);}
	virtual void				DestroyClientObj		()											;
	virtual void				ExportDataToServer		(NET_Packet& P)								{CInventoryItemObject::ExportDataToServer(P);}
	virtual void				RemoveLinksToCLObj		(CObject* O )								;
	
	virtual void				ScheduledUpdate			(u32 dt)									;
	virtual bool				shedule_Needed			();
	virtual void				UpdateCL				()											;
	
	virtual void				renderable_Render		(IRenderBuffer& render_buffer)											; 
	
	virtual void				OnEvent					(NET_Packet& P, u16 type)					;
	
	virtual CGameObject			*cast_game_object		()											{return this;}
	virtual CExplosive*			cast_explosive			()											{return this;}
	virtual IDamageSource*		cast_IDamageSource		()											{return CExplosive::cast_IDamageSource();}

	virtual void				GetRayExplosionSourcePos(Fvector &pos)								;
	virtual void				ActivateExplosionBox	(const Fvector &size,Fvector &in_out_pos)	;

	virtual	void				Hit						(SHit* pHDS)								;

	virtual void				ChangeCondition			(float fDeltaCondition)						{CInventoryItem::ChangeCondition(fDeltaCondition);};
	virtual void				StartTimerEffects		()											;

};