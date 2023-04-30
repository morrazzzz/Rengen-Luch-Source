#pragma once

#include "inventory_item_object.h"
#include "NightVisionUsable.h"
#include "script_export_space.h"

class CNightVisionDevice
	: public CInventoryItemObject
	, public CNightVisionUsable
{
public:
	CNightVisionDevice();
	virtual ~CNightVisionDevice();

	void LoadCfg(LPCSTR item_sect) override
	{
		CInventoryItemObject::LoadCfg(item_sect);
		CNightVisionUsable::LoadCfg(item_sect);
	}

	DECLARE_SCRIPT_REGISTER_FUNCTION
};

add_to_type_list(CNightVisionDevice)
#undef script_type_list
#define script_type_list save_type_list(CNightVisionDevice)