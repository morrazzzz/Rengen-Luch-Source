#pragma once

#include "../xrcdb/ispatial.h"
#include "isheduled.h"
#include "irenderable.h"
#include "icollidable.h"
#include "engineapi.h"
#include "device.h"

class	ENGINE_API	IRender_Sector;
class	ENGINE_API	IRender_ObjectSpecific;
class	ENGINE_API	CCustomHUD;
class	NET_Packet;
class	CSE_Abstract;

#define FORCE_UPDATE_CL_RADIUS (30.f)
#define FORCE_UPDATE_CL_RADIUS2 (60.f)

class	IPhysicsShell;
xr_pure_interface	IObjectPhysicsCollision;

class ENGINE_API CObject :
	public DLL_Pure,
	public ISpatial,
	public ISheduled,
	public IRenderable,
	public ICollidable
{
public:
	struct	SavedPosition
	{
		u32			dwTime;
		Fvector		vPosition;
	};
	union	ObjectProperties
	{
		struct 
		{
			u32	net_ID			:	16;
			u32	bActiveCounter	:	8;
			u32	bEnabled		:	1;
			u32	bVisible		:	1;
			u32	bDestroy		:	1;
			u32	net_Local		:	1;
			u32	net_Ready		:	1;
			u32 net_SV_Update	:	1;
			u32 is_in_upd_list	:	1;
			u32	bPreDestroy		:	1;
		};
		u32	storage;
	};
private:
	// Some property variables
	ObjectProperties					Props;
	shared_str							NameObject;
	shared_str							NameSection;
	shared_str							NameVisual;
protected:
	// Parentness
	CObject*							Parent;

	// Geometric (transformation)
	svector<SavedPosition,4>			PositionStack;
public:
#ifdef DEBUG
	u32									dbg_update_cl;
#endif
	u32									dwFrame_UpdateCL;

#ifdef DEBUG
		void							DBGGetProps			(ObjectProperties &p) const { p = Props; }
#endif
		void							MakeMeUpdatable		(); // Makes object get UpdateCL call

	virtual BOOL						AlwaysInUpdateList	()					{ return FALSE; } // is always in the "UpdateCL list" ?

	ICF	bool							IsInUpdateList		() const			{ return !!Props.is_in_upd_list; }
	ICF	void							SetIsInUpdateList	(BOOL val)			{ Props.is_in_upd_list = val;}

	// Network
	ICF u16								ID					()			const	{ return Props.net_ID; }
	ICF void							setID				(u16 _ID)			{ Props.net_ID = _ID; }
	virtual BOOL						Ready				()					{ return Props.net_Ready; }
	BOOL								GetTmpPreDestroy	()			const	{ return Props.bPreDestroy; }
	void								SetTmpPreDestroy	(BOOL b)			{ Props.bPreDestroy = b;}
	virtual float						shedule_Scale		()					{ return Device.vCameraPosition.distance_to(Position()) / 200.f; }
	virtual bool						shedule_Needed		()					{ return processing_enabled(); };

	virtual	Fvector						get_new_local_point_on_mesh	(u16& bone_id);
	virtual	Fvector						get_last_local_point_on_mesh(Fvector const& last_point, const u16 bone_id);

	// Parentness
	IC CObject*							H_Parent			()					{ return Parent; }
	IC const CObject*					H_Parent			()			const	{ return Parent; }
	CObject*							H_Root				()					{ return Parent ? Parent->H_Root() : this; }
	const CObject*						H_Root				()			const	{ return Parent ? Parent->H_Root() : this; }
	CObject*							H_SetParent			(CObject* O, bool just_before_destroy = false);

	// Geometry xform
	virtual void						Center				(Fvector& C) const;
	IC const Fmatrix&					XFORM				()			 const	{ VERIFY(_valid(renderable.xform));	return renderable.xform; }
	ICF Fmatrix&						XFORM				()					{ return renderable.xform; }
	virtual void						spatial_register_intern		();
	virtual void						spatial_unregister_intern	();
	virtual void						spatial_move_intern		();
	void								spatial_update		(float eps_P, float eps_R);

	ICF Fvector&						Direction			() 					{ return renderable.xform.k; }
	ICF const Fvector&					Direction			() 			const	{ return renderable.xform.k; }
	ICF Fvector&						Position			() 					{ return renderable.xform.c; }
	ICF const Fvector&					Position			() 			const	{ return renderable.xform.c; }
	virtual float						Radius				()			const;
	virtual const Fbox&					BoundingBox			()			const;
	
	IC IRender_Sector*					Sector				()					{ return GetCurrentSector(); }
	IC IRender_ObjectSpecific*			ROS					()					{ return renderable_ROS(); }
	virtual BOOL						renderable_ShadowGenerate	()			{ return TRUE; }
	virtual BOOL						renderable_ShadowReceive	()			{ return TRUE; }

	// Accessors and converters
	ICF IRenderVisual*					Visual				() const			{ return renderable.visual; }
	ICF ICollisionForm*					CFORM				() const			{ return collidable.model; }
	virtual		CObject*				dcast_CObject		()					{ return this; }
	virtual		IRenderable*			dcast_Renderable	()					{ return this; }
	virtual void						OnChangeVisual		()					{ }
	virtual IPhysicsShell*				physics_shell		()					{ return 0; }

	// Name management
	ICF shared_str						ObjectName			()			const	{ return NameObject; } // Unique object name (usualy set in SDK or generated as section + id)
	ICF LPCSTR							ObjectNameStr		()			const	{ return NameObject.c_str(); } // Unique object name (usualy set in SDK or generated as section + id) (LPCSTR)
	void								SetObjectName		(shared_str N);
	ICF shared_str						SectionName			()			const	{ return NameSection; } // Name of section, from which the object loads variables cfg
	ICF LPCSTR							SectionNameStr		()			const	{ return NameSection.c_str(); } // Name (LPCSTR) of section, from which the object loads variables cfg
	void								SetSectionName		(shared_str N);
	ICF shared_str						VisualName			()			const	{ return NameVisual; } // Name of mesh/model/visual/ogf
	ICF LPCSTR							VisualNameStr		()			const	{ return NameVisual.c_str(); } // Name (LPCSTR) of mesh/model/visual/ogf
	void								SetVisualName		(shared_str N);
	virtual	shared_str					SchedulerName		()			const	{ return ObjectName(); }; // Name for debuging scheduled updates
	
	// Properties
	void								processing_activate		();				// request	to enable	UpdateCL
	void								processing_deactivate	();				// request	to disable	UpdateCL
	bool								processing_enabled		()				{ return 0!=Props.bActiveCounter; }

	void								setVisible			(BOOL _visible);
	ICF BOOL							getVisible			()			const	{ return Props.bVisible; }
	void								setEnabled			(BOOL _enabled);
	ICF BOOL							getEnabled			()			const	{ return Props.bEnabled; }
		void							setDestroy			(BOOL _destroy);
	ICF BOOL							getDestroy			()			const	{ return Props.bDestroy; }
	ICF void							setReady			(BOOL _ready)		{ Props.net_Ready = _ready ? 1 : 0; }
	ICF BOOL							getReady			()			const	{ return Props.net_Ready; }

	//---------------------------------------------------------------------
										CObject				();
	virtual								~CObject			();

	virtual void						LoadCfg				(LPCSTR section);					// Read object config
	
	virtual BOOL						SpawnAndImportSOData(CSE_Abstract* data_containing_so);	// Initialize client object and read data from Server Object
	virtual void						DestroyClientObj	();
	virtual void						ExportDataToServer	(NET_Packet& P) {};					// Export basic vatiables data to server storage. Use this to keep necessary data when object gets offline
	virtual BOOL						NeedDataExport		()				{ return FALSE; };	// Relevant for export to server
	virtual void						RemoveLinksToCLObj	(CObject*	 O) { };				// Destroy all links to another destroying objects
	
	// Update
	virtual void						ScheduledUpdate		(u32 dt);							// Called by Update Scheduler (Not each frame)
	virtual	void _stdcall				ScheduledFrameEnd	();									// Called by Update Scheduler(Not each frame). At frame end. Try to not pile it up
	virtual void						UpdateCL			();									// Called each frame, so no need for dt
	virtual void _stdcall				FrameEndCL			();									// Called each frame end. Try to not pile it up

	virtual void						renderable_Render	(IRenderBuffer& render_buffer);
	
	// Position stack
	IC u32								ps_Size				()			const	{ return PositionStack.size(); }
	virtual	SavedPosition				ps_Element			(u32 ID)	const;
	virtual void						ForceTransform		(const Fmatrix& m)	{};

	// HUD
	virtual void						OnHUDDraw			(CCustomHUD* hud, IRenderBuffer& render_buffer)	{};

	// Active/non active
	virtual void						BeforeAttachToParent	(); // process some misc stuff, before object is atteched to parent object
	virtual void						BeforeDetachFromParent	(bool just_before_destroy); // process some misc stuff, before object is detached from parent object
	virtual void						AfterAttachToParent		(); // process some misc stuff, after object is attached to parent object
	virtual void						AfterDetachFromParent	(); // process some misc stuff, after object is detached from parent object

	virtual void						On_SetEntity		()	{};
	virtual void						On_LostEntity		()	{};

public:
	virtual bool						register_schedule	() const {return true;}

	virtual	const IObjectPhysicsCollision	*physics_collision	()					{ return  0; }
};
