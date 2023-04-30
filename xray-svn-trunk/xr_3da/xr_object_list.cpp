#include "stdafx.h"
#include "igame_level.h"
#include "igame_persistent.h"

#include "xrSheduler.h"
#include "xr_object_list.h"
#include "BaseInstanceClasses.h"

#include "xr_object.h"
#include "CustomHUD.h"

class fClassEQ {
	CLASS_ID cls;
public:
	fClassEQ(CLASS_ID C) : cls(C) {};
	IC bool operator() (CObject* O) { return cls == O->CLS_ID; }
};

#ifdef	DEBUG
BOOL debug_destroy = TRUE;
#endif

CObjectList::CObjectList()
{
	ZeroMemory(map_NETID, MAX_OBJ_IDS * sizeof(CObject*));
}

CObjectList::~CObjectList()
{
	R_ASSERT(objects_active.empty());
	R_ASSERT(objects_sleeping.empty());
	R_ASSERT(destroy_queue.empty());
}

CObject* CObjectList::FindObjectByName(shared_str name)
{
	for (Objects::iterator I = objects_active.begin(); I != objects_active.end(); I++)
		if ((*I)->ObjectName().equal(name))	return (*I);

	for (Objects::iterator I = objects_sleeping.begin(); I != objects_sleeping.end(); I++)
		if ((*I)->ObjectName().equal(name))	return (*I);

	return	NULL;
}
CObject* CObjectList::FindObjectByName(LPCSTR name)
{
	return FindObjectByName(shared_str(name));
}

CObject* CObjectList::FindObjectByCLS_ID(CLASS_ID cls)
{
	{
		Objects::iterator O = std::find_if(objects_active.begin(), objects_active.end(), fClassEQ(cls));

		if (O != objects_active.end())
			return *O;
	}

	{
		Objects::iterator O = std::find_if(objects_sleeping.begin(), objects_sleeping.end(), fClassEQ(cls));

		if (O != objects_sleeping.end())
			return *O;
	}

	return NULL;
}


void CObjectList::o_remove(Objects&	v, CObject* O)
{
	Objects::iterator _i = std::find(v.begin(), v.end(), O);

	VERIFY(_i != v.end());

	v.erase(_i);
}

void CObjectList::o_activate(CObject* O)
{
	VERIFY(O && O->processing_enabled());

	o_remove(objects_sleeping, O);
	objects_active.push_back(O);

	O->MakeMeUpdatable();
}
void CObjectList::o_sleep(CObject* O)
{
	VERIFY(O && !O->processing_enabled());

	o_remove(objects_active, O);
	objects_sleeping.push_back(O);

	O->MakeMeUpdatable();
}

extern ENGINE_API BOOL logPerfomanceProblems_;

void CObjectList::SingleUpdate(CObject* O)
{
#ifdef MEASURE_UPDATES
	CTimer T; T.Start();
#endif


	R_ASSERT(O);
	R_ASSERT(O->SectionName());

	if (CurrentFrame() == O->dwFrame_UpdateCL)
	{
		return;
	}

	if (!O->processing_enabled())
	{
		return;
	}

	if (O->H_Parent())
		SingleUpdate(O->H_Parent());

	Device.Statistic->UpdateClient_updated++;

	O->dwFrame_UpdateCL = CurrentFrame();

	O->UpdateCL();

	VERIFY3(O->dbg_update_cl == CurrentFrame(), "Broken sequence of calls to 'UpdateCL'", *O->ObjectName());

	if (O->H_Parent() && (O->H_Parent()->getDestroy() || O->H_Root()->getDestroy()))
	{
		// Push to destroy-queue if it isn't here already
		Msg("! ERROR: incorrect destroy sequence for object[%d:%s], section[%s], parent[%d:%s]", O->ID(), *O->ObjectName(), *O->SectionName(), O->H_Parent()->ID(), *O->H_Parent()->ObjectName());
	}


#ifdef MEASURE_UPDATES
	float time = T.GetElapsed_sec();

	if (logPerfomanceProblems_ && time * 1000.f > 1.f)
		Msg("!Perfomance Warning: UpdateCL of %s took %f ms", O->ObjectNameStr(), time * 1000.f);
#endif
}

void CObjectList::ClearUpdatablesList(Objects& o)
{
	for (u32 _it = 0; _it < o.size(); _it++)
	{
		o[_it]->SetIsInUpdateList(FALSE);
	}

	o.clear_not_free();
}

