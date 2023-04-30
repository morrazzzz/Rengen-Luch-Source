#include "stdafx.h"
#include "xr_time.h"
#include "ui/UIInventoryUtilities.h"
#include "date_time.h"
#include "string_table.h"

#include "alife_simulator.h"
#include "alife_time_manager.h"

#define sec2ms		1000
#define min2ms		60*sec2ms
#define hour2ms		60*min2ms
#define day2ms		24*hour2ms

namespace Times
{
	u64 GetGameTime()
	{
		return Alife()->time_manager().game_time();
	}

	void SetGameTime(u64 new_game_time)
	{
		Alife()->time_manager().set_game_time(new_game_time);
	}

	u32 GetGameTimeInHours()
	{
		u32 game_hours = u32(Alife()->time_manager().game_time() / 1000 / 60 / 60);

		return game_hours;
	}

	float GetGameTimeFactor()
	{
		return Alife()->time_manager().time_factor();
	}

	void SetGameTimeFactor(float new_time_factor)
	{
		Alife()->time_manager().set_time_factor(new_time_factor);
	}

	void GetGameDateAndTime(u32& year, u32& month, u32& day, u32& hours, u32& mins, u32& secs, u32& milisecs)
	{
		split_time(GetGameTime(), year, month, day, hours, mins, secs, milisecs);
	}

	void GetDayTimeHour(u32& hours)
	{
		u32 dummy1;
		split_time(GetGameTime(), dummy1, dummy1, dummy1, hours, dummy1, dummy1, dummy1);
	}

	void GetDayTimeMinute(u32& minute)
	{
		u32 dummy1;
		split_time(GetGameTime(), dummy1, dummy1, dummy1, dummy1, minute, dummy1, dummy1);
	}

	float GetGameTimeInSec()
	{
		return	(float(GetGameTime() % (24 * 60 * 60 * 1000))) / 1000.f;
	}

	u32 GetGameTimeInSecU()
	{
		return	u32((GetGameTime() % (24 * 60 * 60 * 1000)) / 1000);
	}

	u32 GetGameTimeInMS()
	{
		return	u32(GetGameTime() % (24 * 60 * 60 * 1000));
	}

	const shared_str GetGameDateAsString(EDatePrecision datePrec, char dateSeparator)
	{
		return GetDateAsString(GetGameTime(), datePrec, dateSeparator);
	}


	const shared_str GetGameTimeAsString(ETimePrecision timePrec, char timeSeparator)
	{
		return GetTimeAsString(GetGameTime(), timePrec, timeSeparator);
	}


	const shared_str GetTimeAsString(ALife::_TIME_ID time, ETimePrecision timePrec, char timeSeparator, bool full_mode)
	{
		string64 bufTime;

		ZeroMemory(bufTime, sizeof(bufTime));

		u32 year = 0, month = 0, day = 0, hours = 0, mins = 0, secs = 0, milisecs = 0;

		split_time(time, year, month, day, hours, mins, secs, milisecs);

		// Time
		switch (timePrec)
		{
		case etpTimeToHours:
			xr_sprintf(bufTime, "%02i", hours);
			break;
		case etpTimeToMinutes:
			if (full_mode || hours > 0) {
				xr_sprintf(bufTime, "%02i%c%02i", hours, timeSeparator, mins);
				break;
			}
			xr_sprintf(bufTime, "%02i%c%02i", hours, timeSeparator, mins);
			break;
		case etpTimeToSeconds:
			if (full_mode || hours > 0) {
				xr_sprintf(bufTime, "%02i%c%02i%c%02i", hours, timeSeparator, mins, timeSeparator, secs);
				break;
			}
			xr_sprintf(bufTime, "%02i%c%02i%c%02i", hours, timeSeparator, mins, timeSeparator, secs);
			break;
		case etpTimeToMilisecs:
			xr_sprintf(bufTime, "%02i%c%02i%c%02i%c%02i", hours, timeSeparator, mins, timeSeparator, secs, timeSeparator, milisecs);
			break;
		case etpTimeToSecondsAndDay:
		{
			int total_day = (int)(time / (1000 * 60 * 60 * 24));
			xr_sprintf(bufTime, sizeof(bufTime), "%dd %02i%c%02i%c%02i", total_day, hours, timeSeparator, mins, timeSeparator, secs);
			break;
		}
		default:
			R_ASSERT(!"Unknown type of date precision");
		}

		return bufTime;
	}


