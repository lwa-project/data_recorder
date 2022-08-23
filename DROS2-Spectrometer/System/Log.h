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

#ifndef LOG_H_
#define LOG_H_
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/progress.hpp>
#include "../Primitives/StringList.h"
#include "../Threading/LockHelper.h"
#include <unistd.h>
#include <sys/syscall.h>
#include "../Threading/ThreadManager.h"
#include "Config.h"

#define MAX_LOG_FILES      10
#define MAX_LOG_SIZE_BYTES 10485760 /* 10 MiB */
#define MAX_LOG_LINES      25

#define L_DEBUG   0
#define L_INFO    1
#define L_WARNING 2
#define L_ERROR   3
#define L_FATAL   4


#define QUOTE(x) #x
#define LOG_START_SESSION(level)\
	{																\
		SERIALIZE_ACCESS_ST2(Log,log);								\
		Log::startSession(  										\
				string(__FILE__) +  ":"  + LXS(__LINE__) , 			\
				level												\
		);

#define LOG_END_SESSION()											\
		Log::endSession();											\
	}

#define MESSAGING_COLORS       basic,     green,    black
#define MESSAGING_ERROR_COLORS bold,      green,    yellow
#define SECURITY_COLORS        blink,     white,    red
#define FATAL_COLORS           blink,     black,    red
#define FATAL2_COLORS          blink,     black,    green
#define SYSTEM_COLORS          basic,     magenta,  black
#define OBJECT_COLORS          bold,      cyan,     black
#define OBJECT_ERROR_COLORS    blink,     cyan,     yellow
#define ACTOR_COLORS           basic,     white,    black
#define ACTOR_WARNING_COLORS   bold,      yellow,   black
#define ACTOR_ERROR_COLORS     bold,      red,      black
#define CONFIG_COLORS          bold,      green,    blue
#define TRACE_COLORS           bold,      white,    yellow
#define SCHEDULE_COLORS        basic,     white,    black
#define PROGRAM_ERROR_COLORS   blink,     black,    magenta
#define STORAGE_COLORS         underline, magenta,  white
#define OPERATION_COLORS       underline, blue,     white
#define OPERATION_ERROR_COLORS blink,     blue,     white

#define LOG(lv, msg)         			\
	LOG_START_SESSION(lv) 				\
	cout << msg; 						\
	LOG_END_SESSION()

#define LOGC(lv, msg, ...)  			\
	LOG_START_SESSION(lv) 				\
	cout << ANSI::hl(msg, __VA_ARGS__ );\
	LOG_END_SESSION()

//#define TRACING

#ifdef TRACING
	#define TRACE(XXX)           {if (XXX){ LOGC(L_DEBUG, string(__FUNCTION__) + string("():") + LXS(__LINE__), TRACE_COLORS); }}
	#define TR_MESSAGE_STRUCTS false
	#define TR_UTILITY         false
	#define TR_SPEC			   false
#else
	#define TRACE(XXX)
	#define TR_MESSAGE_STRUCTS false
	#define TR_UTILITY         true
#endif





#include "../Primitives/StdTypes.h"
#include <stdio.h>
#include <string.h>

#include "Time.h"
#include "ANSI.h"
#include "Shell.h"
#include "../Primitives/TypeConversion.h"

namespace fs = boost::filesystem;

class Log {
public:
	static void initializeLog(){
		if (!isLogInitialized){
			rotate();
			openLogFile();
			isLogInitialized=true;
		}
	}

	static void reinitialize(){
		if (isLogInitialized){
			closeLogFile();
		}
		rotate();
		openLogFile();
		isLogInitialized=true;
	}

	static void setLogLevel(int level){
		level=max(0,level);
		level=min(L_FATAL,level);
		loglevel=level;
	}

	static void startSession(string where, int level=L_DEBUG){
		if (!isLogInitialized){
			initializeLog();
		}
		if (current_depth == 0){
			ses_start = Time::humanReadable(Time::now());
			ses_level = level;
			ses_where = where;
			ses_pid   = syscall(SYS_gettid);
			grab_cout();
		}
		current_depth++;
	}