void CObjectList::ObjectListUpdate(bool bForce, bool disable_objects_updateCL_optim)
{
	if (!Device.Paused() || bForce)
	{
		// Clients
		if (TimeDelta() > EPS_S || bForce)
		{
			//Clone the list, to allow corrupt-less adding of objects from secondary thread
			Device.Statistic->UpdateClient_updated = 0;

			protectUpdatables_.Enter();

			updatablesListCopy_ = GetUpdatablesList();

			ClearUpdatablesList(GetUpdatablesList());

			protectUpdatables_.Leave();

#if 0 // sholdn't be needed
			std::sort(updatablesListCopy_.begin(), updatablesListCopy_.end());
			updatablesListCopy_.erase(std::unique(updatablesListCopy_.begin(), updatablesListCopy_.end()), updatablesListCopy_.end());
#endif
			u32 nulled_count = CheckForNulled();

			if (nulled_count)
				Msg("!!! There were %u of nulled objects if the update queue", nulled_count);

			Objects* workload = 0;

			if (!disable_objects_updateCL_optim)
				workload = &updatablesListCopy_; // update only valuable objects
			else
				workload = &objects_active; // In some cases, engine should update all the objects

			Device.Statistic->UpdateClient.Begin();

			Device.Statistic->UpdateClient_actual_list = workload->size();
			Device.Statistic->UpdateClient_active = objects_active.size();
			Device.Statistic->UpdateClient_total = objects_active.size() + objects_sleeping.size();

			for (u32 i = 0; i < workload->size(); ++i)
			{
				CObject* O = workload->at(i);

				R_ASSERT(O);
				R_ASSERT(O->SectionName());

				SingleUpdate(O);
			}

			Device.Statistic->UpdateClient.End();
		}
	}
}

void CObjectList::ObjectListOnFrameEnd()
{
	for (u32 i = 0; i < frameEndObjectCalls.size(); i++)
		frameEndObjectCalls[i]();

	frameEndObjectCalls.clear();
}

void CObjectList::DestroyOldObjects(bool ignore_conds)
{
	g_pGamePersistent->ObjectPool.DeleteReadyObjects();

	// Destroy
	if (!destroy_queue.empty())
	{
		// Info
		for (Objects::iterator oit = objects_active.begin(); oit != objects_active.end(); oit++)
			for (int it = destroy_queue.size() - 1; it >= 0; it--)
			{
				(*oit)->RemoveLinksToCLObj(destroy_queue[it]);
			}

		for (Objects::iterator oit = objects_sleeping.begin(); oit != objects_sleeping.end(); oit++)
			for (int it = destroy_queue.size() - 1; it >= 0; it--)
				(*oit)->RemoveLinksToCLObj(destroy_queue[it]);

		for (int it = destroy_queue.size() - 1; it >= 0; it--)
			Sound->object_relcase(destroy_queue[it]);

		RELCASE_CALLBACK_VEC::iterator It = m_relcase_callbacks.begin();
		RELCASE_CALLBACK_VEC::iterator Ite = m_relcase_callbacks.end();

		for (; It != Ite; ++It)
		{
			VERIFY(*(*It).m_ID == (It - m_relcase_callbacks.begin()));

			Objects::iterator dIt = destroy_queue.begin();
			Objects::iterator dIte = destroy_queue.end();

			for (; dIt != dIte; ++dIt)
			{
				(*It).m_Callback(*dIt);
				g_hud->RemoveLinksToCLObj(*dIt);
			}
		}

		// Destroy
		for (int it = destroy_queue.size() - 1; it >= 0; it--)
		{
			CObject* O = destroy_queue[it];
#ifdef DEBUG
			if (debug_destroy)
				Msg("Destroying object[%x][%x] [%d][%s] frame[%d]", dynamic_cast<void*>(O), O, O->ID(), *O->ObjectName(), CurrentFrame());
#endif
			O->DestroyClientObj();

			Destroy(O, ignore_conds);
		}

		destroy_queue.clear();
	}
}

void CObjectList::net_Register(CObject* O)
{
	R_ASSERT(O);
	R_ASSERT(O->ID() < MAX_OBJ_IDS);

	map_NETID[O->ID()] = O;
}

void CObjectList::net_Unregister(CObject* O)
{
	if (O->ID() < MAX_OBJ_IDS)
		map_NETID[O->ID()] = NULL;
}

int	g_Dump_Export_Obj = 0;

