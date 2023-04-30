////////////////////////////////////////////////////////////////////////////
//	Module 		: inventory_item_object.h
//	Created 	: 24.03.2003
//  Modified 	: 27.12.2004
//	Author		: Victor Reutsky, Yuri Dobronravin
//	Description : Inventory item object implementation
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "physic_item.h"
#include "inventory_item.h"
#include "script_export_space.h"

class CInventoryItemObject : 
			public CInventoryItem, 
			public CPhysicItem
{
public:
							CInventoryItemObject	();
	virtual					~CInventoryItemObject	();
	virtual DLL_Pure		*_construct				();

public:
	virtual CPhysicsShellHolder*cast_physics_shell_holder	()	{return this;}
	virtual CInventoryItem	*cast_inventory_item			()	{return this;}
	virtual CAttachableItem	*cast_attachable_item			()	{return this;}
	virtual CWeapon			*cast_weapon					()	{return 0;}
	virtual CFoodItem		*cast_food_item					()	{return 0;}
	virtual CMissile		*cast_missile					()	{return 0;}
	virtual CHudItem		*cast_hud_item					()	{return 0;}
	virtual CWeaponAmmo		*cast_weapon_ammo				()	{return 0;}
	virtual CGameObject		*cast_game_object				()  {return this;};

public:
	virtual void	LoadCfg					(LPCSTR section);
	
	virtual void	reload					(LPCSTR section);
	virtual void	reinit					();

	virtual BOOL	SpawnAndImportSOData	(CSE_Abstract* data_containing_so);
	virtual void	DestroyClientObj		();
	virtual void	ExportDataToServer		(NET_Packet& P);					// export to server
	
	virtual void	BeforeDetachFromParent	(bool just_before_destroy);
	virtual void	AfterDetachFromParent	();
	virtual void	BeforeAttachToParent	();
	virtual void	AfterAttachToParent		();
	
	virtual void	save					(NET_Packet &output_packet);
	virtual void	load					(IReader &input_packet);
	
	virtual BOOL	net_SaveRelevant		()								{return TRUE;}
	
	virtual void	UpdateCL				();
	
	virtual void	renderable_Render		(IRenderBuffer& render_buffer);
	
	virtual void	OnEvent					(NET_Packet& P, u16 type);
	
	virtual LPCSTR	Name					();
	virtual LPCSTR	NameShort				();
//.	virtual LPCSTR	NameComplex				();
	virtual	void	Hit						(SHit* pHDS);


	virtual void	activate_physic_shell	();
	virtual void	on_activate_physic_shell();
	virtual	void	modify_holder_params			(float &range, float &fov) const;
protected:
#ifdef DEBUG
	virtual void	OnRender				();
#endif

public:
	virtual bool	Useful					() const;

public:
	virtual u32		ef_weapon_type			() const;

	DECLARE_SCRIPT_REGISTER_FUNCTION;
protected:
	virtual bool	use_parent_ai_locations	() const
	{
		return CAttachableItem::use_parent_ai_locations();
	}

};

add_to_type_list(CInventoryItemObject)
#undef script_type_list
#define script_type_list save_type_list(CInventoryItemObject)



#include "inventory_item_inline.h"
