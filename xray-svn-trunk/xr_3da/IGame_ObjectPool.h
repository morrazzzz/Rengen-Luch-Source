#pragma once

class ENGINE_API				CObject;

class ENGINE_API 				IGame_ObjectPool
{
	typedef xr_vector<CObject*>	ObjectVec;
	typedef ObjectVec::iterator	ObjectVecIt;

	ObjectVec					m_PrefetchObjects;
	ObjectVec					delayedDeletinonList; // These objects are disabled but are not removed from mem yet.

public:
	void						prefetch			();
	void						clear				();

	CObject*					create				(LPCSTR	name);
	void						destroy				(CObject* O, bool ignore_render);

	void						DeleteReadyObjects(); // Delete critical for prev frame objects

	IGame_ObjectPool			();
	virtual ~IGame_ObjectPool	();
};

