////////////////////////////////////////////////////////////////////////////
//	Module 		: memory_manager_inline.h
//	Created 	: 02.10.2001
//  Modified 	: 19.11.2003
//	Author		: Dmitriy Iassenev
//	Description : Memory manager inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

template <typename T, typename _predicate>
IC	void CMemoryManager::fill_enemies	(const xr_vector<T> &objects, const _predicate &predicate) const
{
	xr_vector<T>::const_iterator	I = objects.begin();
	xr_vector<T>::const_iterator	E = objects.end();

	for ( ; I != E; ++I)
	{
		if (!(*I).m_enabled)
			continue;

		if ((*I).m_level_time + VisualMManager().forgetVisTime_ < EngineTimeU() || (*I).m_level_time > u32(-1) - 500000) // An attempt to clear grouped npcs endless danger
			continue;

		//Msg("Filling enemy lt %u, et %u name %s", (*I).m_level_time, EngineTimeU(), (*I).m_object->ObjectName().c_str());

		const CEntityAlive	*_enemy = smart_cast<const CEntityAlive*>((*I).m_object);

		if (_enemy && EnemyManager().useful(_enemy))
			predicate		(_enemy);
	}
}

template <typename _predicate>
IC	void CMemoryManager::fill_enemies	(const _predicate &predicate) const
{
	fill_enemies(VisualMManager().GetMemorizedObjects(), predicate);
	//fill_enemies(sound().GetMemorizedSounds(), predicate);
	//fill_enemies(hit().GetMemorizedHits(), predicate);
}

IC	CVisualMemoryManager	&CMemoryManager::VisualMManager() const
{
	VERIFY					(m_visual);
	return					(*m_visual);
}

IC	CSoundMemoryManager		&CMemoryManager::SoundMManager() const
{
	VERIFY					(m_sound);
	return					(*m_sound);
}

IC	CHitMemoryManager		&CMemoryManager::HitMManager() const
{
	VERIFY					(m_hit);
	return					(*m_hit);
}

IC	CEnemyManager			&CMemoryManager::EnemyManager() const
{
	VERIFY					(m_enemy);
	return					(*m_enemy);
}

IC	CItemManager			&CMemoryManager::ItemManager() const
{
	VERIFY					(m_item);
	return					(*m_item);
}

IC	CDangerManager			&CMemoryManager::DangerManager() const
{
	VERIFY					(m_danger);
	return					(*m_danger);
}

IC	CCustomMonster &CMemoryManager::object				() const
{
	VERIFY					(m_object);
	return					(*m_object);
}

IC	CAI_Stalker	&CMemoryManager::stalker				() const
{
	VERIFY					(m_stalker);
	return					(*m_stalker);
}