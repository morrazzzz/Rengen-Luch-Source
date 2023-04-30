////////////////////////////////////////////////////////////////////////////
//	Module 		: memory_manager.h
//	Created 	: 02.10.2001
//  Modified 	: 19.11.2003
//	Author		: Dmitriy Iassenev
//	Description : Memory manager
////////////////////////////////////////////////////////////////////////////

#pragma once

class CVisualMemoryManager;
class CSoundMemoryManager;
class CHitMemoryManager;
class CEnemyManager;
class CItemManager;
class CDangerManager;
class CCustomMonster;
class CAI_Stalker;
class CEntityAlive;
class CSound_UserDataVisitor;

namespace MemorySpace {
	struct CMemoryInfo;
};

class CMemoryManager {
public:
	typedef MemorySpace::CMemoryInfo		CMemoryInfo;

protected:
	CVisualMemoryManager		*m_visual;
	CSoundMemoryManager			*m_sound;
	CHitMemoryManager			*m_hit;
	CEnemyManager				*m_enemy;
	CItemManager				*m_item;
	CDangerManager				*m_danger;

	// Values to somehow controll the npc behavior, when a group member is fighting on the other side of map and all npc from the gorup react on it
	float						ignoreGroupVMemoryDist_;
	float						ignoreGroupSMemoryDist_;
	float						ignoreGroupHMemoryDist_;

	int							playSearchSndChance_;

	u32							nextFrameCheckOld_;
protected:
	CCustomMonster				*m_object;
	CAI_Stalker					*m_stalker;

private:
			void				update_enemies				(const bool registered_in_combat);

protected:
			void				update_v						(bool add_enemies);
			void				update_s						(bool add_enemies);
			void				update_h						(bool add_enemies);

public:
								CMemoryManager				(CEntityAlive *entity_alive, CSound_UserDataVisitor *visitor);
	virtual						~CMemoryManager				();
	virtual	void				LoadCfg						(LPCSTR section);
	virtual	void				reinit						();
	virtual	void				reload						(LPCSTR section);
	virtual	void				update						(float time_delta);
			void				remove_links				(CObject *object);
			void				remove_links_from_sorted	(CObject *object);
	virtual void				on_restrictions_change		();
public:
			void				enable						(const CObject *object, bool enable);
			CMemoryInfo			memory						(const CObject *object) const;
			u32					memory_time					(const CObject *object) const;
			Fvector				memory_position				(const CObject *object) const;
			void				make_object_visible_somewhen(const CEntityAlive *enemy);

			void				EraseOldAndInvalidMem		();
public:
	template <typename T, typename _predicate>
	IC		void				fill_enemies				(const xr_vector<T> &objects, const _predicate &predicate) const;
	template <typename _predicate>
	IC		void				fill_enemies				(const _predicate &predicate) const;

public:
	IC		CVisualMemoryManager&VisualMManager				() const;
	IC		CSoundMemoryManager	&SoundMManager				() const;
	IC		CHitMemoryManager	&HitMManager				() const;
	IC		CEnemyManager		&EnemyManager				() const;
	IC		CItemManager		&ItemManager				() const;
	IC		CDangerManager		&DangerManager				() const;
	IC		CCustomMonster		&object						() const;
	IC		CAI_Stalker			&stalker					() const;

public:
			void				save						(NET_Packet &packet);
			void				load						(IReader &packet);
			void xr_stdcall		on_requested_spawn			(CObject *object);
};

#include "memory_manager_inline.h"