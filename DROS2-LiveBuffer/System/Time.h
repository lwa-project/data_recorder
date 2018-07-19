// ========================= DROSv2 License preamble===========================
// Software for DROSv2 is covered jointly under GPLv3 and Boost 1.0 licenses,
// to the extent required by included Boost sources and GPL sources, and to the
// more restrictive case pertaining thereunto, as defined herebelow. Beyond those
// requirements, any code not explicitly restricted by either of thowse two license
// models shall be deemed to be licensed under the GPLv3 license and subject to
// those restrictions.
//
// Copyright 2012, Virginia Tech, University of New Mexico, and Christopher Wolfe
//
// ========================= Boost License ====================================
// Boost Software License - Version 1.0 - August 17th, 2003
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
//
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
// =============================== GPL V3 ====================================
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef MYTIME_H_
#define MYTIME_H_

#include "../Primitives/StdTypes.h"

#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <ctime>
#include <stdio.h>


typedef size_t modifiedJulianDay;
typedef size_t millisecond;
typedef size_t microsecond;
typedef ssize_t millisecond_delta;
typedef ssize_t microsecond_delta;

#define MICROS_PER_DAY (1000000ll*60ll*60ll*24ll)
#define MILLIS_PER_DAY (1000ll*60ll*60ll*24ll)
#define FOREVER        ((millisecond)-1ll)

class Time;

typedef struct __TimeStamp{
	modifiedJulianDay MJD;
	millisecond MPM;
public:
	__TimeStamp(const modifiedJulianDay& MJD=0, const millisecond& MPM=0):MJD(MJD),MPM(MPM){}
} TimeStamp;

TimeStamp __now();

typedef struct __TimeSlot{
	TimeStamp	start;
	millisecond duration;
public:
	__TimeSlot(const TimeStamp& start = __now(), const millisecond &duration=FOREVER):start(start),duration(duration){
	}
}TimeSlot;

typedef struct __HumanTime{
	size_t year;
	size_t month;
	size_t day;
	size_t hour;
	size_t minute;
	size_t second;
	size_t millisecond;
}HumanTime;



class Time {
public:
	static TimeSlot always;

	static millisecond getMPM(){
		struct timeval dt;
		gettimeofday(&dt, NULL);
		struct tm timeStruct;
		gmtime_r(&dt.tv_sec, &timeStruct);
		size_t hour        = timeStruct.tm_hour;
		size_t minute      = timeStruct.tm_min;
		size_t second      = timeStruct.tm_sec;
		size_t millisecond = dt.tv_usec / 1000;
		// compute MPM
	    return (hour*3600 + minute*60 + second)*1000 + millisecond;
	}
	static modifiedJulianDay getMJD(){
		struct timeval dt;
		gettimeofday(&dt, NULL);
		struct tm timeStruct;
		gmtime_r(&dt.tv_sec, &timeStruct);
		size_t year        = timeStruct.tm_year +1900;
		size_t month       = timeStruct.tm_mon +1;
		size_t day         = timeStruct.tm_mday;
		// compute MJD
	    // adapted from http://paste.lisp.org/display/73536
	    // can check result using http://www.csgnetwork.com/julianmodifdateconv.html
	    size_t a = (14 - month) / 12;
	    size_t y = year + 4800 - a;
	    size_t m = month + (12 * a) - 3;
	    size_t p = day + (((153 * m) + 2) / 5) + (365 * y);
	    size_t q = (y / 4) - (y / 100) + (y / 400) - 32045;
	    return ( (p+q) - 2400000.5);

	}

	static TimeStamp now(){
		TimeStamp rv;
		struct timeval dt;
		gettimeofday(&dt, NULL);
		struct tm timeStruct;
		gmtime_r(&dt.tv_sec, &timeStruct);
		size_t year        = timeStruct.tm_year +1900;
		size_t month       = timeStruct.tm_mon +1;
		size_t day         = timeStruct.tm_mday;
		size_t hour        = timeStruct.tm_hour;
		size_t minute      = timeStruct.tm_min;
		size_t second      = timeStruct.tm_sec;
		size_t millisecond = dt.tv_usec / 1000;
		// compute MJD
		// adapted from http://paste.lisp.org/display/73536
		// can check result using http://www.csgnetwork.com/julianmodifdateconv.html
		size_t a = (14 - month) / 12;
		size_t y = year + 4800 - a;
		size_t m = month + (12 * a) - 3;
		size_t p = day + (((153 * m) + 2) / 5) + (365 * y);
		size_t q = (y / 4) - (y / 100) + (y / 400) - 32045;
		rv.MJD = ( (p+q) - 2400000.5);
	    // compute MPM
	    rv.MPM = (hour*3600 + minute*60 + second)*1000 + millisecond;
		return rv;
	}

