#pragma once

#include "WeaponCustomPistol.h"
#include "script_export_space.h"

class CUIFrameWindow;
class CUIStatic;
class CBinocularsVision;

// CBinocularsVision usage moved to CWeapon
class CWeaponBinoculars: public CWeaponCustomPistol
{
private:
	typedef CWeaponCustomPistol inherited;

public:
					CWeaponBinoculars	(); 
	virtual			~CWeaponBinoculars	();

	void			LoadCfg				(LPCSTR section);

	virtual BOOL	SpawnAndImportSOData(CSE_Abstract* data_containing_so);
	
	virtual void	OnZoomIn			();
	virtual void	OnZoomOut			();
	virtual	void	ZoomInc				();
	virtual	void	ZoomDec				();

	//tatarinrafa: Uncoment when needed
//	virtual void	save				(NET_Packet &output_packet);
//	virtual void	load				(IReader &input_packet);
	bool			can_kill			() const { return false; };

	virtual bool	Action				(u16 cmd, u32 flags);
	virtual bool	use_crosshair		()	const {return false;}

	virtual LPCSTR	GetCurrentFireModeStr	() {return " ";};
	virtual bool	GetBriefInfo		(II_BriefInfo& info);
protected:

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CWeaponBinoculars)
#undef script_type_list
#define script_type_list save_type_list(CWeaponBinoculars)
