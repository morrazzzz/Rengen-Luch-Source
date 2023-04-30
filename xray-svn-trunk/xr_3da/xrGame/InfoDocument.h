///////////////////////////////////////////////////////////////
// InfoDocument.h
// InfoDocument - документ, содержащий сюжетную информацию
///////////////////////////////////////////////////////////////


#pragma once

#include "inventory_item_object.h"
#include "InfoPortionDefs.h"

class CInfoDocument: public CInventoryItemObject {
private:
    typedef	CInventoryItemObject inherited;
public:
	CInfoDocument(void);
	virtual ~CInfoDocument(void);
	
	virtual void LoadCfg				(LPCSTR section);
	
	virtual BOOL SpawnAndImportSOData	(CSE_Abstract* data_containing_so);
	virtual void DestroyClientObj	();
	
	virtual void ScheduledUpdate		(u32 dt);
	virtual void UpdateCL			();
	
	virtual void renderable_Render	(IRenderBuffer& render_buffer);

	virtual void AfterAttachToParent	();
	virtual void BeforeDetachFromParent	(bool just_before_destroy);

protected:
	//индекс информации, содержащейся в документе
	shared_str m_Info;
};
