////////////////////////////////////////////////////////////////////////////
//	Module 		: eatable_item_object.cpp
//	Created 	: 24.03.2003
//  Modified 	: 29.01.2004
//	Author		: Yuri Dobronravin
//	Description : Eatable item object implementation
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "eatable_item_object.h"

CEatableItemObject::CEatableItemObject	()
{
}

CEatableItemObject::~CEatableItemObject	()
{
}

DLL_Pure *CEatableItemObject::_construct	()
{
	CEatableItem::_construct	();
	CPhysicItem::_construct		();
	return						(this);
}

void CEatableItemObject::LoadCfg				(LPCSTR section) 
{
	CPhysicItem::LoadCfg			(section);
	CEatableItem::LoadCfg			(section);
}

//void CEatableItemObject::Hit(float P, Fvector &dir,	
//						 CObject* who, s16 element,
//						 Fvector position_in_object_space, 
//						 float impulse, 
//						 ALife::EHitType hit_type)
void	CEatableItemObject::Hit					(SHit* pHDS)
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
	
	CEatableItem::Hit			(
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
	CEatableItem::Hit(pHDS);
}

void CEatableItemObject::AfterDetachFromParent()
{
	CEatableItem::AfterDetachFromParent		();
	CPhysicItem::AfterDetachFromParent		();
}

void CEatableItemObject::BeforeDetachFromParent(bool just_before_destroy)
{
	CEatableItem::BeforeDetachFromParent		(just_before_destroy);
	CPhysicItem::BeforeDetachFromParent			(just_before_destroy);
}

void CEatableItemObject::BeforeAttachToParent()
{
	CPhysicItem::BeforeAttachToParent		();
	CEatableItem::BeforeAttachToParent		();
}

void CEatableItemObject::AfterAttachToParent()
{
	CPhysicItem::AfterAttachToParent();
	CEatableItem::AfterAttachToParent();
}

void CEatableItemObject::UpdateCL()
{
#ifdef MEASURE_UPDATES
	CTimer measure_updatecl; measure_updatecl.Start();
#endif
	

	CPhysicItem::UpdateCL();
	CEatableItem::UpdateCL();

	
#ifdef MEASURE_UPDATES
	Device.Statistic->updateCL_VariousItems_ += measure_updatecl.GetElapsed_sec();
#endif
}

void CEatableItemObject::OnEvent			(NET_Packet& P, u16 type)
{
	CPhysicItem::OnEvent				(P, type);
	CEatableItem::OnEvent				(P, type);
}

BOOL CEatableItemObject::SpawnAndImportSOData(CSE_Abstract* data_containing_so)
{
	BOOL								res = CPhysicItem::SpawnAndImportSOData(data_containing_so);
	CEatableItem::SpawnAndImportSOData	(data_containing_so);
	return								(res);
}

void CEatableItemObject::DestroyClientObj()
{
	CEatableItem::DestroyClientObj			();
	CPhysicItem::DestroyClientObj			();
}

void CEatableItemObject::ExportDataToServer(NET_Packet& P) 
{	
	CEatableItem::ExportDataToServer(P);
}

void CEatableItemObject::save				(NET_Packet &packet)
{
	CPhysicItem::save					(packet);
	CEatableItem::save				(packet);
}

void CEatableItemObject::load				(IReader &packet)
{
	CPhysicItem::load					(packet);
	CEatableItem::load				(packet);
}

void CEatableItemObject::renderable_Render(IRenderBuffer& render_buffer)
{
	CPhysicItem::renderable_Render(render_buffer);
	CEatableItem::renderable_Render(render_buffer);
}

void CEatableItemObject::reload			(LPCSTR section)
{
	CPhysicItem::reload					(section);
	CEatableItem::reload				(section);
}

void CEatableItemObject::reinit		()
{
	CEatableItem::reinit				();
	CPhysicItem::reinit					();
}

void CEatableItemObject::activate_physic_shell	()
{
	CEatableItem::activate_physic_shell	();
}

void CEatableItemObject::on_activate_physic_shell()
{
	CPhysicItem::activate_physic_shell	();
}

#ifdef DEBUG
void CEatableItemObject::OnRender()
{
	if (Render->currentViewPort != MAIN_VIEWPORT)
		return;
	
	CEatableItem::OnRender();
}
#endif

u32	 CEatableItemObject::ef_weapon_type		() const
{
	return								(0);
}

bool CEatableItemObject::Useful				() const
{
	return			(CEatableItem::Useful());
}
