#pragma once

#include "alife_space.h"
#include "../Times.h"

class xrTime
{
	ALife::_TIME_ID		m_time;

public:
	xrTime() : m_time(0)											{}
	xrTime(const xrTime& other) : m_time(other.m_time)				{}
	xrTime(ALife::_TIME_ID t) : m_time(t)							{}
	xrTime(int y, int mo, int d, int h, int mi, int s, int ms)		{ set(y, mo, d, h, mi, s, ms);			}
	xrTime(int d, int h, int mi, int s, int ms)						{ set(d, h, mi, s, ms);					}

	bool	operator <		(const xrTime& other)	const			{ return m_time < other.m_time;			}
	bool	operator >		(const xrTime& other)	const			{ return m_time > other.m_time;			}
	bool	operator >=		(const xrTime& other)	const			{ return m_time >= other.m_time;		}
	bool	operator <=		(const xrTime& other)	const			{ return m_time <= other.m_time;		}
	bool	operator ==		(const xrTime& other)	const			{ return m_time == other.m_time;		}
	xrTime	operator +		(const xrTime& other)					{ return xrTime(m_time+other.m_time);	}
	xrTime	operator -		(const xrTime& other)					{ return xrTime(m_time-other.m_time);	}
	
	float	diffSec			(const xrTime& other);
	void	add				(const xrTime& other);
	void	sub				(const xrTime& other);

	void	add_script		(xrTime* other){add(*other);};
	void	sub_script		(xrTime* other){sub(*other);};
	float	diffSec_script	(xrTime* other){return diffSec(*other);};

	void	setHMS			(int h, int m, int s);
	void	setHMSms		(int h, int m, int s, int ms);
	void	set				(int y, int mo, int d, int h, int mi, int s, int ms);
	void	set				(int d, int h, int mi, int s, int ms);
	void	get				(u32 &y, u32 &mo, u32 &d, u32 &h, u32 &mi, u32 &s, u32 &ms);
	
	u32		time_id			() const { return u32(m_time & u32(-1)); }
	u64		time			() const { return m_time;				 }

	LPCSTR	dateToString	(int mode);
	LPCSTR	timeToString	(int mode);
};


extern xrTime get_time_struct();
extern xrTime convert_time(u32 time);
extern u32 convert_time(const xrTime &timer);


namespace Times
{
	// Точность возвращаемого функцией GetGameDateTimeAsString значения: до часов, до минут, до секунд
	enum ETimePrecision
	{
		etpTimeToHours = 0,
		etpTimeToMinutes,
		etpTimeToSeconds,
		etpTimeToMilisecs,
		etpTimeToSecondsAndDay
	};

	// Точность возвращаемого функцией GetGameDateTimeAsString значения: до года, до месяца, до дня
	enum EDatePrecision
	{
		edpDateToDay,
		edpDateToMonth,
		edpDateToYear
	};

	// Время Игрового мира
	u64 GetGameTime();
	// Время Игрового мира
	void SetGameTime										(u64 new_game_time);

	// Время Игрового мира в часах
	u32 GetGameTimeInHours();

	// Скорость времени Игрового мира
	float GetGameTimeFactor();
	// Скорость времени Игрового мира
	void SetGameTimeFactor									(float new_time_factor);

	// Отформатированное игровое время в обычном календарно-временном представлении
	void GetGameDateAndTime									(u32& year, u32& month, u32& day, u32& hours, u32& mins, u32& secs, u32& milisecs);
	// Час (в пределах 0-23)
	void GetDayTimeHour										(u32& hours);
	// Минуты (в пределах 0-59)
	void GetDayTimeMinute									(u32& minute);

	float GetGameTimeInSec();
	u32 GetGameTimeInSecU();
	u32 GetGameTimeInMS();

	// Конвертирует игровую дату в строку 
	const shared_str GetGameDateAsString					(EDatePrecision datePrec, char dateSeparator = '/');
	// Конвертирует игровое время в строку 
	const shared_str GetGameTimeAsString					(ETimePrecision timePrec, char timeSeparator = ':');

	const shared_str GetTimeAndDateAsString					(ALife::_TIME_ID time);

	// Конвертирует указаную дату в строку 
	const shared_str GetDateAsString						(ALife::_TIME_ID time, EDatePrecision datePrec, char dateSeparator = '/');
	// Конвертирует указаное время в строку 
	const shared_str GetTimeAsString						(ALife::_TIME_ID time, ETimePrecision timePrec, char timeSeparator = ':', bool full_mode = false);

	LPCSTR GetTimePeriodAsString							(LPSTR _buff, u32 buff_sz, ALife::_TIME_ID _from, ALife::_TIME_ID _to);
}

using namespace Times;
