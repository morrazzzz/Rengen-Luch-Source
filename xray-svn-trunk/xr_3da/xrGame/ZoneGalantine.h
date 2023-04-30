#pragma once

#include "mincer.h"

class CZoneGalantine : public CMincer
{
private:
	typedef CMincer inherited;
public:
						CZoneGalantine						();
	virtual				~CZoneGalantine						();
	
	virtual BOOL		SpawnAndImportSOData				(CSE_Abstract* data_containing_so);

	DECLARE_SCRIPT_REGISTER_FUNCTION
};

add_to_type_list(CZoneGalantine)
#undef script_type_list
#define script_type_list save_type_list(CZoneGalantine)