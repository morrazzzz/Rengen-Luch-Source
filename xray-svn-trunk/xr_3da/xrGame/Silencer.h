///////////////////////////////////////////////////////////////
// Silencer.h
// Silencer - апгрейд оружия глушитель 
///////////////////////////////////////////////////////////////

#pragma once
#include "inventory_item_object.h"

class CSilencer : public CInventoryItemObject {
private:
	typedef CInventoryItemObject inherited;
public:
	CSilencer (void);
	virtual ~CSilencer(void);

	virtual void LoadCfg				(LPCSTR section);
	
	virtual BOOL SpawnAndImportSOData	(CSE_Abstract* data_containing_so);
	virtual void DestroyClientObj		();

	virtual void AfterAttachToParent	();
	virtual void BeforeDetachFromParent	(bool just_before_destroy);

	virtual void UpdateCL			();
	
	virtual void renderable_Render	(IRenderBuffer& render_buffer);

};