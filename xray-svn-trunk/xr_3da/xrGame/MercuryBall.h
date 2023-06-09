///////////////////////////////////////////////////////////////
// MercuryBall.h
// MercuryBall - �������������� � ������������ ���
// �������������� � ����� �� �����
///////////////////////////////////////////////////////////////

#pragma once
#include "artifact.h"

class CMercuryBall : public CArtefact 
{
private:
	typedef CArtefact inherited;
public:
	CMercuryBall(void);
	virtual ~CMercuryBall(void);

	virtual void LoadCfg				(LPCSTR section);
protected:
	virtual void	UpdateCLChild	();

	//����� ���������� ���������� ��������� ����
	ALife::_TIME_ID m_timeLastUpdate;
	//����� ����� ���������
	ALife::_TIME_ID m_timeToUpdate;

	//�������� ��������� ������� ����
	float m_fImpulseMin;
	float m_fImpulseMax;
};

/*

#pragma once
#include "gameobject.h"
#include "../../xrphysics/PhysicsShell.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// ������� ���
// ���������� ����� �������, �������� �������, ����� ���� ����������.
// ����:  �� 50 �� 200 ������, � ����������� �� ������� 
// ���������: ������� ���������� �����������, ������� ������ � ���������� ����������,
// �������� � ������� R1.
class CMercuryBall : public CGameObject {
typedef	CGameObject	inherited;
public:
	CMercuryBall(void);
	virtual ~CMercuryBall(void);

	virtual void AfterAttachToParent();
	virtual void BeforeDetachFromParent(bool just_before_destroy);

	
	virtual BOOL			SpawnAndImportSOData			(CSE_Abstract* data_containing_so);
};
*/