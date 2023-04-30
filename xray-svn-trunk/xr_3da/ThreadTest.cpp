#include "stdafx.h"
#include "threadtest.h"

#include <thread>

Mutex TestMutex;

#pragma optimize( "", off ) //Critical here. Otherwise it gets optimized and does not work

__declspec(noinline) void mt_MeasureCPU(){
	TestMutex.lock();

//	FPU::m64r();

	//Initialize almost maximum values
	s32 ii32 = 214748364;
	s64 ii64 = 922337203685477580;
	f32 ff32 = powf(33.3332f, 30.0f);
	f64 ff64 = f64(powl(17.777777777772, 300.0));

	f64 felapsedtimeIs = 0.0;
	f64 felapsedtimeIs64 = 0.0;
	f64 felapsedtimeFs = 0.0;
	f64 felapsedtimeDs = 0.0;

	checkFreezing_ = false;

	//Warm up CPU clock
	u64 y = 5859375;
	while (y > 0)
	{
		ii32 = ((((ii32 + ii32) / ii32) * ii32) / 2);
		ii64 = ((((ii64 + ii64) / ii64) * ii64) / 2);
		ff32 = ((((ff32 + ff32) / ff32) * ff32) / 2);
		ff64 = ((((ff64 + ff64) / ff64) * ff64) / 2);
		y--;
	}

	Msg("-- CPU warmup finished. Starting actual test", ii32, ii64, ff32, ff64);

	//Actual test
	CTimer MeasureTimer;

	u8 ii = 16;
	while (ii > 0) //To smooth the floatin CPU clock scaler
	{
		//32-bit integers
		MeasureTimer.Start();
		u64 i = 65535 * 64; //otherwise, to fast
		while (i > 0)
		{
			ii32 = ((((ii32 + ii32) / ii32) * ii32) / 2);
			ii32 = ((((ii32 + ii32) / ii32) * ii32) / 2);
			ii32 = ((((ii32 + ii32) / ii32) * ii32) / 2);
			i--;
		}

		felapsedtimeIs += MeasureTimer.GetElapsed_sec();

		//64-bit integers
		MeasureTimer.Start();
		i = 65535 * 16;
		while (i > 0)
		{
			ii64 = ((((ii64 + ii64) / ii64) * ii64) / 2);
			ii64 = ((((ii64 + ii64) / ii64) * ii64) / 2);
			ii64 = ((((ii64 + ii64) / ii64) * ii64) / 2);
			i--;
		}

		felapsedtimeIs64 += MeasureTimer.GetElapsed_sec();

		//32-bit floats
		MeasureTimer.Start();
		i = 65535 * 64; //otherwise, to fast
		while (i > 0)
		{
			ff32 = ((((ff32 + ff32) / ff32) * ff32) / 2);
			ff32 = ((((ff32 + ff32) / ff32) * ff32) / 2);
			ff32 = ((((ff32 + ff32) / ff32) * ff32) / 2);
			i--;
		}

		felapsedtimeFs += MeasureTimer.GetElapsed_sec();

		//64-bit floats
		MeasureTimer.Start();
		i = 65535 * 4;
		while (i > 0)
		{
			ff64 = ((((ff64 + ff64) / ff64) * ff64) / 2);
			ff64 = ((((ff64 + ff64) / ff64) * ff64) / 2);
			ff64 = ((((ff64 + ff64) / ff64) * ff64) / 2);
			i--;
		}

		felapsedtimeDs += MeasureTimer.GetElapsed_sec();
		ii--;
	}

	f64 scoreInts = 65535.0 / felapsedtimeIs;
	f64 scoreInts64 = 65535.0 / felapsedtimeIs64;
	f64 scoreFloats = 65535.0 / felapsedtimeFs;
	f64 scoreDoubles = 65535.0 / felapsedtimeDs;

	f64 ftimetotal = felapsedtimeIs + felapsedtimeIs64 + felapsedtimeFs + felapsedtimeDs;

	f64 scoretotal = scoreInts + scoreInts64 + scoreFloats + scoreDoubles;

	Msg("-- Finished Test");
	Msg("-- Test Time [%fs]; 64 loops of Int32 [%fs]; 16 loops of Int64 [%fs]; 64 loops of F32 [%fs]; 4 loops of F64 [%fs]", ftimetotal, felapsedtimeIs, felapsedtimeIs64, felapsedtimeFs, felapsedtimeDs);
	Msg("-- Single Thread Score - Total [%i]; Int32 [%i]; Int64 [%i]; F32 [%i]; F64 [%i]", (int)scoretotal, (int)scoreInts, (int)scoreInts64, (int)scoreFloats, (int)scoreDoubles);

	checkFreezing_ = true;
	//::m24r();
	TestMutex.unlock();
}


void RunSingleThreadTest()
{
	Msg("-- Running test");

	std::thread t2(mt_MeasureCPU);
	t2.detach();

}
#pragma optimize( "", on )