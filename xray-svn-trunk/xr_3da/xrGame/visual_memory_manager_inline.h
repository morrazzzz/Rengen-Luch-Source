
//	Created 	: 02.10.2001
//	Author		: Dmitriy Iassenev

#pragma once

IC const CVisualMemoryManager::VISIBLES& CVisualMemoryManager::GetMemorizedObjects() const
{
	return(visibleObjects_);
}

IC CVisualMemoryManager::VISIBLES* CVisualMemoryManager::GetMemorizedObjectsP()
{
	return(&visibleObjects_);
}

IC const CVisualMemoryManager::RAW_VISIBLES &CVisualMemoryManager::raw_objects() const
{
	return(m_visible_objects);
}

IC const CVisualMemoryManager::NOT_YET_VISIBLES &CVisualMemoryManager::not_yet_visible_objects() const
{
	return(m_not_yet_visible_objects);
}

IC void CVisualMemoryManager::set_squad_objects(VISIBLES *squad_objects)
{
	if (squad_objects)
		visibleObjects_ = *squad_objects;
	else
	{
		visibleObjects_.clear();
		m_not_yet_visible_objects.clear();
	}
}

IC bool CVisualMemoryManager::enabled() const
{
	return							(m_enabled);
}

IC void CVisualMemoryManager::enable(bool value)
{
	m_enabled = value;
}

IC float CVisualMemoryManager::GetMinViewDistance() const
{
	if (m_stalker)
		return current_state().m_min_view_distance * RankDependancy().minViewDistanceK_;
	else
		return current_state().m_min_view_distance;
}

IC float CVisualMemoryManager::GetMaxViewDistance() const
{
	if (m_stalker)
		return current_state().m_max_view_distance * RankDependancy().maxViewDistanceK_;
	else
		return current_state().m_max_view_distance;
}

IC float CVisualMemoryManager::GetVisibilityThreshold() const
{
	if (m_stalker)
		return current_state().m_visibility_threshold * RankDependancy().visibilityThresholdK_;
	else
		return current_state().m_visibility_threshold;
}

IC float CVisualMemoryManager::GetAlwaysVisibleDistance() const
{
	if (m_stalker)
		return current_state().m_always_visible_distance * RankDependancy().alwaysVisibleDistanceK_;
	else
		return current_state().m_always_visible_distance;
}

IC float CVisualMemoryManager::GetTimeQuant() const
{
	if (m_stalker)
		return current_state().m_time_quant * RankDependancy().timeQuantK_;
	else
		return current_state().m_time_quant;
}

IC float CVisualMemoryManager::GetDecreaseValue() const
{
	if (m_stalker)
		return current_state().m_decrease_value * RankDependancy().decreaseValueK_;
	else
		return current_state().m_decrease_value;
}

IC float CVisualMemoryManager::GetVelocityFactor() const
{
	if (m_stalker)
		return current_state().m_velocity_factor * RankDependancy().velocityFactorK_;
	else
		return current_state().m_velocity_factor;
}

IC float CVisualMemoryManager::GetTransparencyThreshold() const
{
	if (m_stalker)
		return current_state().m_transparency_threshold * RankDependancy().transparencyThresholdK_;
	else
		return current_state().m_transparency_threshold;
}

IC float CVisualMemoryManager::GetLuminocityFactor() const
{
	if (m_stalker)
		return current_state().m_luminocity_factor * RankDependancy().luminocityFactorK_;
	else
		return current_state().m_luminocity_factor;
}

IC u32	CVisualMemoryManager::GetStillVisibleTime() const
{
	if (m_stalker)
		return current_state().m_still_visible_time * (u32)RankDependancy().stillVisibleTimeK_;
	else
		return current_state().m_still_visible_time;
}

IC float CVisualMemoryManager::GetRankEyeFovK() const
{
	return RankDependancy().eyeFovK_;
}

IC float CVisualMemoryManager::GetRankEyeRangeK() const
{
	return RankDependancy().eyeRangeK_;
}