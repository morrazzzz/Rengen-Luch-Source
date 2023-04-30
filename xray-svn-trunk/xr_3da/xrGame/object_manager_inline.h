////////////////////////////////////////////////////////////////////////////
//	Module 		: object_manager.h
//	Created 	: 30.12.2003
//  Modified 	: 30.12.2003
//	Author		: Dmitriy Iassenev
//	Description : Object manager
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION template <\
	typename T\
>

#define CAbstractObjectManager CObjectManager<T>

TEMPLATE_SPECIALIZATION
CAbstractObjectManager::CObjectManager()
{
//	objectsArray_.reserve		(100);
}

TEMPLATE_SPECIALIZATION
CAbstractObjectManager::~CObjectManager()
{
}

TEMPLATE_SPECIALIZATION
void CAbstractObjectManager::LoadCfg(LPCSTR section)
{
}

TEMPLATE_SPECIALIZATION
void CAbstractObjectManager::reinit()
{
	objectsArray_.clear();

	m_selected = 0;
}

TEMPLATE_SPECIALIZATION
void CAbstractObjectManager::reload(LPCSTR section)
{
}

TEMPLATE_SPECIALIZATION
void CAbstractObjectManager::update()
{
	float					result = flt_max;
	m_selected				= 0;

	OBJECTS::const_iterator	I = objectsArray_.begin();
	OBJECTS::const_iterator	E = objectsArray_.end();

	for ( ; I != E; ++I)
	{
		float value = do_evaluate(*I);

		if (result > value)
		{
			result			= value;
			m_selected		= *I;
		}
	}
}

TEMPLATE_SPECIALIZATION
float CAbstractObjectManager::do_evaluate(T *object) const
{
	return (0.f);
}

TEMPLATE_SPECIALIZATION
bool CAbstractObjectManager::is_useful(T *object) const
{
	const ISpatial* self = (const ISpatial*)(object);

	if (!self)
		return (false);

	if ((object->spatial.s_type & STYPE_VISIBLEFORAI) != STYPE_VISIBLEFORAI)
		return (false);

	return (true);
}

TEMPLATE_SPECIALIZATION
bool CAbstractObjectManager::add(T *object)
{
	if (!is_useful(object))
		return (false);

	OBJECTS::const_iterator	I = std::find(objectsArray_.begin(), objectsArray_.end(), object);

	if (objectsArray_.end() == I)
	{
		objectsArray_.push_back(object);

		return (true);
	}

	return (true);
}

TEMPLATE_SPECIALIZATION
IC	T *CAbstractObjectManager::selected() const
{
	return (m_selected);
}

TEMPLATE_SPECIALIZATION
void CAbstractObjectManager::reset()
{
	objectsArray_.clear();
}

TEMPLATE_SPECIALIZATION
IC	const typename CAbstractObjectManager::OBJECTS &CAbstractObjectManager::objects() const
{
	return (objectsArray_);
}

#undef TEMPLATE_SPECIALIZATION
#undef CAbstractObjectManager