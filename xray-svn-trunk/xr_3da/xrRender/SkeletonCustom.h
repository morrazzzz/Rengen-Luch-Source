//---------------------------------------------------------------------------
#ifndef SkeletonCustomH
#define SkeletonCustomH

#include "fhierrarhyvisual.h"
#include "../bone.h"
#include "../../Include/xrRender/Kinematics.h"

// refs
class	 CKinematics;
class	 CInifile;
class	 CBoneData;
struct	SEnumVerticesCallback;

class CSkeletonWallmark : public intrusive_base
{
	CKinematics*		m_Parent;
	const Fmatrix*		m_XForm;
	ref_shader			m_Shader;
	Fvector3			m_ContactPoint;
	float				m_fTimeStart;
public:

#ifdef DEBUG
	u32					used_in_render;	
#endif

	Fsphere				m_LocalBounds;

	struct WMFace
	{
		Fvector3		vert	[3];
		Fvector2		uv		[3];
		u16				bone_id	[3][4];
		float			weight	[3][3];
		u8				render_mode;
	};

	DEFINE_VECTOR		(WMFace, WMFacesVec, WMFacesVecIt);

	WMFacesVec			m_Faces;
public:
	Fsphere				m_Bounds;
public:									
						CSkeletonWallmark(CKinematics* p,const Fmatrix* m, ref_shader s, const Fvector& cp, float ts):
						m_Parent(p), m_XForm(m), m_Shader(s), m_fTimeStart(ts), m_ContactPoint(cp)
						{
#ifdef DEBUG
						used_in_render = u32(-1);
#endif
}
						~CSkeletonWallmark()
#ifdef DEBUG
							;
#else
							{}
#endif

	IC CKinematics*		Parent			()													{ return m_Parent; }
	IC u32				VCount			()													{ return m_Faces.size() * 3; }
	IC bool				Similar			(ref_shader& sh, const Fvector& cp, float eps)		{ return (m_Shader == sh) && m_ContactPoint.similar(cp, eps); }
	IC float			TimeStart		()													{ return m_fTimeStart; }
	IC const Fmatrix*	XFORM			()													{ return m_XForm; }
	IC const Fvector3&	ContactPoint	()													{ return m_ContactPoint; }
	IC ref_shader		Shader			()													{ return m_Shader; }
};
DEFINE_VECTOR(intrusive_ptr<CSkeletonWallmark>, SkeletonWMVec, SkeletonWMVecIt);

class CKinematics: public FHierrarhyVisual, public IKinematics
{
	typedef FHierrarhyVisual	inherited;
	friend class				CBoneData;
	friend class				CSkeletonX;
protected: //--#SM+#--
	DEFINE_VECTOR(KinematicsABT::additional_bone_transform, BONE_TRANSFORM_VECTOR, BONE_TRANSFORM_VECTOR_IT);
	BONE_TRANSFORM_VECTOR m_bones_offsets;
public:

			void				Bone_Calculate		(CBoneData* bd, Fmatrix* parent);
			void				CLBone				(const CBoneData* bd, CBoneInstance& bi, const Fmatrix* parent, u8 mask_channel = (1<<0));

			void				BoneChain_Calculate	(const CBoneData* bd, CBoneInstance& bi,u8 channel_mask, bool ignore_callbacks);
			void				Bone_GetAnimPos		(Fmatrix& pos,u16 id, u8 channel_mask, bool ignore_callbacks);


	virtual	void				BuildBoneMatrix		(const CBoneData* bd, CBoneInstance& bi, const Fmatrix* parent, u8 mask_channel = (1<<0));
	virtual void				OnCalculateBones	(){}
	
	virtual void				CalculateBonesAdditionalTransforms(const CBoneData* bd, CBoneInstance& bi, const Fmatrix* parent, u8 mask_channel = (1 << 0)); //--#SM+#--
	virtual void				LL_AddTransformToBone(KinematicsABT::additional_bone_transform& offset); //--#SM+#--
	virtual void				LL_ClearAdditionalTransform(u16 bone_id = BI_NONE); //--#SM+#--

public:
	dxRender_Visual*			m_lod;
protected:
	SkeletonWMVec				wallmarks;
	u32							wm_frame;

	xr_vector<dxRender_Visual*>	children_invisible;

	// Globals
    CInifile*					pUserData;
	CBoneInstance*				bone_instances;	// bone instances
	vecBones*					bones;			// all bones	(shared)
	u16							iRoot;			// Root bone index

	// Fast search
	accel*						bone_map_N;		// bones  associations	(shared)	- sorted by name
	accel*						bone_map_P;		// bones  associations	(shared)	- sorted by name-pointer

	BOOL						Update_Visibility;
	u32							calculatedBonesFrame_;
	s32							UCalc_Visibox;

