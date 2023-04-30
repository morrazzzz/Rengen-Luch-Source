////////////////////////////////////////////////////////////////////////////
//	Module 		: inventory_item_object.cpp
//	Created 	: 24.03.2003
//  Modified 	: 27.12.2004
//	Author		: Victor Reutsky, Yuri Dobronravin
//	Description : Inventory item object implementation
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "inventory_item_object.h"

CInventoryItemObject::CInventoryItemObject	()
{
}

CInventoryItemObject::~CInventoryItemObject	()
{
}

DLL_Pure *CInventoryItemObject::_construct	()
{
	CInventoryItem::_construct	();
	CPhysicItem::_construct		();
	return						(this);
}

void CInventoryItemObject::LoadCfg				(LPCSTR section) 
{
	CPhysicItem::LoadCfg			(section);
	CInventoryItem::LoadCfg		(section);
}

LPCSTR CInventoryItemObject::Name			()
{
	return						(CInventoryItem::Name());
}

LPCSTR CInventoryItemObject::NameShort		()
{
	return						(CInventoryItem::NameShort());
}
/*
LPCSTR CInventoryItemObject::NameComplex	()
{
	return						(CInventoryItem::NameComplex());
}
*/

void				CInventoryItemObject::Hit					(SHit* pHDS)
{
	/*
	CPhysicItem::Hit			(
		P,
		dir,
		who,
		element,
		position_in_object_space,
		impulse,
		hit_type
	);
	
	CInventoryItem::Hit			(
		P,
		dir,
		who,
		element,
		position_in_object_space,
		impulse,
		hit_type
	);
	*/
	CPhysicItem::Hit(pHDS);
	CInventoryItem::Hit(pHDS);
}

void CInventoryItemObject::BeforeDetachFromParent(bool just_before_destroy)
{
	CInventoryItem::BeforeDetachFromParent	(just_before_destroy);
	CPhysicItem::BeforeDetachFromParent		(just_before_destroy);
}

void CInventoryItemObject::AfterDetachFromParent()
{
	CInventoryItem::AfterDetachFromParent	();
	CPhysicItem::AfterDetachFromParent		();
}


void CInventoryItemObject::BeforeAttachToParent()
{
	CPhysicItem::BeforeAttachToParent			();
	CInventoryItem::BeforeAttachToParent		();
}

void CInventoryItemObject::AfterAttachToParent()
{
	CPhysicItem::AfterAttachToParent	();
	CInventoryItem::AfterAttachToParent	();
}

void CInventoryItemObject::UpdateCL			()
{
	CPhysicItem::UpdateCL				();
	CInventoryItem::UpdateCL			();
}

void CInventoryItemObject::OnEvent			(NET_Packet& P, u16 type)
{
	CPhysicItem::OnEvent				(P, type);
	CInventoryItem::OnEvent				(P, type);
}

BOOL CInventoryItemObject::SpawnAndImportSOData(CSE_Abstract* data_containing_so)
{
	BOOL								res = CPhysicItem::SpawnAndImportSOData(data_containing_so);
	CInventoryItem::SpawnAndImportSOData(data_containing_so);
	return								(res);
}

void CInventoryItemObject::DestroyClientObj	()
{
	CInventoryItem::DestroyClientObj		();
	CPhysicItem::DestroyClientObj			();
}

void CInventoryItemObject::ExportDataToServer(NET_Packet& P) 
{	
	CInventoryItem::ExportDataToServer(P);
}

void CInventoryItemObject::save				(NET_Packet &packet)
{
	CPhysicItem::save					(packet);
	CInventoryItem::save				(packet);
}

void CInventoryItemObject::load				(IReader &packet)
{
	CPhysicItem::load					(packet);
	CInventoryItem::load				(packet);
}

void CInventoryItemObject::renderable_Render(IRenderBuffer& render_buffer)
{
	CPhysicItem::renderable_Render		(render_buffer);
#pragma todo("no need to call CInventoryItem::renderable_Render since it does same as CPhysicItem::renderable_Render")
	CInventoryItem::renderable_Render	(render_buffer);
}

void CInventoryItemObject::reload			(LPCSTR section)
{
	CPhysicItem::reload					(section);
	CInventoryItem::reload				(section);
}

void CInventoryItemObject::reinit		()
{
	CInventoryItem::reinit				();
	CPhysicItem::reinit					();
}

void CInventoryItemObject::activate_physic_shell	()
{
	CInventoryItem::activate_physic_shell	();
}

void CInventoryItemObject::on_activate_physic_shell	()
{
	CPhysicItem::activate_physic_shell	();
}

#ifdef DEBUG
void CInventoryItemObject::OnRender			()
{
	if (Render->currentViewPort != MAIN_VIEWPORT)
		return;
	
	CInventoryItem::OnRender			();
}
#endif

void CInventoryItemObject::modify_holder_params	(float &range, float &fov) const
{
	CInventoryItem::modify_holder_params		(range,fov);
}

u32	 CInventoryItemObject::ef_weapon_type		() const
{
	return								(0);
}

bool CInventoryItemObject::Useful				() const
{
	return			(CInventoryItem::Useful());
}
