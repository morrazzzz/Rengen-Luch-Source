#pragma once
#include "../BaseMonster/base_monster.h"
#include "../controlled_entity.h"
#include "../../../script_export_space.h"

class CAI_Flesh : public CBaseMonster,
				  public CControlledEntity<CAI_Flesh> {

	typedef		CBaseMonster					inherited;
	typedef		CControlledEntity<CAI_Flesh>	CControlled;

public:
							CAI_Flesh		();
	virtual					~CAI_Flesh		();	
	
	virtual	void	LoadCfg					(LPCSTR section);
	virtual	BOOL	SpawnAndImportSOData	(CSE_Abstract* data_containing_so);

	virtual	void	CheckSpecParams			(u32 spec_params);

	virtual bool	ability_can_drag		() {return true;}

	virtual	char*	get_monster_class_name () { return "flesh"; }


private:
	bool	ConeSphereIntersection	(Fvector ConeVertex, float ConeAngle, Fvector ConeDir, 
									Fvector SphereCenter, float SphereRadius);
	
	DECLARE_SCRIPT_REGISTER_FUNCTION
};

add_to_type_list(CAI_Flesh)
#undef script_type_list
#define script_type_list save_type_list(CAI_Flesh)

