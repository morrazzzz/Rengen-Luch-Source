////////////////////////////////////////////////////////////////////////////
//	Module 		: CameraRecoil.h
//	Created 	: 26.05.2008
//	Author		: Evgeniy Sokolov
//	Description : Camera Recoil struct
////////////////////////////////////////////////////////////////////////////

#pragma once

struct CameraRecoil
{
public:
	float					camRelaxSpeed;
	float					camRelaxSpeed_AI;
	float					camDispersion;
	float					camDispersionInc;
	float					camDispersionFrac;
	float					camMaxAngleVert; // new
	float					camMaxAngleHorz;
	float					camStepAngleHorz;
	bool					camReturnMode; // new
	bool					camStopReturn; // new

	CameraRecoil() {};

	CameraRecoil(const CameraRecoil& clone) { Clone(clone); };

	IC void Clone(const CameraRecoil& clone)
	{
		// *this = clone;
		camRelaxSpeed = clone.camRelaxSpeed;
		camRelaxSpeed_AI = clone.camRelaxSpeed_AI;
		camDispersion = clone.camDispersion;
		camDispersionInc = clone.camDispersionInc;
		camDispersionFrac = clone.camDispersionFrac;
		camMaxAngleVert = clone.camMaxAngleVert;
		camMaxAngleHorz = clone.camMaxAngleHorz;
		camStepAngleHorz = clone.camStepAngleHorz;

		camReturnMode = clone.camReturnMode;
		camStopReturn = clone.camStopReturn;

		VERIFY(!fis_zero(camRelaxSpeed));
		VERIFY(!fis_zero(camRelaxSpeed_AI));
		VERIFY(!fis_zero(camMaxAngleVert));
		VERIFY(!fis_zero(camMaxAngleHorz));
	};
};
