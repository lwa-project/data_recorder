/*
  MCS-DROS: Monitor and Control System - Data Recorder Operating Software
  Copyright (C) 2009-2010  Virginia Tech, Christopher Wolfe <chwolfe2@vt.edu>

  This file is part of MCS-DROS.
 
  MCS-DROS is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  MCS-DROS is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with MCS-DROS.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "TimeKeeper.h"
#include <sys/time.h>
#include <time.h>
#include <iostream>
#include <sstream>
using namespace std;

TimeKeeper::TimeKeeper(){
}

TimeKeeper::~TimeKeeper(){
}
uint64_t TimeKeeper::getMPM(){
	timeval dt;
	gettimeofday(&dt, NULL);
	tm timeStruct;
	gmtime_r(&dt.tv_sec, &timeStruct);
	uint64_t hour        = timeStruct.tm_hour;
	uint64_t minute      = timeStruct.tm_min;
	uint64_t second      = timeStruct.tm_sec;
	uint64_t millisecond = dt.tv_usec / 1000;
	// compute MPM
    return (hour*3600 + minute*60 + second)*1000 + millisecond; 
}
string TimeKeeper::DottedDate(){
	timeval dt;
	gettimeofday(&dt, NULL);
	tm timeStruct;
	gmtime_r(&dt.tv_sec, &timeStruct);
	uint64_t year        = timeStruct.tm_year +1900;
	uint64_t month       = timeStruct.tm_mon +1;
	uint64_t day         = timeStruct.tm_mday;
	stringstream ss;
	ss << year << "."<< month << "."<< day ;
	return ss.str();
}
uint64_t TimeKeeper::getMJD(){
	timeval dt;
	gettimeofday(&dt, NULL);
	tm timeStruct;
	gmtime_r(&dt.tv_sec, &timeStruct);
	uint64_t year        = timeStruct.tm_year +1900;
	uint64_t month       = timeStruct.tm_mon +1;
	uint64_t day         = timeStruct.tm_mday;
	// compute MJD
    // adapted from http://paste.lisp.org/display/73536
    // can check result using http://www.csgnetwork.com/julianmodifdateconv.html
    uint64_t a = (14 - month) / 12;
    uint64_t y = year + 4800 - a;
    uint64_t m = month + (12 * a) - 3;
    uint64_t p = day + (((153 * m) + 2) / 5) + (365 * y);
    uint64_t q = (y / 4) - (y / 100) + (y / 400) - 32045;
    return ( (p+q) - 2400000.5);
    
}
uint64_t TimeKeeper::getTT(){
	timeval dt;
	gettimeofday(&dt, NULL);
	uint64_t rv = (dt.tv_sec * 196000000l) + (dt.tv_usec * 196l);
	return rv;
}

bool TimeKeeper::isFuture(uint64_t MJD, uint64_t MPM){
	uint64_t nowMPM=getMPM();
	uint64_t nowMJD=getMJD();
	if (MJD > nowMJD) 
		return true;
	if (MJD == nowMJD) 
		if (MPM > nowMPM)
			return true;
	return false;
	
}
timeval TimeKeeper::startTime;
timeval TimeKeeper::stopTime;
bool	TimeKeeper::initialized=false;	
double TimeKeeper::getRuntime(){
	if (!initialized) return 0;
	gettimeofday(&stopTime, NULL);
	double p2=(double)stopTime.tv_sec  + ((double) stopTime.tv_usec  )/1000000;
	double p1=(double)startTime.tv_sec + ((double) startTime.tv_usec )/1000000;
	return p2-p1;
}
void   TimeKeeper::resetRuntime(){
	gettimeofday(&startTime, NULL);
	initialized=true;
}