    Flags64						visimask;
    Flags64						hidden_bones;
    
	CSkeletonX*					LL_GetChild				(u32 idx);

	AccessLock					protectWalmarkCalc_;
	AccessLock					protectCalculatedBonesFrame_;

	// internal functions
	virtual CBoneData*			CreateBoneData			(u16 ID) { return xr_new<CBoneData>(ID); }
	virtual void				IBoneInstances_Create	();
	virtual void				IBoneInstances_Destroy	();
	void						Visibility_Invalidate	()	{ Update_Visibility = TRUE; };
	void						Visibility_Update		();

    void						LL_Validate				();

	IC u32						GetFrameCalced			()			{ protectCalculatedBonesFrame_.Enter(); u32 res = calculatedBonesFrame_; protectCalculatedBonesFrame_.Leave(); return res; };
	IC void						SetFrameCalced			(u32 val)	{ protectCalculatedBonesFrame_.Enter(); calculatedBonesFrame_ = val; protectCalculatedBonesFrame_.Leave(); };

public:
	UpdateCallback				Update_Callback;
	void*						Update_Callback_Param;
public:
	// wallmarks
	void						AddWallmark			(const Fmatrix* parent, const Fvector3& start, const Fvector3& dir, ref_shader shader, float size);
	void						CalculateWallmarks	(DsGraphBuffer& buffer, CFrustum& view_frust);
	void						RenderWallmark		(intrusive_ptr<CSkeletonWallmark> wm, FVF::LIT* &verts);
	void						ClearWallmarks		();
public:
				
				bool			PickBone			(const Fmatrix &parent_xform, IKinematics::pick_result &r, float dist, const Fvector& start, const Fvector& dir, u16 bone_id);
	virtual		void			EnumBoneVertices	(SEnumVerticesCallback &C, u16 bone_id);
public:
								CKinematics			();
	virtual						~CKinematics		();

	// Low level interface
				u16				_BCL	LL_BoneID			(LPCSTR  B);
				u16				_BCL	LL_BoneID			(const shared_str& B);
				LPCSTR			_BCL	LL_BoneName_dbg		(u16 ID);

				CInifile*		_BCL	LL_UserData			()						{ return pUserData; }
				accel*					LL_Bones			()						{ return bone_map_N; }
	ICF			CBoneInstance&	_BCL	LL_GetBoneInstance	(u16 bone_id)			{ R_ASSERT2(bone_id < LL_BoneCount(), make_string("Id %u. Max Id %u", bone_id, LL_BoneCount())); VERIFY(bone_instances); return bone_instances[bone_id]; }
	ICF const	CBoneInstance&	_BCL	LL_GetBoneInstance	(u16 bone_id) const		{ R_ASSERT2(bone_id < LL_BoneCount(), make_string("Id %u. Max Id %u", bone_id, LL_BoneCount())); VERIFY(bone_instances); return bone_instances[bone_id]; }

	CBoneData&					_BCL	LL_GetData			(u16 bone_id)
    {
		R_ASSERT2(bone_id < LL_BoneCount(), make_string("Id %u. Max Id %u", bone_id, LL_BoneCount()));
		R_ASSERT(bones);

		if (bone_id >= LL_BoneCount())
		{
			CBoneData& bd = *((*bones)[0]);

			return bd;
		}

        CBoneData& bd = *((*bones)[bone_id]);

        return bd;
    }

	virtual	const IBoneData&_BCL	GetBoneData(u16 bone_id) const
	{
		R_ASSERT2(bone_id < LL_BoneCount(), make_string("Id %u. Max Id %u", bone_id, LL_BoneCount()));
		R_ASSERT(bones);

		if (bone_id >= LL_BoneCount())
		{
			CBoneData& bd = *((*bones)[0]);

			return bd;
		}

        CBoneData& bd = *((*bones)[bone_id]);

        return bd;
	}

	CBoneData* _BCL LL_GetBoneData (u16 bone_id)
	{		
		R_ASSERT2(bone_id < LL_BoneCount(), make_string("Id %u. Max Id %u", bone_id, LL_BoneCount()));
		R_ASSERT(bones);

		if (bone_id >= LL_BoneCount())
		{
			CBoneData* bd = ((*bones)[0]);

			return bd;
		}

		u32	sz = sizeof(vecBones);
		u32	sz1 = sizeof(((*bones)[bone_id])->children);

		Msg("sz: %d",sz);
		Msg("sz1: %d",sz1);

        CBoneData* bd = ((*bones)[bone_id]);

        return bd;
	}
	u16						_BCL	LL_BoneCount		()	const			{ return u16(bones->size()); }
	u16								LL_VisibleBoneCount	()					{ u64 F = visimask.flags&((u64(1)<<u64(LL_BoneCount()))-1); return (u16)btwCount1(F); }
	ICF Fmatrix&			_BCL	LL_GetTransform		(u16 bone_id)		{ return LL_GetBoneInstance(bone_id).mTransform; }
	ICF const Fmatrix&		_BCL	LL_GetTransform		(u16 bone_id) const	{ return LL_GetBoneInstance(bone_id).mTransform; }
	ICF Fmatrix&					LL_GetTransform_R	(u16 bone_id)		{ return LL_GetBoneInstance(bone_id).mRenderTransform; }	// rendering only
	Fobb&							LL_GetBox			(u16 bone_id)		{ R_ASSERT2(bone_id < LL_BoneCount(), make_string("Id %u. Max Id %u", bone_id, LL_BoneCount())); return (*bones)[bone_id]->obb; }
	const Fbox&				_BCL	GetBox				()const				{ return vis.box ; }

