#pragma once

// Хранилище внешних шейдерных параметров, которые читаются в Blender_Recorder_StandartBinding.cpp
class WeaponSVPParamsShExport //--#SM+#--
{
public:
	Fvector4 svpParams;
	Fvector4 commonVpParams;

	WeaponSVPParamsShExport()
	{
		svpParams.set(0.f, 0.f, 0.f, 0.f);
		commonVpParams.set(0.f, 0.f, 0.f, 0.f);
	}
};