	static void endSession(){
		if (!isLogInitialized){
			initializeLog();
		}
		current_depth--;
		if (current_depth == 0){
			release_cout();
		}
	}

	static void finalizeLog(){
		if (isLogInitialized){
			closeLogFile();
		}
		isLogInitialized=false;
	}

	static void createDump(){
		if (isLogInitialized){
			rotate(false, true);
		}
	}

	static int logCount(){
		SERIALIZE_ACCESS_ST(lastlog);
		return loglines.size();
	}

	static string lastLog(){
		SERIALIZE_ACCESS_ST(lastlog);
		return (ANSI::strip(loglines[loglines.size()-1]));
	}

	static string logEntry(int i){
		SERIALIZE_ACCESS_ST(lastlog);
		unsigned c  = loglines.size();
		if (i < (-((int)c))) return "";
		if (i >= ((int)c))    return "";
		if (i<0){
			return LXS(c+i)+":"+ANSI::strip(loglines[c+i]);
		} else {
			return LXS(i)+":"+ANSI::strip(loglines[i]);
		}
	}

public:
	DECLARE_ACCESS_MUTEX_ST(log);

private:

	static void rotate(bool isBecauseDayChange=false, bool createDump=false){
		string fn;
		string fn_next;

		if (isLogInitialized){
			closeLogFile();
		}
		for (int i=(MAX_LOG_FILES-1); i>=0; i--){
			fn      = DEFAULT_LOG_FILE + string(".") + LXS(i);
			fn_next = DEFAULT_LOG_FILE + string(".") + LXS(i+1);
			if (fs::exists(fn)){
				::rename(fn.c_str(),fn_next.c_str());
			}
		}
		fn = DEFAULT_LOG_FILE + string(".") + LXS(MAX_LOG_FILES);
		if (fs::exists(fn)){
			::remove(fn.c_str());
		}
		fn      = DEFAULT_LOG_FILE;
		fn_next = DEFAULT_LOG_FILE + string(".") + LXS(0);
		if (fs::exists(fn)){
			if (createDump){
				string dateStr = Time::filenameSuitable(Time::now());
				string fn_dump = "DROS_crash_log_" + dateStr;
				Shell::run("cp " + fn + " " + fn_dump);
			}
			::rename(fn.c_str(),fn_next.c_str());
			::remove(fn.c_str());
		}
		if (isBecauseDayChange)
			cout << ANSI::hl("Logfile rotated due to midnight crossing.",bold,white, blue) << endl;
		else
			cout << ANSI::hl("========================== Logfile rotated ==========================",bold,white, blue) << endl;
		if (isLogInitialized){
			openLogFile();
		}

	}

	static void openLogFile(){
		if (logfile != NULL){
			cout << ANSI::hl("Program error: Tried to reopen logfile.",bold,yellow,red) << endl;
			return;
		}
		logfile = new ofstream(DEFAULT_LOG_FILE,ios::out | ios::app);
		if (!logfile){
			cout << ANSI::hl("Can't allocate storage for logfile.",bold,yellow,red) << endl;
		} else {
			if (!logfile->good()){
				cout << ANSI::hl("Can't open logfile.",bold,yellow,red) << endl;
				delete logfile;
				logfile = NULL;
			} else {
				(*logfile) << (Time::humanReadable(Time::now())) << ANSI::hl("========================== Logfile opened ==========================",bold,white, blue) << endl;
				cout << ANSI::hl("========================== Logfile opened ==========================",bold,white, blue) << endl;
				logopen_MJD = Time::now().MJD;
			}
		}
	}
	static void closeLogFile(){
		if (logfile == NULL){
			cout << ANSI::hl("Program error: Tried to close unopened logfile.",bold,yellow,red) << endl;
			return;
		} else {
			if (logfile->good()){
				(*logfile) << Time::humanReadable(Time::now()) << ANSI::hl("========================== Logfile closed ==========================",bold,white, blue) << endl;
				logfile->flush();
				logfile->close();
				cout << ANSI::hl("========================== Logfile closed ==========================",bold,white, blue) << endl;
			}
			delete logfile;
			logfile = NULL;
		}

	}

