////////////////////////////////////////////////////////////////////////////
//	Module 		: script_binder.h
//	Created 	: 26.03.2004
//  Modified 	: 26.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Script objects binder
////////////////////////////////////////////////////////////////////////////

#pragma once

class CSE_Abstract;
class CScriptBinderObject;
class NET_Packet;

class CScriptBinder {
protected:
	CScriptBinderObject			*m_object;

public:
								CScriptBinder	();
	virtual						~CScriptBinder	();
			void				init			();
			void				clear			();
	virtual void				reinit			();
	
	virtual void				LoadCfg			(LPCSTR section);
	virtual void				reload			(LPCSTR section);
	
	virtual BOOL				SpawnAndImportSOData	(CSE_Abstract* data_containing_so);
	virtual void				DestroyClientObj		();
	virtual BOOL				net_SaveRelevant		();
	virtual void				RemoveLinksToCLObj		(CObject *object);
	
	virtual void				ScheduledUpdate	(u32 time_delta);
	
	virtual void				save			(NET_Packet &output_packet);
	virtual void				load			(IReader &input_packet);

			void				set_object		(CScriptBinderObject *object);
	IC		CScriptBinderObject	*object			();
};

#include "script_binder_inline.h"