#pragma once

#include "../feel_touch.h"
#include "inventory_item_object.h"

#include "InfoPortionDefs.h"
#include "character_info_defs.h"
#include "script_export_space.h"

#include "PdaMsg.h"


class CInventoryOwner;
class CPda;

DEF_VECTOR (PDA_LIST, CPda*);

class CPda :
	public CInventoryItemObject,
	public Feel::Touch
{
	typedef	CInventoryItemObject inherited;
public:
											CPda					();
	virtual									~CPda					();

	virtual void 							LoadCfg					(LPCSTR section);
	
	virtual BOOL 							SpawnAndImportSOData	(CSE_Abstract* data_containing_so);
	virtual void 							DestroyClientObj		();

	virtual void 							AfterAttachToParent		();
	virtual void 							BeforeDetachFromParent	(bool just_before_destroy);

	virtual void							save					(NET_Packet &output_packet);
	virtual void							load					(IReader &input_packet);

	virtual void 							ScheduledUpdate			(u32 dt);

	virtual void 							feel_touch_new			(CObject* O);
	virtual void 							feel_touch_delete		(CObject* O);
	virtual BOOL 							feel_touch_contact		(CObject* O);


	virtual u16								GetOriginalOwnerID		() {return m_idOriginalOwner;}
	virtual CInventoryOwner*				GetOriginalOwner		();
	virtual CObject*						GetOwnerObject			();


			void							TurnOn					() {m_bTurnedOff = false;}
			void							TurnOff					() {m_bTurnedOff = true;}
	
			bool 							IsActive				() {return IsOn();}
			bool 							IsOn					() {return !m_bTurnedOff;}
			bool 							IsOff					() {return m_bTurnedOff;}


			void							ActivePDAContacts		(xr_vector<CPda*>& res);
			CPda*							GetPdaFromOwner			(CObject* owner);
			u32								ActiveContactsNum		()							{return m_active_contacts.size();}
			void							PlayScriptFunction		();
			bool							CanPlayScriptFunction	() {if(!xr_strcmp(m_functor_str, "")) return false; return true;};


//*	virtual LPCSTR							Name					();

protected:
	void									UpdateActiveContacts	();


	xr_vector<CObject*>						m_active_contacts;
	float									m_fRadius;

	u16										m_idOriginalOwner;
	shared_str					m_SpecificChracterOwner;
	xr_string								m_sFullName;

	bool									m_bTurnedOff;

public:
	shared_str								m_functor_str;

	DECLARE_SCRIPT_REGISTER_FUNCTION
};

add_to_type_list(CPda)
#undef script_type_list
#define script_type_list save_type_list(CPda)
