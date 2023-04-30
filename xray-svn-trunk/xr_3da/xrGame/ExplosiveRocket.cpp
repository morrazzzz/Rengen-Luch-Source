//////////////////////////////////////////////////////////////////////
// ExplosiveRocket.cpp:	ракета, которой стреляет RocketLauncher 
//						взрывается при столкновении
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ExplosiveRocket.h"


CExplosiveRocket::CExplosiveRocket() 
{
}

CExplosiveRocket::~CExplosiveRocket() 
{
}

DLL_Pure *CExplosiveRocket::_construct	()
{
	CCustomRocket::_construct	();
	CInventoryItem::_construct	();
	return						(this);
}

void CExplosiveRocket::LoadCfg(LPCSTR section) 
{
	inherited::LoadCfg(section);
	CInventoryItem::LoadCfg(section);
	CExplosive::LoadCfg(section);
}

BOOL CExplosiveRocket::SpawnAndImportSOData(CSE_Abstract* data_containing_so) 
{
	BOOL result = inherited::SpawnAndImportSOData(data_containing_so);
	result=result&&CInventoryItem::SpawnAndImportSOData(data_containing_so);
	Fvector box;BoundingBox().getsize(box);
	float max_size=_max(_max(box.x,box.y),box.z);
	box.set(max_size,max_size,max_size);
	box.mul(3.f);
	CExplosive::SetExplosionSize(box);
	return result;
}

void CExplosiveRocket::Contact(const Fvector &pos, const Fvector &normal)
{
	if(eCollide == m_eState) return;

	if(m_bLaunched)
		CExplosive::GenExplodeEvent(pos,normal);

	inherited::Contact(pos, normal);
}

void CExplosiveRocket::DestroyClientObj() 
{
	CInventoryItem::DestroyClientObj();
	CExplosive::DestroyClientObj();
	inherited::DestroyClientObj();
}

void CExplosiveRocket::AfterDetachFromParent() 
{
	inherited::AfterDetachFromParent();
}

void CExplosiveRocket::BeforeDetachFromParent(bool just_before_destroy) 
{
	CInventoryItem::BeforeDetachFromParent(just_before_destroy);
	inherited::BeforeDetachFromParent(just_before_destroy);
}

void CExplosiveRocket::UpdateCL() 
{
	if(eCollide == m_eState)
	{
		CExplosive::UpdateCL();
		inherited::UpdateCL();
	}
	else
		inherited::UpdateCL();
}


void  CExplosiveRocket::OnEvent (NET_Packet& P, u16 type) 
{
	CExplosive::OnEvent(P, type);
	inherited::OnEvent(P,type);
}

#ifdef DEBUG
void CExplosiveRocket::OnRender()
{
	if (Render->currentViewPort != MAIN_VIEWPORT)
		return;
	
	inherited::OnRender();
}
#endif

void CExplosiveRocket::reinit				()
{
	inherited::reinit			();
	CInventoryItem::reinit			();
}

void CExplosiveRocket::reload					(LPCSTR section)
{
	inherited::reload				(section);
	CInventoryItem::reload			(section);
}

void CExplosiveRocket::activate_physic_shell	()
{
	inherited::activate_physic_shell();
}

void CExplosiveRocket::on_activate_physic_shell	()
{
	CCustomRocket::activate_physic_shell();
}

void CExplosiveRocket::setup_physic_shell		()
{
	inherited::setup_physic_shell();
}

void CExplosiveRocket::create_physic_shell		()
{
	inherited::create_physic_shell();
}

bool CExplosiveRocket::Useful					() const
{
	return			(inherited::Useful());
}
void CExplosiveRocket::RemoveLinksToCLObj(CObject* O )
{
	CExplosive::RemoveLinksToCLObj(O);
	inherited::RemoveLinksToCLObj(O);
}