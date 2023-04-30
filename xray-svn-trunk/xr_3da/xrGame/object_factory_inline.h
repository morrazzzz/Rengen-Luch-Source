////////////////////////////////////////////////////////////////////////////
//	Module 		: object_factory_inline.h
//	Created 	: 27.05.2004
//  Modified 	: 27.05.2004
//	Author		: Dmitriy Iassenev
//	Description : Object factory inline functions
////////////////////////////////////////////////////////////////////////////

#ifndef object_factory_inlineH
#define object_factory_inlineH

#pragma once

IC	const CObjectFactory &object_factory()
{
	if (!g_object_factory) {
		g_object_factory		= xr_new <CObjectFactory>();
		g_object_factory->init	();
	}
	return						(*g_object_factory);
}

IC	bool CObjectFactory::CObjectItemPredicate::operator()	(const CObjectItemAbstract *item1, const CObjectItemAbstract *item2) const
{
	return				(item1->clsid() < item2->clsid());
}

IC	bool CObjectFactory::CObjectItemPredicate::operator()	(const CObjectItemAbstract *item, const CLASS_ID &clsid) const
{
	return				(item->clsid() < clsid);
}

IC	CObjectFactory::CObjectItemPredicateCLSID::CObjectItemPredicateCLSID	(const CLASS_ID &clsid) :
	m_clsid				(clsid)
{
}

IC	bool CObjectFactory::CObjectItemPredicateCLSID::operator()	(const CObjectItemAbstract *item) const
{
	return				(m_clsid == item->clsid());
}

IC	CObjectFactory::CObjectItemPredicateScript::CObjectItemPredicateScript	(const shared_str &script_clsid_name) :
	m_script_clsid_name	(script_clsid_name)
{
}

IC	bool CObjectFactory::CObjectItemPredicateScript::operator()	(const CObjectItemAbstract *item) const
{
	return				(m_script_clsid_name == item->script_clsid());
}

IC	const CObjectFactory::OBJECT_ITEM_STORAGE &CObjectFactory::clsids	() const
{
	return				(m_clsids);
}

#ifndef NO_XR_GAME
IC	const CObjectItemAbstract &CObjectFactory::item	(const CLASS_ID &clsid) const
{
	actualize			();
	const_iterator		I = std::lower_bound(clsids().begin(),clsids().end(),clsid,CObjectItemPredicate());

	string16 class_to_text;
	CLSID2TEXT(clsid, class_to_text);
	VERIFY2((I != clsids().end()) && ((*I)->clsid() == clsid), make_string("%s", class_to_text));

	return				(**I);
}
#else

IC	const CObjectItemAbstract *CObjectFactory::item(const CLASS_ID &clsid, bool no_assert) const
{
	actualize();

	const_iterator I = std::lower_bound(clsids().begin(),clsids().end(),clsid,CObjectItemPredicate());

	if ((I == clsids().end()) || ((*I)->clsid() != clsid))
	{
		string256 class_name;

		CLSID2TEXT(clsid, class_name);

		Msg(make_string("Class %s is not registered in Server. Check if Engine or Scripted class register handlers are loaded", class_name).c_str());

		return(0);
	}

	return(*I);
}
#endif

IC	void CObjectFactory::add(CObjectItemAbstract *item)
{
	const_iterator		I;

	I = std::find_if(clsids().begin(),clsids().end(),CObjectItemPredicateCLSID(item->clsid()));

	if (I != clsids().end()) // Crash debug info
	{
		Msg("List of already registered classes:");

		string16 class_to_text;
		for (auto iter = clsids().begin(); iter != clsids().end(); ++iter)
		{

			CLSID2TEXT((*iter)->clsid(), class_to_text);
			Msg("Class to Text [%s]; Script Class [%s]", class_to_text, (*iter)->script_clsid().c_str());
		}

		Msg(LINE_SPACER);
		Msg("Prepare your ass for bom-bom...");

		CLSID2TEXT(item->clsid(), class_to_text);
		Msg("Registering class failed: ClassID to text = [%s]; Script Class = [%s]", class_to_text, item->script_clsid().c_str());

		R_ASSERT2(I == clsids().end(), make_string("Class to Text = [%s], Script Class = [%s]. Looks like a dublicate of already registered class being registered", class_to_text, item->script_clsid().c_str()));
	}

	
#ifndef NO_XR_GAME
	I = std::find_if(clsids().begin(),clsids().end(),CObjectItemPredicateScript(item->script_clsid()));

	if (I != clsids().end()) // Crash debug info
	{
		Msg("List of already registered classes:");

		string16 class_to_text;
		for (auto iter = clsids().begin(); iter != clsids().end(); ++iter)
		{

			CLSID2TEXT((*iter)->clsid(), class_to_text);
			Msg("Class to Text [%s]; Script Class [%s]", class_to_text, (*iter)->script_clsid().c_str());
		}

		Msg("Prepare your ass for bom-bom...");

		CLSID2TEXT(item->clsid(), class_to_text);
		Msg("Registering class failed: ClassID to text = [%s]; Script Class = [%s]", class_to_text, item->script_clsid().c_str());

		R_ASSERT2(I == clsids().end(), make_string("Class to Text = [%s], Class ID u64 = [%s], Script Class = [%s]. Looks like a dublicate of already registered class being registered", class_to_text, item->clsid(), item->script_clsid().c_str()));
	}
#endif
	
	m_actual			= false;
	m_clsids.push_back	(item);
}

IC	int	CObjectFactory::script_clsid	(const CLASS_ID &clsid) const
{
	actualize			();
	const_iterator		I = std::lower_bound(clsids().begin(),clsids().end(),clsid,CObjectItemPredicate());
	VERIFY				((I != clsids().end()) && ((*I)->clsid() == clsid));
	return				(int(I - clsids().begin()));
}

#ifndef NO_XR_GAME
IC	CObjectFactory::CLIENT_BASE_CLASS *CObjectFactory::client_object	(const CLASS_ID &clsid) const
{
	return				(item(clsid).client_object());
}

IC	CObjectFactory::SERVER_BASE_CLASS *CObjectFactory::server_object	(const CLASS_ID &clsid, LPCSTR section) const
{
begin:

	SERVER_BASE_CLASS* object	= item(clsid).server_object(section);

	if (!object)
	{
		string16 class_to_text;
		CLSID2TEXT(clsid, class_to_text);

		R_ASSERT2(object, make_string("%s %s", class_to_text, section));

		goto begin;
	}

	return						object;
}
#else
IC	CObjectFactory::SERVER_BASE_CLASS *CObjectFactory::server_object	(const CLASS_ID &clsid, LPCSTR section) const
{
	const CObjectItemAbstract	*object = item(clsid, false);
	return				(object ? object->server_object(section) : 0);
}
#endif

IC	void CObjectFactory::actualize										() const
{
	if (m_actual)
		return;

	m_actual			= true;
	std::sort			(m_clsids.begin(),m_clsids.end(),CObjectItemPredicate());
}

#endif