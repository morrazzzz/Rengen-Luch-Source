///////////////////////////////////////////////////////////////
// GrenadeLauncher.h
// GrenadeLauncher - апгрейд оружия поствольный гранатомет
///////////////////////////////////////////////////////////////

#pragma once
#include "inventory_item_object.h"

class CGrenadeLauncher : public CInventoryItemObject {
private:
	typedef CInventoryItemObject inherited;
public:
	CGrenadeLauncher (void);
	virtual ~CGrenadeLauncher(void);

	virtual void LoadCfg				(LPCSTR section);
	
	virtual BOOL SpawnAndImportSOData	(CSE_Abstract* data_containing_so);
	virtual void DestroyClientObj		();

	virtual void AfterAttachToParent	();
	virtual void BeforeDetachFromParent	(bool just_before_destroy);

	virtual void UpdateCL			();
	virtual void renderable_Render	(IRenderBuffer& render_buffer);

	float	GetGrenadeVel() {return m_fGrenadeVel;}

protected:
	//стартовая скорость вылета подствольной гранаты
	float m_fGrenadeVel;
};