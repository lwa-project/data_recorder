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
#ifndef TIMEKEEPER_H_
#define TIMEKEEPER_H_

#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <string>
using namespace std;

class TimeKeeper{
private:
	static timeval 	startTime;
	static timeval 	stopTime;
	static bool		initialized;
	TimeKeeper(); // only use static functions, so don't allow instantiation
public:
	static size_t getMPM();
	static size_t getMJD();
	static size_t getTT();
	static bool   isFuture(size_t MJD, size_t MPM);
	static double getRuntime();
	static void   resetRuntime();
	static string DottedDate();
	virtual ~TimeKeeper();
};

#endif /*TIMEKEEPER_H_*/
