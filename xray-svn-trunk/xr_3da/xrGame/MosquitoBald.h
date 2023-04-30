#pragma once

#include "customzone.h"
#include "script_export_space.h"

class CMosquitoBald : public CCustomZone
{
private:
	typedef	CCustomZone	inherited;
public:
	CMosquitoBald(void);
	virtual ~CMosquitoBald(void);

	virtual void LoadCfg(LPCSTR section);
	virtual void Postprocess(f32 val);
	virtual bool EnableEffector() {return true;}

	virtual void UpdateSecondaryHit();

	virtual void Affect(SZoneObjectInfo* O);

protected:
	virtual bool BlowoutState();
			bool ShouldIgnoreObject(CGameObject*) override;

	//��� ���� ����� blowout ��������� ���� ���
	//����� ���� ��� ���� ������������ � ������ ���������
	bool m_bLastBlowoutUpdate;
	bool m_killCarEngine;
	float m_hitKoefCar;

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CMosquitoBald)
#undef script_type_list
#define script_type_list save_type_list(CMosquitoBald)