	void							LL_GetBindTransform (xr_vector<Fmatrix>& matrices);
    int 							LL_GetBoneGroups 	(xr_vector<xr_vector<u16> >& groups);

	u16						_BCL	LL_GetBoneRoot		()					{ return iRoot; }
	void							LL_SetBoneRoot		(u16 bone_id)		{ R_ASSERT2(bone_id < LL_BoneCount(), make_string("Id %u. Max Id %u", bone_id, LL_BoneCount())); iRoot = bone_id; }

    BOOL					_BCL	LL_GetBoneVisible	(u16 bone_id)		{ R_ASSERT2(bone_id < LL_BoneCount(), make_string("Id %u. Max Id %u", bone_id, LL_BoneCount())); return visimask.is(u64(1)<<bone_id); }
    BOOL					_BCL	LL_IsNotBoneHidden	(u16 bone_id)		{ R_ASSERT2(bone_id < LL_BoneCount(), make_string("Id %u. Max Id %u", bone_id, LL_BoneCount())); return hidden_bones.is(u64(1)<<bone_id); }

	void							LL_SetBoneVisible	(u16 bone_id, BOOL val, BOOL bRecursive);
	void					_BCL	LL_HideBoneVisible	(u16 bone_id, BOOL bRecursive);

	u64						_BCL	LL_GetBonesVisible	()					{ return visimask.get(); }
	void							LL_SetBonesVisible	(u64 mask);

	// Main functionality
	virtual void					CalculateBones				(BonesCalcType calc_type = can_optimize);		// Recalculate skeleton
	void							CalculateBones_Invalidate	();
	void							Callback					(UpdateCallback C, void* Param)		{ Update_Callback = C; Update_Callback_Param = Param; }
	virtual void					NeedToCalcBones				(bool exact = false); // Assign me for bones update

	//	Callback: data manipulation
	virtual void					SetUpdateCallback(UpdateCallback pCallback) { Update_Callback = pCallback; }
	virtual void					SetUpdateCallbackParam(void* pCallbackParam) { Update_Callback_Param = pCallbackParam; }

	virtual UpdateCallback			GetUpdateCallback() { return Update_Callback; }
	virtual void*					GetUpdateCallbackParam() { return Update_Callback_Param; }

	// debug
#if defined(DRENDER) && defined(DEBUG)
	void							DebugRender			(Fmatrix& XFORM);
#else
       void							DebugRender			(Fmatrix& XFORM) {}
#endif

#ifdef DEBUG
protected:
	virtual shared_str		_BCL	getDebugName()	{ return dbg_name; }
public:
#endif


	// General "Visual" stuff
    virtual void					Copy				(dxRender_Visual* pFrom);
	virtual void					Load				(const char* N, IReader* data, u32 dwFlags);
	virtual void 					Spawn				();
	virtual void					Depart				();
    virtual void 					Release				();

	virtual	IKinematicsAnimated*dcast_PKinematicsAnimated()		{ return 0;	}
	virtual IRenderVisual*	_BCL dcast_RenderVisual()			{ return this; }
	virtual IKinematics*	_BCL dcast_PKinematics()			{ return this; }

	virtual u32	mem_usage (bool bInstance)
	{
		u32 sz = sizeof(*this);
		sz += bone_instances?bone_instances->mem_usage() : 0;

		if (!bInstance)
		{
			for (vecBonesIt b_it = bones->begin(); b_it != bones->end(); b_it++)
				sz += sizeof(vecBones::value_type) + (*b_it)->mem_usage();
		}

		return sz;
	}

private:
	bool			m_is_original_lod;
	
	static float	m_bloodmark_lifetime;
	
	static void		StaticInitClass();
	static bool		IsStaticInited;

	xr_vector<shared_str> L_parents; // local temp usage
	xr_vector<xr_vector<u16> > tempGroups; // local temp usage
};

IC CKinematics* PCKinematics(dxRender_Visual* V){ return V ? (CKinematics*)V->dcast_PKinematics() : 0; }

#endif