u32	CObjectList::ExportObjectsDataToServer(NET_Packet* _Packet, u32 start, u32 max_object_size)
{
	if (g_Dump_Export_Obj)
		Msg("---- ExportDataToServer --- ");

	NET_Packet& Packet = *_Packet;

	u32 position;

	for (; start < objects_active.size() + objects_sleeping.size(); start++)
	{
		CObject* P = (start<objects_active.size()) ? objects_active[start] : objects_sleeping[start - objects_active.size()];

		if (P->NeedDataExport() && !P->getDestroy())
		{
			Packet.w_u16(u16(P->ID()));
			Packet.w_chunk_open8(position);

			P->ExportDataToServer(Packet);

#ifdef DEBUG
			u32 size = u32(Packet.w_tell() - position) - sizeof(u8);

			if (size >= 256)
			{
				Debug.fatal(DEBUG_INFO, "Object [%s][%d] exceed network-data limit\n size=%d, Pend=%d, Pstart=%d",
					*P->ObjectName(), P->ID(), size, Packet.w_tell(), position);
			}
#endif
			if (g_Dump_Export_Obj)
			{
				u32 sizE = u32(Packet.w_tell() - position) - sizeof(u8);
				Msg("* %s : %d", *(P->SectionName()), sizE);
			}

			Packet.w_chunk_close8(position);

			if (max_object_size >= (NET_PacketSizeLimit - Packet.w_tell()))
				break;
		}
	}

	if (g_Dump_Export_Obj)
		Msg("------------------- ");

	return start + 1;
}

void CObjectList::Load()
{
	R_ASSERT(objects_active.empty() && destroy_queue.empty() && objects_sleeping.empty());
}

void CObjectList::Unload()
{
	if (objects_sleeping.size() || objects_active.size())
		Msg("! objects-leaked: %d", objects_sleeping.size() + objects_active.size());

	// Destroy objects
	while (objects_sleeping.size())
	{
		CObject*	O = objects_sleeping.back();
		Msg("! [%x] s[%4d]-[%s]-[%s]", O, O->ID(), *O->SectionName(), *O->ObjectName());
		O->setDestroy(true);

#ifdef DEBUG
		if (debug_destroy)
			Msg("Destroying object [%d][%s]", O->ID(), *O->ObjectName());
#endif
		O->DestroyClientObj();

		Destroy(O, true);
	}

	while (objects_active.size())
	{
		CObject* O = objects_active.back();
		Msg("! [%x] a[%4d]-[%s]-[%s]", O, O->ID(), *O->SectionName(), *O->ObjectName());

		O->setDestroy(true);

#ifdef DEBUG
		if (debug_destroy)
			Msg("Destroying object [%d][%s]", O->ID(), *O->ObjectName());
#endif
		O->DestroyClientObj();

		Destroy(O, true);
	}
}

CObject* CObjectList::Create(LPCSTR	name)
{
	CObject* O = g_pGamePersistent->ObjectPool.create(name);

	objects_sleeping.push_back(O);

	return O;
}

void CObjectList::Destroy(CObject* O, bool ignore_conds)
{
	if (!O)
		return;

	net_Unregister(O);

	Objects& list = GetUpdatablesList();
	Objects::iterator _i0 = std::find(list.begin(), list.end(), O);

	if (_i0 != list.end())
	{
		list.erase(_i0);

		VERIFY(std::find(list.begin(), list.end(), O) == list.end());
	}

	// active/inactive
	Objects::iterator _i = std::find(objects_active.begin(), objects_active.end(), O);

	if (_i != objects_active.end())
	{
		objects_active.erase(_i);

		VERIFY(std::find(objects_active.begin(), objects_active.end(), O) == objects_active.end());

		VERIFY(std::find(objects_sleeping.begin(), objects_sleeping.end(), O) == objects_sleeping.end());
	}
	else
	{
		Objects::iterator _ii = std::find(objects_sleeping.begin(), objects_sleeping.end(), O);

		if (_ii != objects_sleeping.end())
		{
			objects_sleeping.erase(_ii);

			VERIFY(std::find(objects_sleeping.begin(), objects_sleeping.end(), O) == objects_sleeping.end());
		}
		else
			Debug.fatal(DEBUG_INFO, "! Unregistered object [%d][%s] being destroyed", O->ID(), O->ObjectName().c_str());
	}

	g_pGamePersistent->ObjectPool.destroy(O, ignore_conds);
}

