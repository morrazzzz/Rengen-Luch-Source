#pragma once

#include "xrstring.h"

///		An interface which provides quick access to the NV type
///		of the item, the engine class of which should derive from this
///		interface. 	

class CNightVisionUsable
{
public:
	CNightVisionUsable() = default;
	virtual ~CNightVisionUsable() = default;

	void LoadCfg(LPCSTR item_sect)
	{
		nvEffectType_ = READ_IF_EXISTS(pSettings, r_u8, item_sect, "nv_use_shader_instead_off_ppe", 0);
		if (nvEffectType_ == 0)
			NightVisionSectionPPE = READ_IF_EXISTS(pSettings, r_string, item_sect, "nightvision_sect_ppe", NULL);
		else
			NightVisionSectionShader = READ_IF_EXISTS(pSettings, r_string, item_sect, "nightvision_sect_shader", NULL);
	}

	u8 nvEffectType_; //0 - ppe, 1 - advabced shader effect

	shared_str NightVisionSectionPPE;
	shared_str NightVisionSectionShader;

};