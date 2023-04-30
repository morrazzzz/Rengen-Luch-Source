#include "stdafx.h"
#include "igame_level.h"
#include "IGame_Persistent.h"
#include "igame_objectpool.h"
#include "xr_object.h"

IGame_ObjectPool::IGame_ObjectPool()
{
}

IGame_ObjectPool::~IGame_ObjectPool()
{
	R_ASSERT(m_PrefetchObjects.empty());
}

// prefetch objects
void IGame_ObjectPool::prefetch()
{
	R_ASSERT(m_PrefetchObjects.empty());

	string256 prefetch_objects_section = "prefetch_objects";

	CInifile::Sect const & sect = pSettings->r_section(prefetch_objects_section);

	// First check sections presence
	bool found_not_existing = false;

	Msg(LINE_SPACER);

	for (CInifile::SectCIt I = sect.Data.begin(); I != sect.Data.end(); I++)
	{
		const CInifile::Item& item = *I;

		if (!pSettings->section_exist(item.first.c_str()))
		{
			Msg("!Section from object prefetch list does not exist: %s", item.first.c_str());

			found_not_existing = true;
		}
	}

	if (found_not_existing)
	{
		R_ASSERT2(false, make_string("One or more section from prefetch list does not exist. Check log and prefetch_objects.ltx. Object Prefetching is aborting"));

		return;
	}

	// Now do prefetching

	int	p_count = 0;
	::Render->model_Logging(FALSE);

	for (CInifile::SectCIt I=sect.Data.begin(); I!=sect.Data.end(); I++)
	{
		const CInifile::Item& item = *I;
		CLASS_ID CLS = pSettings->r_clsid(item.first.c_str(),"class");

		p_count ++;

		CObject* pObject = (CObject*) NEW_INSTANCE(CLS);
		pObject->LoadCfg(item.first.c_str());

		VERIFY2(pObject->SectionName().c_str(),item.first.c_str());

		m_PrefetchObjects.push_back(pObject);
	}

	// out statistic
	::Render->model_Logging	(TRUE);
}

void IGame_ObjectPool::clear()
{
	// Clear POOL
	ObjectVecIt it	= m_PrefetchObjects.begin();
	ObjectVecIt itE	= m_PrefetchObjects.end();

	for (; it!=itE; it++)	
		xr_delete(*it);

	m_PrefetchObjects.clear(); 
}

CObject* IGame_ObjectPool::create(LPCSTR name)
{
	CLASS_ID CLS = pSettings->r_clsid(name,"class");
	CObject* O = (CObject*) NEW_INSTANCE(CLS);

	O->SetSectionName	(name);
	O->LoadCfg			(name);

	return O;
}

void IGame_ObjectPool::DeleteReadyObjects()
{
#ifdef DEBUG
	u32 cnt = delayedDeletinonList.size();
#endif

	for (size_t i = 0; i < delayedDeletinonList.size(); ++i)
	{
		CObject* o = delayedDeletinonList[i];
		xr_delete(o);
	}

	delayedDeletinonList.clear();

#ifdef DEBUG
	if (cnt)
		Msg("* Post frame object deletion: %u objects deleted", cnt);
#endif
}

void IGame_ObjectPool::destroy(CObject*	O, bool ignore_render)
{
	if (O->renderable.needToDelete || ignore_render)
		xr_delete(O);
	else // Guarantee renderable lifetime for 1 more frame
	{
		delayedDeletinonList.push_back(O);
		O->renderable.needToDelete = true; // so that render don't add this obj again
	}
}