	static int compareTimestamps(const TimeStamp& a, const TimeStamp& b){
		if (a.MJD<b.MJD) return -1;
		if (a.MJD>b.MJD) return  1;
		if (a.MPM<b.MPM) return -1;
		if (a.MPM>b.MPM) return  1;
		return 0;
	}

	static TimeStamp addTime(const TimeStamp& when, const millisecond& increment){
		millisecond q = (when.MJD*MILLIS_PER_DAY) + when.MPM + increment;
		TimeStamp rv;
		rv.MJD = q/MILLIS_PER_DAY;
		rv.MPM = q%MILLIS_PER_DAY;
		return rv;
	}

	static TimeStamp subTime(const TimeStamp& when, const millisecond& decrement){
		millisecond q = (when.MJD*MILLIS_PER_DAY) + when.MPM - decrement;
		TimeStamp rv;
		rv.MJD = q/MILLIS_PER_DAY;
		rv.MPM = q%MILLIS_PER_DAY;
		return rv;
	}

	static TimeSlot applyWindow(const TimeSlot ts, millisecond winsize){
		TimeSlot rv;
		rv.start = Time::subTime(ts.start, winsize);
		if (ts.duration == FOREVER)
			rv.duration = FOREVER;
		else
			rv.duration = ts.duration + (2 * winsize);
		return rv;
	}

	static TimeStamp getTimeStampDelta(const TimeStamp& start, const TimeStamp& stop){
		millisecond q = (start.MJD*MILLIS_PER_DAY) + start.MPM;
		millisecond p = (stop.MJD*MILLIS_PER_DAY) + stop.MPM;
		if (q>p){
			return __TimeStamp();
		} else {
			millisecond o = p-q;
			TimeStamp rv;
			rv.MJD = o/MILLIS_PER_DAY;
			rv.MPM = o%MILLIS_PER_DAY;
			return rv;
		}
	}
	static millisecond_delta getTimeStampDelta_ms(const TimeStamp& start, const TimeStamp& stop){
		millisecond q = (start.MJD*MILLIS_PER_DAY) + start.MPM;
		millisecond p = (stop.MJD*MILLIS_PER_DAY) + stop.MPM;
		return (millisecond_delta) p - (millisecond_delta) q;
	}


	static millisecond millis_getSinceEpoch(){
		return micros_getSinceEpoch()/1000ll;
	}
	static millisecond millis_getMidnight(){
		return micros_getMidnight()/1000ll;
	}
	static void millis_waitUntil(millisecond when,millisecond checkInterval=1){
		while(!micros_nowIsAfter(when)){
			usleep(checkInterval*1000ll);
		}
	}
	static bool millis_nowIsAfter(millisecond when){
		return millis_getSinceEpoch() >= when;
	}


	static TimeStamp micros_toStamp(size_t micros){
		TimeStamp rv;
		struct timeval dt;
		dt.tv_sec  = micros/1000000lu;
		dt.tv_usec = micros%1000000lu;
		struct tm timeStruct;
		gmtime_r(&dt.tv_sec, &timeStruct);
		size_t year        = timeStruct.tm_year +1900;
		size_t month       = timeStruct.tm_mon +1;
		size_t day         = timeStruct.tm_mday;
		size_t hour        = timeStruct.tm_hour;
		size_t minute      = timeStruct.tm_min;
		size_t second      = timeStruct.tm_sec;
		size_t millisecond = dt.tv_usec / 1000;
		// compute MJD
		// adapted from http://paste.lisp.org/display/73536
		// can check result using http://www.csgnetwork.com/julianmodifdateconv.html
		size_t a = (14 - month) / 12;
		size_t y = year + 4800 - a;
		size_t m = month + (12 * a) - 3;
		size_t p = day + (((153 * m) + 2) / 5) + (365 * y);
		size_t q = (y / 4) - (y / 100) + (y / 400) - 32045;
		rv.MJD = ( (p+q) - 2400000.5);
	    // compute MPM
	    rv.MPM = (hour*3600 + minute*60 + second)*1000 + millisecond;
		return rv;
	}

