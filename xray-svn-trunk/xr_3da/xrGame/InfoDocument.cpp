///////////////////////////////////////////////////////////////
// InfoDocument.cpp
// InfoDocument - документ, содержащий сюжетную информацию
///////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "InfoDocument.h"
#include "../../xrphysics/PhysicsShell.h"
#include "PDA.h"
#include "inventoryowner.h"
#include "xrserver_objects_alife_items.h"

CInfoDocument::CInfoDocument(void) 
{
	m_Info = NULL;
}

CInfoDocument::~CInfoDocument(void) 
{
}


BOOL CInfoDocument::SpawnAndImportSOData(CSE_Abstract* data_containing_so) 
{
	BOOL					res = inherited::SpawnAndImportSOData(data_containing_so);

	CSE_Abstract			*l_tpAbstract = static_cast<CSE_Abstract*>(data_containing_so);
	CSE_ALifeItemDocument	*l_tpALifeItemDocument = smart_cast<CSE_ALifeItemDocument*>(l_tpAbstract);
	R_ASSERT				(l_tpALifeItemDocument);

	m_Info					= l_tpALifeItemDocument->m_wDoc;

	return					(res);
}

void CInfoDocument::LoadCfg(LPCSTR section) 
{
	inherited::LoadCfg(section);
}

void CInfoDocument::DestroyClientObj() 
{
	inherited::DestroyClientObj();
}

void CInfoDocument::ScheduledUpdate(u32 dt) 
{
	inherited::ScheduledUpdate(dt);
}

void CInfoDocument::UpdateCL() 
{
	inherited::UpdateCL();
}


void CInfoDocument::AfterAttachToParent() 
{
	inherited::AfterAttachToParent();
	
	//передать информацию содержащуюся в документе
	//объекту, который поднял документ
	CInventoryOwner* pInvOwner = smart_cast<CInventoryOwner*>(H_Parent());
	if(!pInvOwner) return;
	
	//создать и отправить пакет о получении новой информации
	if(m_Info.size())
		pInvOwner->OnReceiveInfo(m_Info);
}

void CInfoDocument::BeforeDetachFromParent(bool just_before_destroy) 
{
	inherited::BeforeDetachFromParent(just_before_destroy);
}

void CInfoDocument::renderable_Render(IRenderBuffer& render_buffer) 
{
	inherited::renderable_Render(render_buffer);
}