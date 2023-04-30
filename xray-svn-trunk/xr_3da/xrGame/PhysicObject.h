#pragma once

#include "gameobject.h"
#include "physicsshellholder.h"
#include "physicsskeletonobject.h"
#include "PHSkeleton.h"
#include "script_export_space.h"
#include "animation_script_callback.h"
#include "xrserver_objects_alife.h"

class CSE_ALifeObjectPhysic;
class CPhysicsElement;
class moving_bones_snd_player;

class CSE_ALifeObjectPhysic;
struct SPHNetState;
typedef CSE_ALifeObjectPhysic::mask_num_items	mask_num_items;


class CPhysicObject : 
	public CPhysicsShellHolder,
	public CPHSkeleton
{
	typedef CPhysicsShellHolder inherited;
	EPOType					m_type					;
	float					m_mass					;
	ICollisionHitCallback	*m_collision_hit_callback;
	CBlend					*m_anim_blend			;
	moving_bones_snd_player *bones_snd_player		;
	anim_script_callback	m_anim_script_callback	;
private:
	//Creating
			void	CreateBody			(CSE_ALifeObjectPhysic	*po)													;
			void	CreateSkeleton		(CSE_ALifeObjectPhysic	*po)													;
			void	AddElement			(CPhysicsElement* root_e, int id)												;
private:
			void						run_anim_forward				();
			void						run_anim_back					();
			void						stop_anim						();
			void						play_bones_sound				();
			void						stop_bones_sound				();
			float						anim_time_get					();
			void						anim_time_set					( float time );
			void						create_collision_model			( );
			void						set_door_ignore_dynamics		( );
			void						unset_door_ignore_dynamics		( );
public:
			bool						get_door_vectors				( Fvector& closed, Fvector& open ) const;
public:
			CPhysicObject(void);
	virtual ~CPhysicObject(void);

	virtual void						LoadCfg							(LPCSTR section);
	
	virtual BOOL						SpawnAndImportSOData			( CSE_Abstract* data_containing_so)																	;
	virtual void						CreatePhysicsShell				(CSE_Abstract* e);
	virtual void						DestroyClientObj				();

	virtual void						net_Save						(NET_Packet& P)																	;
	virtual void						ScheduledUpdate(u32 dt);	//
	virtual void						UpdateCL						();
	virtual	BOOL						net_SaveRelevant				()																				;
	virtual BOOL						UsedAI_Locations				()																				;
	virtual ICollisionHitCallback		*get_collision_hit_callback		()																				;
	virtual void						set_collision_hit_callback		(ICollisionHitCallback *cc)														;
	virtual	bool						is_ai_obstacle					() const;

	virtual void						ExportDataToServer				(NET_Packet& P);
protected:
	virtual void						SpawnInitPhysics				(CSE_Abstract	*D)																;
	virtual void						RunStartupAnim					(CSE_Abstract	*D)																;
	virtual CPhysicsShellHolder			*PPhysicsShellHolder			()													{return PhysicsShellHolder();}
	virtual CPHSkeleton					*PHSkeleton						()																	{return this;}
	virtual	void						InitServerObject				(CSE_Abstract	*po)															;
	virtual void						PHObjectPositionUpdate			()																				;

	void								net_Export_PH_Params			(NET_Packet& P, SPHNetState& State, mask_num_items&	num_items);

	Flags16								m_flags;
	bool								m_just_after_spawn;
	bool								m_activated;

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CPhysicObject)
#undef script_type_list
#define script_type_list save_type_list(CPhysicObject)