	const shared_str GetDateAsString(ALife::_TIME_ID date, EDatePrecision datePrec, char dateSeparator)
	{
		string64 bufDate;

		ZeroMemory(bufDate, sizeof(bufDate));

		u32 year = 0, month = 0, day = 0, hours = 0, mins = 0, secs = 0, milisecs = 0;

		split_time(date, year, month, day, hours, mins, secs, milisecs);

		// Date
		switch (datePrec)
		{
		case edpDateToYear:
			xr_sprintf(bufDate, "%04i", year);
			break;
		case edpDateToMonth:
			xr_sprintf(bufDate, "%02i%c%04i", month, dateSeparator, year);
			break;
		case edpDateToDay:
			xr_sprintf(bufDate, "%02i%c%02i%c%04i", day, dateSeparator, month, dateSeparator, year);
			break;
		default:
			R_ASSERT(!"Unknown type of date precision");
		}

		return bufDate;
	}

	const shared_str GetTimeAndDateAsString(ALife::_TIME_ID time)
	{
		string256 buf;

		LPCSTR time_str = GetTimeAsString(time, etpTimeToMinutes).c_str();
		LPCSTR date_str = GetDateAsString(time, edpDateToDay).c_str();

		strconcat(sizeof(buf), buf, time_str, ", ", date_str);

		return buf;
	}

	LPCSTR GetTimePeriodAsString(LPSTR _buff, u32 buff_sz, ALife::_TIME_ID _from, ALife::_TIME_ID _to)
	{
		u32 year1, month1, day1, hours1, mins1, secs1, milisecs1;
		u32 year2, month2, day2, hours2, mins2, secs2, milisecs2;

		split_time(_from, year1, month1, day1, hours1, mins1, secs1, milisecs1);
		split_time(_to, year2, month2, day2, hours2, mins2, secs2, milisecs2);

		int cnt = 0;
		_buff[0] = 0;

		if (month1 != month2)
			cnt = xr_sprintf(_buff + cnt, buff_sz - cnt, "%d %s ", month2 - month1, *CStringTable().translate("ui_st_months"));

		if (!cnt && day1 != day2)
			cnt = xr_sprintf(_buff + cnt, buff_sz - cnt, "%d %s", day2 - day1, *CStringTable().translate("ui_st_days"));

		if (!cnt && hours1 != hours2)
			cnt = xr_sprintf(_buff + cnt, buff_sz - cnt, "%d %s", hours2 - hours1, *CStringTable().translate("ui_st_hours"));

		if (!cnt && mins1 != mins2)
			cnt = xr_sprintf(_buff + cnt, buff_sz - cnt, "%d %s", mins2 - mins1, *CStringTable().translate("ui_st_mins"));

		if (!cnt && secs1 != secs2)
			cnt = xr_sprintf(_buff + cnt, buff_sz - cnt, "%d %s", secs2 - secs1, *CStringTable().translate("ui_st_secs"));

		return _buff;
	}

}

xrTime convert_time(u32 time)
{
	float time_factor = GetGameTimeFactor();
	ALife::_TIME_ID val = iFloor(time_factor * ((float)time) / 10);
	return xrTime(val);
}

u32 convert_time(const xrTime &timer) 
{
	return (u32) timer.time_id();
}

xrTime get_time_struct()
{
	return xrTime(GetGameTime());
}

LPCSTR	xrTime::dateToString	(int mode)								
{ 
	return *GetDateAsString(m_time,(EDatePrecision)mode);
}
LPCSTR	xrTime::timeToString	(int mode)								
{ 
	return *GetTimeAsString(m_time,(ETimePrecision)mode);
}

void	xrTime::add				(const xrTime& other)					
{  
	m_time += other.m_time;				
}
void	xrTime::sub				(const xrTime& other)					
{  
	if(*this>other)
		m_time -= other.m_time; 
	else 
		m_time=0;	
}

void	xrTime::setHMS			(int h, int m, int s)					
{ 
	m_time=0; 
	m_time+=generate_time(1,1,1,h,m,s);
}

void	xrTime::setHMSms		(int h, int m, int s, int ms)			
{ 
	m_time=0; 
	m_time+=generate_time(1,1,1,h,m,s,ms);
}

void	xrTime::set				(int y, int mo, int d, int h, int mi, int s, int ms)
{ 
	m_time=0; 
	m_time+=generate_time(y,mo,d,h,mi,s,ms);
}

void	xrTime::set				(int d, int h, int mi, int s, int ms)
{ 
	m_time=0; 
	m_time+=__generate_add_time(d,h,mi,s)+ms;
}

void	xrTime::get				(u32 &y, u32 &mo, u32 &d, u32 &h, u32 &mi, u32 &s, u32 &ms)
{
	split_time(m_time,y,mo,d,h,mi,s,ms);
}

float	xrTime::diffSec			(const xrTime& other)					
{ 
	if(*this>other) 
		return (m_time-other.m_time)/(float)sec2ms; 
	return ((other.m_time-m_time)/(float)sec2ms)*(-1.0f);	
}
