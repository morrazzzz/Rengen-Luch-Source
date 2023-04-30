#include "stdafx.h"
#include "physicsshell.h"
#include "phinterpolation.h"
#include "phobject.h"
#include "phworld.h"
#include "phshell.h"

void CPHShell::ExportDataToServer(NET_Packet& P)
{
	ELEMENT_I i=elements.begin(),e=elements.end();
	for(;i!=e;++i)
	{
		(*i)->ExportDataToServer(P);
	}	
}