	static string lvlStringHl(int lvl){
		switch(lvl){
			case L_DEBUG   : return ANSI::hl(lvlString(lvl), bold,  blue,   black);
			case L_INFO    : return ANSI::hl(lvlString(lvl), bold,  green,  black);
			case L_WARNING : return ANSI::hl(lvlString(lvl), bold,  yellow, black);
			case L_ERROR   : return ANSI::hl(lvlString(lvl), blink,  red,    black);
			case L_FATAL   : return ANSI::hl(lvlString(lvl), blink, yellow, red);
			default        : return ANSI::hl(lvlString(lvl), blink, blue,   white);
		}
	}
	static string lvlString(int lvl){
		switch(lvl){
		case L_DEBUG   : return "[D]";
		case L_INFO    : return "[I]";
		case L_WARNING : return "[W]";
		case L_ERROR   : return "[E]";
		case L_FATAL   : return "[F]";
		default        : return "[?]";
		}
	}


	static void grab_cout(){
		cout_orig = cout.rdbuf();
		cout.rdbuf(ss.rdbuf());
	}
	static void release_cout(){
		string s = ss.str();
		cout.rdbuf(cout_orig);
		ss.str(string());
		ss.clear();
		if (ses_level >= loglevel){
			string threadName = ThreadManager::getInstance()->backtrace(ses_pid);
			if (threadName.length() > 16){
				threadName = threadName.erase(12,string::npos) + " ...";
			}
			while (threadName.length() < 16){
				threadName.push_back(' ');
			}

			string threadNameId = "["+threadName+"]";

			string pad      = "                       ";
			string prefix   = pad + lvlStringHl(ses_level);
			vector<string> lines;
			boost::split(lines, s, boost::is_any_of("\r\n"));
			lines.erase( remove_if( lines.begin(), lines.end(), boost::bind( &string::empty, _1 ) ), lines.end() );
			if (lines.size() > 1){
				cout << ses_start << "\t" <<  lvlStringHl(ses_level) /*<< " message originating in \t"<< ses_where*/ << endl;
				if (logfile && logfile->good()){
					(*logfile) << ses_start << " " <<  lvlStringHl(ses_level) << " " /*<< " message originating in "<< ses_where*/ << endl;
				}
				foreach(string line, lines){
					{
						SERIALIZE_ACCESS_ST(lastlog);
						loglines.push_back(line);
					}
					if (logfile && logfile->good()){
						(*logfile) << pad << " " <<  lvlStringHl(ses_level) << " " << threadNameId << " " << line << endl;
					}
					cout << pad << " " <<  lvlStringHl(ses_level) << " " << threadNameId << " " << line << endl;
				}
			} else if (lines.size() == 1){
				{
					SERIALIZE_ACCESS_ST(lastlog);
					loglines.push_back(lines[0]);
				}

				if (logfile && logfile->good()){
					(*logfile) << ses_start << " " <<  lvlStringHl(ses_level) << " " << threadNameId << " " << lines[0] << endl;
				}
				cout << ses_start << " " <<  lvlStringHl(ses_level) << " " << threadNameId << " " << lines[0] << endl;
			}
			if (logfile && logfile->good()){
				if (logfile->tellp() > MAX_LOG_SIZE_BYTES){
					rotate();
				}
			}
			while (loglines.size()>MAX_LOG_LINES){
				SERIALIZE_ACCESS_ST(lastlog);
				loglines.pop_front();
			}
		}
	}

	static streambuf*    cout_orig;
	static stringstream  ss;
	static string        ses_start;
	static int           ses_level;
	static string        ses_where;
	static bool          isLogInitialized;
	static int           current_depth;
	static ofstream*     logfile;
	static size_t        logopen_MJD;
	static int			 loglevel;
	static deque<string> loglines;
	static pid_t		 ses_pid;
	DECLARE_ACCESS_MUTEX_ST(lastlog);

	Log(){}
	virtual ~Log(){}
};

#endif /* LOG_H_ */
