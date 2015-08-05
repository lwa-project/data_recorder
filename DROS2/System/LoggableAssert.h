/*
 * LoggableAssert.h
 *
 *  Created on: Nov 27, 2012
 *      Author: chwolfe2
 */

#ifndef LOGGABLEASSERT_H_
#define LOGGABLEASSERT_H_


#include "../System/Log.h"
#include "../System/ANSI.h"
#include "../System/System.h"
#include <cassert>

#define LOG_ASSERT(x)																\
		if (!__builtin_expect(x,1)){												\
			string test = #x;														\
			string file = __FILE__;													\
			string line = LXS(__LINE__);											\
			string func = __PRETTY_FUNCTION__;										\
			LOG_START_SESSION(L_FATAL);												\
			cout << endl;															\
			cout << ANSI::hl("Fatal: Assertion failure !!!", blink, red, black);	\
			cout << endl;															\
			cout << ANSI::hl("\tFile:       " + file, bold, magenta, black);		\
			cout << endl;															\
			cout << ANSI::hl("\tLine:       " + line, bold, magenta, black);		\
			cout << endl;															\
			cout << ANSI::hl("\tFunction:   " + func, bold, magenta, black);		\
			cout << endl;															\
			cout << ANSI::hl("\tAssertion:  " + test, bold, cyan, black);			\
			cout << endl;															\
			LOG_END_SESSION();														\
			System::notifyFatalError(file+":"+line+"<"+func+">","Runtime Assertion Failure: "+test);\
			usleep(100000); /* give system time to notice */                        \
		}


#endif /* LOGGABLEASSERT_H_ */
