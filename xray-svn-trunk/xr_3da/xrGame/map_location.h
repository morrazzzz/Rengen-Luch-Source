
#pragma once
#include "object_interfaces.h"
#include "alife_space.h"
#include "game_graph_space.h"

class CMapSpot;
class CMiniMapSpot;
class CMapSpotPointer;
class CComplexMapSpot;
class CUICustomMap;
class CInventoryOwner;

class CMapLocation :public IPureDestroyableObject
{
public:
enum ELocationFlags
{
	eSerailizable		= (1<<0),
	eHideInOffline		= (1<<1),
	eTTL				= (1<<2),
	ePosToActor			= (1<<3),
	ePointerEnabled		= (1<<4),
	eSpotEnabled		= (1<<5),
	eUserDefined		= (1<<6),
	eDontShowIfDead		= (1<<7),
	eMapLocation		= (1<<8), // special, for drawing of mapspots, that show areas names
	eCollidable			= (1<<9),
	eHintEnabled		= (1<<10),
};

protected:
	flags32					m_flags;
	shared_str				m_hint;
	u8						m_hint_priority;
	CMapSpot*				m_level_spot;
	CMapSpotPointer*		m_level_spot_pointer;
	CMiniMapSpot*			m_minimap_spot;
	CMapSpotPointer*		m_minimap_spot_pointer;
	bool					m_user_defined;
	LPCSTR					m_type;
	CComplexMapSpot*		m_complex_spot;
	CMapSpotPointer*		m_complex_spot_pointer;

	shared_str				m_spot_border_names			[6];
	CMapSpot*				m_level_map_spot_border;
	CMapSpot*				m_mini_map_spot_border;
	CMapSpot*				m_complex_spot_border;

	CMapSpot*				m_level_map_spot_border_na;
	CMapSpot*				m_mini_map_spot_border_na;
	CMapSpot*				m_complex_spot_border_na;

	u16						m_objectID;
	CSE_ALifeDynamicObject*	m_owner_se_object;
	int						m_ttl;
	u32						m_actual_time;
	Fvector					m_position_global; //last global position, actual time only current frame 
	Fvector2 				m_position_on_map; //last position on parent map, actual time only current frame

	struct SCachedValues{
		u32					m_updatedFrame;
		GameGraph::_GRAPH_ID m_graphID;
		Fvector2			m_Position;
		Fvector2			m_Direction;
		shared_str			m_LevelName;
		bool				m_Actuality;
	};
	SCachedValues			m_cached;

	bool					is_shown;
private:
							CMapLocation					(const CMapLocation&){R_ASSERT(0);} //disable copy ctor

protected :
	void					LoadSpot						(LPCSTR type, bool bReload); 
	void					UpdateSpot						(CUICustomMap* map, CMapSpot* sp );
	void					UpdateSpotPointer				(CUICustomMap* map, CMapSpotPointer* sp );
	void					CalcLevelName					();
	CMapSpotPointer*		GetSpotPointer					(CMapSpot* sp);
	CMapSpot*				GetSpotBorder					(CMapSpot* sp);
public:
							CMapLocation					(LPCSTR type, u16 object_id);
	virtual					~CMapLocation					();

	bool					IsShown							(){ return is_shown; };

	virtual void			destroy							();

	IC		bool			HintEnabled						()					{return !!m_flags.test(eHintEnabled);}
			LPCSTR			GetHint							();
	void					SetHint							(const shared_str& hint);
	u8						GetHintPrior					() const			{ return m_hint_priority; };
	void					SetHintPrior					(u8 value)			{ m_hint_priority = value; };
	LPCSTR					GetType							() const			{return m_type; };
	
	Fvector2				SpotSize						();
	CComplexMapSpot*		complex_spot					()					{return m_complex_spot;}
	const CMapSpot*			LevelMapSpot					()					{return m_level_spot;}
	const CMiniMapSpot*		MiniMapSpot						()					{return m_minimap_spot;}

	IC bool					PointerEnabled					()					{return SpotEnabled() && !!m_flags.test(ePointerEnabled);};
	IC void					EnablePointer					()					{m_flags.set(ePointerEnabled,TRUE);};
	IC void					DisablePointer					()					{m_flags.set(ePointerEnabled,FALSE);};

	IC bool					Collidable						() const			{return !!m_flags.test(eCollidable);}
	IC bool					SpotEnabled						()					{return !!m_flags.test(eSpotEnabled);};
	void					EnableSpot						()					{m_flags.set(eSpotEnabled,TRUE);};
	void					DisableSpot						()					{m_flags.set(eSpotEnabled,FALSE);};
	bool					IsUserDefined					() const			{return !!m_flags.test(eUserDefined);}
	bool					DontShowIfDead					() const			{return !!m_flags.test(eDontShowIfDead);}
	virtual void			UpdateMiniMap					(CUICustomMap* map);
	virtual void			UpdateLevelMap					(CUICustomMap* map);
	void					HighlightSpot					(bool state);


	void					CalcPosition					();
	const Fvector2&			CalcDirection					();
	IC const shared_str&	GetLevelName					()	{return m_cached.m_LevelName;}
	const Fvector2&			GetPosition						()	{return m_cached.m_Position;}

	u16						ObjectID						() {return m_objectID;}
	virtual		bool		Update							();
	Fvector					GetLastPosition					() {return m_position_global;};
	bool					Serializable					() const {return !!m_flags.test(eSerailizable);}
	void					SetSerializable					(bool b) {m_flags.set(eSerailizable,b);}

	virtual void			save							(IWriter &stream);
	virtual void			load							(IReader &stream);
	virtual bool			CanBeSelected					()						{return true;}
	virtual bool			CanBeUserRemoved				()						{return false;}


	virtual void			PrintMapLocationInfo(){};
	shared_str				m_owner_task_id;

#ifdef DEBUG
	virtual void			Dump							(){};
#endif

};

class CRelationMapLocation :public CMapLocation
{
	typedef CMapLocation inherited;
	shared_str				m_curr_spot_name;
	u16						m_pInvOwnerActorID;
	ALife::ERelationType	m_last_relation;
	bool					m_b_visible;
	bool					m_b_minimap_visible;
	bool					m_b_levelmap_visible;
protected:
	bool					IsVisible							() const {return m_b_visible;};
public:
							CRelationMapLocation			(const shared_str& type, u16 object_id, u16 pInvOwnerActorID);
	virtual					~CRelationMapLocation			();
	virtual bool			Update							();

	virtual void			UpdateMiniMap					(CUICustomMap* map);
	virtual void			UpdateLevelMap					(CUICustomMap* map);


	virtual void			PrintMapLocationInfo			();

};

class CUserDefinedMapLocation :public CMapLocation
{
	typedef CMapLocation inherited;
	shared_str				m_level_name;
	Fvector					m_position;
public:
	GameGraph::_GRAPH_ID	m_graph_id;
							CUserDefinedMapLocation			(LPCSTR type, u16 object_id);
	virtual					~CUserDefinedMapLocation		();
	virtual bool			Update							(); //returns actual
	virtual Fvector			PositionReal					();		
	virtual Fvector2		Position						();
	virtual Fvector2		Direction						();
	virtual shared_str		LevelName						();

			void			InitExternal					(const shared_str& level_name, const Fvector& pos);
	virtual void			save							(IWriter &stream);
	virtual void			load							(IReader &stream);

	virtual bool			CanBeSelected					()						{return true;}
	virtual bool			CanBeUserRemoved				()						{return true;}

	virtual void			PrintMapLocationInfo();

};
