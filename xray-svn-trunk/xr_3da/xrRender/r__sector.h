
#if !defined(_PORTAL_H_)
#define _PORTAL_H_
#pragma once

class CPortal;
class CSector;

struct _scissor : public Fbox2
{
	float	depth;
};


// Connector
class CPortal : public IRender_Portal
#ifdef DEBUG
	, public pureRender
#endif
{
private:
	svector<Fvector,8>				poly;
	CSector							*pFace,*pBack;
public:
	Fplane							P;
	Fsphere							S;
	u32								marker;
	BOOL							bDualRender;

	void							Setup								(Fvector* V, int vcnt, CSector* face, CSector* back);

	svector<Fvector,8>&				getPoly()							{ return poly; }
	CSector*						Back()								{ return pBack; }
	CSector*						Front()								{ return pFace; }
	CSector*						getSector		(CSector* pFrom)	{ return pFrom == pFace ? pBack : pFace; }
	CSector*						getSectorFacing	(const Fvector& V)	{ if (P.classify(V) > 0) return pFace; else return pBack; }
	CSector*						getSectorBack	(const Fvector& V)	{ if (P.classify(V) > 0) return pBack; else return pFace; }
	float							distance		(const Fvector &V)	{ return _abs(P.classify(V)); }

									CPortal			();
	virtual							~CPortal		();

#ifdef DEBUG
	virtual void					OnRender		();
#endif
};


class dxRender_Visual;
class DsGraphBuffer;

struct LocalSubrenderSectorData // for mt support
{
public:

	LocalSubrenderSectorData()
	{

	}

	xr_vector<CFrustum> r_frustums;
	xr_vector<_scissor> r_scissors;

	u32					r_marker;
};

// Main 'Sector' class
class CSector : public IRender_Sector
{
protected:
	dxRender_Visual*				m_root;			// whole geometry of that sector
	xr_vector<CPortal*>				m_portals;

	AccessLock						protectMapingNewBuffer_;
public:

	xr_map<size_t, LocalSubrenderSectorData> subrendersData; // local for each subrender. Can overflow if many short life sources spam its mark and dont erase it at destruction

	_scissor						r_scissor_merged;
public:
	// Main interface
	dxRender_Visual*				root			(){ return m_root; }
	void							traverse		(CFrustum& F, _scissor& R, DsGraphBuffer& buffer);
	void							load			(IReader& fs);
	

	IC LocalSubrenderSectorData&	GetSubrenderData(size_t buffer_mark)
	{
		protectMapingNewBuffer_.Enter();

		auto it_f = subrendersData.find(buffer_mark); // find our buffer mark

		// if dont have our buffer mark - create it (ususally first scene frames)
		if (it_f == subrendersData.end())
#pragma todo("Can overflow if many short-life sources spam its mark. May be need to make a erase when light gets destroyed")
		{
			auto res = subrendersData.insert(mk_pair(buffer_mark, LocalSubrenderSectorData()));
			it_f = res.first;
		}

		LocalSubrenderSectorData& data = (*it_f).second;

		protectMapingNewBuffer_.Leave();

		return data;
	};

	CSector							()				{ m_root = NULL; }
	virtual							~CSector		();
};


class CPortalTraverser
{
public:
	enum
	{
		VQ_HOM		= (1<<0),
		VQ_SSA		= (1<<1),
		VQ_SCISSOR	= (1<<2),
		VQ_FADE		= (1<<3),				// requires SSA to work
	};

public:
	u32										iportal_marker;	// This is for portals only, mt usafe. Could not use DsGraphBuffer::portalTraverserMark since it causes flickering
	u32										i_options;		// input:	culling options
	Fvector									i_vBase;		// input:	"view" point
	Fmatrix									i_mXFORM;		// input:	4x4 xform
	Fmatrix									i_mXFORM_01;
	CSector*								i_start;		// input:	starting point
	xr_vector<std::pair<CPortal*, float>>	f_portals;
	ref_shader								f_shader;
	ref_geom								f_geom;

public:
									CPortalTraverser	();
	void							initialize			();
	void							destroy				();
	void							traverse			(IRender_Sector* start, CFrustum& F, Fvector& vBase, Fmatrix& mXFORM, u32 options, DsGraphBuffer& buffer);
	void							fade_portal			(CPortal* _p, float ssa);
	void							fade_render			();

#ifdef DEBUG
	void							dbg_draw		();
#endif
};

extern CPortalTraverser PortalTraverser;

#endif
