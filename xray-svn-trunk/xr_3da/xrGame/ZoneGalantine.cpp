#include "stdafx.h"
#include "zonegalantine.h"

CZoneGalantine::CZoneGalantine(void) 
{
	m_fActorBlowoutRadiusPercent=0.5f;
}

CZoneGalantine::~CZoneGalantine(void) 
{
}

BOOL CZoneGalantine::SpawnAndImportSOData(CSE_Abstract* data_containing_so)
{
	return inherited::SpawnAndImportSOData(data_containing_so);
}