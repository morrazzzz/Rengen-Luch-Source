///////////////////////////////////////////////////////////////
// GraviArtifact.cpp
// GraviArtefact - �������������� ��������, ������� �� �����
// � ����������� ����� ��� ������
///////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "GraviArtifact.h"
#include "../../xrphysics/PhysicsShell.h"
#include "level.h"
#include "../../xrphysics/iphworld.h"

#define CHOOSE_MAX(x,inst_x,y,inst_y,z,inst_z)\
	if(x>y)\
		if(x>z){inst_x;}\
		else{inst_z;}\
	else\
		if(y>z){inst_y;}\
		else{inst_z;}


CGraviArtefact::CGraviArtefact(void) 
{
	shedule.t_min = 20;
	shedule.t_max = 50;
	
	m_fJumpHeight = 0;
	m_fEnergy = 1.f;
}

CGraviArtefact::~CGraviArtefact(void) 
{
}

void CGraviArtefact::LoadCfg(LPCSTR section) 
{
	inherited::LoadCfg(section);

	if(pSettings->line_exist(section, "jump_height")) m_fJumpHeight = pSettings->r_float(section,"jump_height");
//	m_fEnergy = pSettings->r_float(section,"energy");
}



void CGraviArtefact::UpdateCLChild() 
{

	VERIFY(!physics_world()->Processing());
	if (getVisible() && m_pPhysicsShell) {
		if (m_fJumpHeight) {
			Fvector dir; 
			dir.set(0, -1.f, 0);
			collide::rq_result RQ;
			
			//��������� ������ ���������
			if(Level().ObjectSpace.RayPick(Position(), dir, m_fJumpHeight, collide::rqtBoth, RQ, this)) 
			{
				dir.y = 1.f; 
				m_pPhysicsShell->applyImpulse(dir, 
											  30.f * TimeDelta() * 
											  m_pPhysicsShell->getMass());
			}
		}
	} else 
		if(H_Parent()) 
		{
			XFORM().set(H_Parent()->XFORM());
		};
}