void CObjectList::relcase_register(RELCASE_CALLBACK cb, int *ID)
{

#ifdef DEBUG
	RELCASE_CALLBACK_VEC::iterator It = std::find(m_relcase_callbacks.begin(), m_relcase_callbacks.end(), cb);

	VERIFY(It == m_relcase_callbacks.end());
#endif

	*ID = m_relcase_callbacks.size();

	m_relcase_callbacks.push_back(SRelcasePair(ID, cb));
}

struct SRemoveObj
{
	CObject* who_to_look_for;

	SRemoveObj(CObject* who)
	{
		who_to_look_for = who;
	}

	bool operator() (const CObject* object) const
	{
		if (object == who_to_look_for)
			return true;

		return false;
	}
};

struct SRemoveNulledObj
{
	bool operator() (const CObject* object) const
	{
		if (!object)
			return true;

		if (!object->SectionName())
			return true;

		return false;
	}
};

void CObjectList::RemoveFromUpdList(CObject* O)
{
	protectUpdatables_.Enter();

	Objects& upd_list = GetUpdatablesList();

#if 0
	u32 cur_size = upd_list.size();
#endif

	upd_list.erase(std::remove_if(upd_list.begin(), upd_list.end(), SRemoveObj(O)), upd_list.end());

#if 0
	u32 new_size = upd_list.size();

	Msg("RemoveFromUpdList: %u instances are removed", cur_size - new_size);
#endif

	protectUpdatables_.Leave();
}

u32 CObjectList::CheckForNulled()
{
	protectUpdatables_.Enter();

	Objects& upd_list = GetUpdatablesList();

	u32 cur_size = upd_list.size();
	upd_list.erase(std::remove_if(upd_list.begin(), upd_list.end(), SRemoveNulledObj()), upd_list.end());
	u32 new_size = upd_list.size();

	protectUpdatables_.Leave();

	return cur_size - new_size;
}

void CObjectList::relcase_unregister(int* ID)
{
	VERIFY(m_relcase_callbacks[*ID].m_ID == ID);

	m_relcase_callbacks[*ID] = m_relcase_callbacks.back();
	*m_relcase_callbacks.back().m_ID = *ID;

	m_relcase_callbacks.pop_back();
}

void CObjectList::dump_list(Objects& v, LPCSTR reason)
{
	Objects::iterator it = v.begin();
	Objects::iterator it_e = v.end();

	for (; it != it_e; ++it)
		Msg("Dump for %s: %x - name [%s] ID[%d] parent[%s] getDestroy()=[%s]", reason, (*it), (*it)->ObjectName().c_str(), (*it)->ID(), ((*it)->H_Parent()) ? (*it)->H_Parent()->ObjectName().c_str() : "", ((*it)->getDestroy()) ? "yes" : "no");
}

bool CObjectList::dump_all_objects()
{
	dump_list(destroy_queue, "destroy_queue");
	dump_list(objects_active, "objects_active");
	dump_list(objects_sleeping, "objects_sleeping");
	dump_list(GetUpdatablesList(), "updateCL list");

	return false;
}

void CObjectList::register_object_to_destroy(CObject *object_to_destroy)
{
	VERIFY(!registered_object_to_destroy(object_to_destroy));

	destroy_queue.push_back(object_to_destroy);

	Objects::iterator it = objects_active.begin();
	Objects::iterator it_e = objects_active.end();

	for (; it != it_e; ++it)
	{
		CObject* O = *it;

		if (!O->getDestroy() && O->H_Parent() == object_to_destroy)
		{
			Msg("setDestroy called, but not-destroyed child found parent[%d] child[%d]", object_to_destroy->ID(), O->ID(), CurrentFrame());

			O->setDestroy(TRUE);
		}
	}

	it = objects_sleeping.begin();
	it_e = objects_sleeping.end();

	for (; it != it_e; ++it)
	{
		CObject* O = *it;

		if (!O->getDestroy() && O->H_Parent() == object_to_destroy)
		{
			Msg("setDestroy called, but not-destroyed child found parent[%d] child[%d]", object_to_destroy->ID(), O->ID(), CurrentFrame());
			O->setDestroy(TRUE);
		}
	}
}

#ifdef DEBUG

bool CObjectList::registered_object_to_destroy(const CObject *object_to_destroy) const
{
	return(std::find(destroy_queue.begin(), destroy_queue.end(), object_to_destroy) != destroy_queue.end());
}

#endif