	static microsecond micros_getSinceEpoch(size_t tt=0){
		if (tt == 0){
			struct timeval dt;
			gettimeofday(&dt, NULL);
			return ((((microsecond) dt.tv_sec) * 1000000lu) +((microsecond) dt.tv_usec));
		} else {
			return tt/196lu;
		}
	}

	static microsecond micros_getMidnight(){
		return (((micros_getSinceEpoch() / MICROS_PER_DAY)+1)*MICROS_PER_DAY)-MICROS_PER_DAY;
	}
	static void micros_waitUntil(microsecond when,microsecond checkInterval=500){
		while(!micros_nowIsAfter(when)){
			usleep(checkInterval);
		}
	}
	static bool micros_nowIsAfter(microsecond when){
		return micros_getSinceEpoch() >= when;
	}


	static bool overlaps(const TimeSlot& a,const TimeSlot& b){
		TimeStamp astart = a.start;
		TimeStamp astop  = addTime(a.start, a.duration);
		TimeStamp bstart = b.start;
		if ((a.duration == FOREVER)||(b.duration == FOREVER))
			return true;

		if (	(compareTimestamps(astart, bstart)==-1) &&
				(compareTimestamps(astop,  bstart)==-1) ) {
			return false;
		}
		TimeStamp bstop  = addTime(b.start, b.duration);
		if (	(compareTimestamps(astart, bstop)==1) &&
				(compareTimestamps(astop,  bstop)==1) ) {
			return false;
		}
		return true;
	}

	static int compare(const TimeSlot& a,const TimeStamp& b){
		if (compareTimestamps(b, a.start) == -1) return -1;
		if (a.duration == FOREVER) return 0;
		if (compare(b, addTime(a.start,a.duration))<=0) return 0;
		return 1;
	}

	// return:
	//    -1  if a is before b, without overlap
	//     1  if b is before a, without overlap
	//     0  if overlapping
	static int compare(const TimeSlot& a,const TimeSlot& b){
		switch (compareTimestamps(a.start, b.start)){
		case -1:
			if (a.duration==FOREVER){
				return 0;
			} else {
				TimeStamp a_stop = addTime(a.start,a.duration);
				if (compareTimestamps(a_stop, b.start) == -1)
					return -1;
				else
					return 0;
			}
		case 1:
			if (b.duration==FOREVER){
				return 0;
			} else {
				TimeStamp b_stop = addTime(b.start,b.duration);
				if (compareTimestamps(b_stop, a.start) == -1)
					return 1;
				else
					return 0;
			}
		case 0: // fall through
		default: return 0;
		}
	}


	static HumanTime humanTime(const TimeStamp& ts){

		time_t t = (ts.MPM + ((ts.MJD-40587) * MILLIS_PER_DAY))/1000ll;
		struct tm *tm = gmtime(&t);
		HumanTime h;
		h.year        = tm->tm_year+1900;
		h.month       = tm->tm_mon+1;
		h.day         = tm->tm_mday;
		h.hour        = tm->tm_hour;
		h.minute      = tm->tm_min;
		h.second      = tm->tm_sec;
		h.millisecond = (ts.MPM % 1000ll);
		return h;
	}

	static TimeStamp timeTagToStamp(size_t tt=0){
		return micros_toStamp(micros_getSinceEpoch(tt));
	}
	static string humanReadable(size_t tt=0){
		return htToString(humanTime(timeTagToStamp(tt)));
	}
	static string humanReadable(const TimeStamp& ts){
		return htToString(humanTime(ts));
	}
	static string filenameSuitable(const TimeStamp& ts){
		return htToString2(humanTime(ts));
	}
	static string htToString(const HumanTime& ht){
		char buf[1024];
		snprintf(buf,1024,"%02lu-%02lu-%02lu %02lu:%02lu:%02lu.%03lu",
		ht.year,ht.month,ht.day,ht.hour,ht.minute,ht.second,ht.millisecond);
		return string(buf);
	}

	static string htToString2(const HumanTime& ht){
		char buf[1024];
		snprintf(buf,1024,"%02lu-%02lu-%02lu_%02lu.%02lu.%02lu.%03lu",
		ht.year,ht.month,ht.day,ht.hour,ht.minute,ht.second,ht.millisecond);
		return string(buf);
	}

	virtual ~Time(){}
private:
	Time(){}

};
ostream& operator<< (ostream& out, const TimeStamp& ts);

#endif /* MYTIME_H_ */