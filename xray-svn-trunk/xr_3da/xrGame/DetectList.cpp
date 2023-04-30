#include "stdafx.h"
#include "DetectList.h"
#include "CustomZone.h"

ITEM_INFO::ITEM_INFO()
{
	curr_ref = NULL;
}

ITEM_INFO::~ITEM_INFO()
{
}

BOOL CZoneList::feel_touch_contact(CObject* O)
{
	TypesMapIt it = m_TypesMap.find(O->SectionName());
	bool res = (it != m_TypesMap.end());

	CCustomZone *pZone = smart_cast<CCustomZone*>(O);
	if (pZone && !pZone->IsEnabled())
	{
		res = false;
	}
	return res;
}

CZoneList::CZoneList()
{
}

CZoneList::~CZoneList()
{
	clear();
	destroy();
}