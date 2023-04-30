#pragma once

#define MAX_OBJ_IDS 65535

class ENGINE_API CObject;
class NET_Packet;

class ENGINE_API CObjectList
{
private:

//.	xr_map<u32,CObject*>		map_NETID;
	CObject*					map_NETID[MAX_OBJ_IDS];

private:
	typedef xr_vector<CObject*>	Objects;

private:
	Objects						destroy_queue;
	Objects						objects_active;
	Objects						objects_sleeping;

	Objects						updatablesList_;
	Objects						updatablesListCopy_;

public:
	typedef fastdelegate::FastDelegate1<CObject*>	RELCASE_CALLBACK;
	struct SRelcasePair{
		int*					m_ID;
		RELCASE_CALLBACK		m_Callback;
								SRelcasePair		(int* id, RELCASE_CALLBACK cb) : m_ID(id), m_Callback(cb){}

		bool					operator==			(RELCASE_CALLBACK cb) { return m_Callback == cb; }
	};

	typedef xr_vector<SRelcasePair>					RELCASE_CALLBACK_VEC;
	RELCASE_CALLBACK_VEC							m_relcase_callbacks;

	void						relcase_register	(RELCASE_CALLBACK, int*);
	void						relcase_unregister	(int*);

	AccessLock					protectUpdatables_;

	xr_vector<fastdelegate::FastDelegate0<>>		frameEndObjectCalls;

public:
								CObjectList			();
								~CObjectList		();

	CObject*					FindObjectByName	(shared_str	name);
	CObject*					FindObjectByName	(LPCSTR name);
	CObject*					FindObjectByCLS_ID	(CLASS_ID cls);

	void						Load				();
	void						Unload				();

	CObject*					Create				(LPCSTR name);
	void						Destroy				(CObject* O, bool ignore_conds);

	void						SingleUpdate		(CObject* O);
	void						ObjectListUpdate	(bool bForce, bool disable_objects_updateCL_optim);
	void						ObjectListOnFrameEnd();
	void						DestroyOldObjects	(bool ignore_conds);

	void						net_Register		(CObject* O);
	void						net_Unregister		(CObject* O);

	// export objects data to server storage
	u32							ExportObjectsDataToServer(NET_Packet* P, u32 _start, u32 _count); // return next start

	ICF CObject* net_Find (u16 ID) const
	{
		if (ID == u16(-1))
			return nullptr;

		R_ASSERT2(ID < MAX_OBJ_IDS, make_string("ID is beyond of max objects ids. %u", ID));
		
		return (map_NETID[ID]);
	}

	void						o_updatable			(CObject* O);
	void						o_remove			(Objects& v, CObject* O);
	void						o_activate			(CObject* O);
	void						o_sleep				(CObject* O);

	IC u32						o_count				()	{ return objects_active.size() + objects_sleeping.size(); };

	IC CObject*					o_get_by_iterator	(u32 _it)
	{
		if (_it < objects_active.size())
			return objects_active[_it];
		else
			return objects_sleeping[_it - objects_active.size()];
	}

	bool						dump_all_objects	();

public:
	void						register_object_to_destroy	(CObject* object_to_destroy);
#ifdef DEBUG
	bool						registered_object_to_destroy(const CObject* object_to_destroy) const;
#endif

	IC const Objects&			GetUpdatablesList	() const
	{
		return (updatablesList_);
	}

	// Remve object from update list
	void						RemoveFromUpdList	(CObject* O);
	// Sanity check of update list
	u32							CheckForNulled		();


private:

	IC Objects&					GetUpdatablesList	()
	{
		return (updatablesList_);
	}

	static	void				ClearUpdatablesList	(Objects& o);
	static	void				dump_list			(Objects& v, LPCSTR reason);
};
