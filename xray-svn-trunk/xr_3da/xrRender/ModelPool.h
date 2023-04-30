// ModelPool.h: interface for the CModelPool class.

#ifndef ModelPoolH
#define ModelPoolH
#pragma once


class dxRender_Visual;

namespace PS
{ 
	struct  SEmitter; 
};


class ECORE_API CModelPool
{
private:
	friend class CRender;

	struct str_pred : public std::binary_function<const shared_str&, const shared_str&, bool> 
	{	
		IC bool operator()(const shared_str& x, const shared_str& y) const
		{
			return xr_strcmp(x, y) < 0;
		}
	};

	struct ModelDef
	{
		shared_str			name;
		dxRender_Visual*	model;
        u32					refs;
        ModelDef()			{ refs = 0; model = 0; }
	};

	typedef xr_multimap<shared_str,dxRender_Visual*,str_pred>	POOL;
	typedef POOL::iterator										POOL_IT;
	typedef xr_map<dxRender_Visual*,shared_str>					REGISTRY;
	typedef REGISTRY::iterator									REGISTRY_IT;

private:
	xr_vector<ModelDef>			Models;				// Reference / Base
	xr_vector<dxRender_Visual*>	ModelsToDelete;		// 
	REGISTRY					Registry;			// Just pairing of pointer / Name
	POOL						Pool;				// Unused / Inactive
	BOOL						bLogging;
    BOOL						bForceDiscard;
    BOOL						bAllowChildrenDuplicate;

	xr_vector<shared_str>		suggestedForPrefetchingVisuals;

	void						Destroy	();

public:
	u32							createdModelsCnt_;
	u32							deletedModelsCnt_;


								CModelPool			();
	virtual 					~CModelPool			();

	dxRender_Visual*			Instance_Create		(u32 Type);
	dxRender_Visual*			Instance_Duplicate	(dxRender_Visual* V);
	dxRender_Visual*			Instance_Load		(LPCSTR N, BOOL allow_register);
	dxRender_Visual*			Instance_Load		(LPCSTR N, IReader* data, BOOL allow_register);
	void						Instance_Register	(LPCSTR N, dxRender_Visual* V);
	dxRender_Visual*			Instance_Find		(LPCSTR N);

	dxRender_Visual*			CreatePE			(PS::CPEDef* source);
	dxRender_Visual*			CreatePG			(PS::CPGDef* source);
	dxRender_Visual*			Create				(LPCSTR name, IReader* data=0);
	dxRender_Visual*			CreateChild			(LPCSTR name, IReader* data);

	void					DeleteVisual		(dxRender_Visual* &V, BOOL bDiscard=FALSE);
	void					Discard				(dxRender_Visual* &V, BOOL b_complete);
	void					DeleteInternal		(dxRender_Visual* &V, BOOL bDiscard=FALSE);
	void					DeleteQueue			();

	void					RemoveModelReffs	(dxRender_Visual* visual_to_delete);

	void					Logging				(BOOL bEnable)	{ bLogging = bEnable; }
	
	void					Prefetch			();
	void					ClearPool			(BOOL b_complete);

	void					dump 				();
	void					ReportVisualsForPrefetching(); //Prints the list of visuals, which caused stutterings during game

	void					memory_stats		(u32& vb_mem_video, u32& vb_mem_system, u32& ib_mem_video, u32& ib_mem_system);
};

#endif
