
//	Created 	: 02.10.2001
//	Author		: Dmitriy Iassenev

#pragma once

IC CSoundMemoryManager::CSoundMemoryManager(CCustomMonster *object, CAI_Stalker *stalker, CSound_UserDataVisitor *visitor)
{
	VERIFY						(object);

	m_object					= object;

	VERIFY						(visitor);

	m_visitor					= visitor;
	m_stalker					= stalker;
	m_max_sound_count			= 0;

#ifdef USE_SELECTED_SOUND
	m_selected_sound			= 0;
#endif
}

IC const CSoundMemoryManager::SOUNDS &CSoundMemoryManager::GetMemorizedSounds() const
{
	return (soundMemory_);
}

IC CSoundMemoryManager::SOUNDS* CSoundMemoryManager::GetMemorizedSoundsP()
{
	return (&soundMemory_);
}

IC const SSoundParametersKoef& CSoundMemoryManager::HearingRankDependancy() const
{
	return soundRankCoef_;
}

IC void CSoundMemoryManager::priority(const ESoundTypes &sound_type, u32 priority)
{
	PRIORITIES::const_iterator	I = m_priorities.find(sound_type);

	VERIFY(m_priorities.end() == I);

	m_priorities.insert(std::make_pair(sound_type,priority));
}

#ifdef USE_SELECTED_SOUND
IC	const MemorySpace::CSoundObject *CSoundMemoryManager::sound		() const
{
	return(m_selected_sound);
}
#endif

IC void CSoundMemoryManager::set_squad_objects(SOUNDS *squad_objects)
{
	soundMemory_ = *squad_objects;
}

IC void CSoundMemoryManager::set_threshold(float threshold)
{
	m_sound_threshold = threshold;

	VERIFY(_valid(m_sound_threshold));
}

IC void CSoundMemoryManager::restore_threshold()
{
	m_sound_threshold = m_min_sound_threshold;
}

IC float CSoundMemoryManager::GetSoundThreshold() const
{
	return m_sound_threshold * HearingRankDependancy().soundThresholdK_;
}

IC float CSoundMemoryManager::GetWeaponSoundValue() const
{
	return m_weapon_factor * HearingRankDependancy().soundPowerK_;
}

IC float CSoundMemoryManager::GetItemSoundValue() const
{
	return m_item_factor * HearingRankDependancy().soundPowerK_;
}

IC float CSoundMemoryManager::GetNpcSoundValue() const
{
	return m_npc_factor * HearingRankDependancy().soundPowerK_;
}

IC float CSoundMemoryManager::GetAnomalySoundValue() const
{
	return m_anomaly_factor * HearingRankDependancy().soundPowerK_;
}

IC float CSoundMemoryManager::GetWorldSoundValue() const
{
	return m_world_factor * HearingRankDependancy().soundPowerK_;
}
