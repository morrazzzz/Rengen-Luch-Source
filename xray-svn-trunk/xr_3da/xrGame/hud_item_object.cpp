#include "stdafx.h"
#include "hud_item_object.h"

CHudItemObject::CHudItemObject			()
{
}

CHudItemObject::~CHudItemObject			()
{
}

DLL_Pure *CHudItemObject::_construct	()
{
	CInventoryItemObject::_construct();
	CHudItem::_construct		();
	return						(this);
}

void CHudItemObject::LoadCfg				(LPCSTR section)
{
	CInventoryItemObject::LoadCfg	(section);
	CHudItem::LoadCfg				(section);
}

bool CHudItemObject::Action				(u16 cmd, u32 flags)
{
	if (CInventoryItemObject::Action(cmd, flags))
		return					(true);
	return						(CHudItem::Action(cmd,flags));
}

void CHudItemObject::SwitchState		(u32 S)
{
	CHudItem::SwitchState		(S);
}

void CHudItemObject::OnStateSwitch		(u32 S)
{
	CHudItem::OnStateSwitch		(S);
}

void CHudItemObject::OnMoveToRuck()
{
	CInventoryItemObject::OnMoveToRuck();
	CHudItem::OnMoveToRuck			();
}

void CHudItemObject::OnEvent			(NET_Packet& P, u16 type)
{
	CInventoryItemObject::OnEvent(P,type);
	CHudItem::OnEvent			(P,type);
}

void CHudItemObject::AfterAttachToParent()
{
	CHudItem::AfterAttachToParent();
	CInventoryItemObject::AfterAttachToParent();
}

void CHudItemObject::BeforeAttachToParent()
{
	CInventoryItemObject::BeforeAttachToParent();
	CHudItem::BeforeAttachToParent();
}

void CHudItemObject::BeforeDetachFromParent(bool just_before_destroy)
{
	CHudItem::BeforeDetachFromParent				(just_before_destroy);
	CInventoryItemObject::BeforeDetachFromParent	(just_before_destroy);
}

void CHudItemObject::AfterDetachFromParent()
{
	CHudItem::AfterDetachFromParent				();
	CInventoryItemObject::AfterDetachFromParent	();
}

BOOL CHudItemObject::SpawnAndImportSOData(CSE_Abstract* data_containing_so)
{
	return						(
		CInventoryItemObject::SpawnAndImportSOData(data_containing_so) &&
		CHudItem::SpawnAndImportSOData(data_containing_so)
	);
}

void CHudItemObject::DestroyClientObj()
{
	CHudItem::DestroyClientObj();
	CInventoryItemObject::DestroyClientObj();
}

bool CHudItemObject::ActivateItem()
{
	return (CHudItem::ActivateItem());
}

void CHudItemObject::DeactivateItem()
{
	CHudItem::DeactivateItem();
}

void CHudItemObject::UpdateCL()
{
	CInventoryItemObject::UpdateCL();
	CHudItem::UpdateCL();
}

void CHudItemObject::renderable_Render(IRenderBuffer& render_buffer)
{
	CHudItem::renderable_Render(render_buffer);
}

void CHudItemObject::on_renderable_Render(IRenderBuffer& render_buffer)
{
	CInventoryItemObject::renderable_Render(render_buffer);
}
