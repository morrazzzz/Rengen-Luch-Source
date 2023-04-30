///////////////////////////////////////////////////////////////
// ElectricBall.cpp
// ElectricBall - артефакт электрический шар
///////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ElectricBall.h"


CElectricBall::CElectricBall(void) 
{
}

CElectricBall::~CElectricBall(void) 
{
}

void CElectricBall::LoadCfg(LPCSTR section) 
{
	inherited::LoadCfg(section);
}

void CElectricBall::UpdateCLChild	()
{
	inherited::UpdateCLChild();

	if(H_Parent()) XFORM().set(H_Parent()->XFORM());
};