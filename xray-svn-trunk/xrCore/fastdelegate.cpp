#include "stdafx.h"
#include "fastdelegate.h"

struct SRemoveDelegate0
{
	fastdelegate::FastDelegate0<>* d;

	SRemoveDelegate0(fastdelegate::FastDelegate0<>* delegate)
	{
		d = delegate;
	}

	bool operator() (const fastdelegate::FastDelegate0<>& delegate) const
	{
		if (!delegate)
			return true;

		if (delegate == *d)
			return true;

		return false;
	}
};


XRCORE_API void fastdelegate::RemoveFastDelegate(xr_vector<fastdelegate::FastDelegate0<>>& list, fastdelegate::FastDelegate0<> delegate)
{
	list.erase(std::remove_if(list.begin(), list.end(), SRemoveDelegate0(&delegate)), list.end());
};
