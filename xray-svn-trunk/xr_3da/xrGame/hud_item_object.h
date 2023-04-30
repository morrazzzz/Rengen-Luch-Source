////////////////////////////////////////////////////////////////////////////
//	Module 		: hud_item_object.h
//	Created 	: 24.03.2003
//  Modified 	: 27.12.2004
//	Author		: Yuri Dobronravin
//	Description : HUD item
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "inventory_item_object.h"
#include "huditem.h"

class CHudItemObject : 
		public CInventoryItemObject,
		public CHudItem
{
protected: //чтоб нельзя было вызвать на прямую
						CHudItemObject		();
	virtual				~CHudItemObject		();

public:
	virtual	DLL_Pure	*_construct			();

public:
	virtual CHudItem	*cast_hud_item		()	{return this;}

public:
	virtual void		LoadCfg				(LPCSTR section);

	virtual	BOOL		SpawnAndImportSOData(CSE_Abstract* data_containing_so);
	virtual void		DestroyClientObj	();
	
	virtual void		AfterAttachToParent		();
	virtual void		BeforeAttachToParent	();
	virtual void		BeforeDetachFromParent	(bool just_before_destroy);
	virtual void		AfterDetachFromParent	();
	
	virtual void		UpdateCL			();
	
	virtual void		renderable_Render	(IRenderBuffer& render_buffer);
	virtual void		on_renderable_Render(IRenderBuffer& render_buffer);
	
	virtual void		OnEvent				(NET_Packet& P, u16 type);
	
	virtual bool		ActivateItem		();
	virtual void		DeactivateItem		();
	
	virtual bool		Action				(u16 cmd, u32 flags);
	virtual void		SwitchState			(u32 S);
	virtual void		OnStateSwitch		(u32 S);

			void		OnMoveToRuck		() override;

	virtual bool			use_parent_ai_locations	() const
	{
		return				CInventoryItemObject::use_parent_ai_locations	() && (CurrentFrame() != dwXF_Frame);
	}
};
