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


#ifndef PATTERNS_H_
#define PATTERNS_H_
#include "../Threading/LockHelper.h"
#include <boost/preprocessor/arithmetic/add.hpp>
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/control/deduce_d.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/seq/size.hpp>

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// helper macros for enum creation
////////////////////////////////////////////////////////////////////////////
// use boost preprocessor to turn a sequence into an enum type with
// string-conversion convenience function
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
#define CXRX(u1,u2,v)        case v: return BOOST_PP_STRINGIZE(v);
#define STRINGIFY(x)         BOOST_PP_STRINGIZE(x)
#define JOIN(x,y)            BOOST_PP_CAT(x,y)
// declare an enum type
#define MAKE_ENUM(e_name, e_values) enum e_name{ BOOST_PP_SEQ_ENUM(e_values) }
// declare function mapping enum value to string
#define DECL_ENUM_STR(e_name)  string JOIN(e_name,ToString)(const e_name& e);
#define DEFN_ENUM_STR(e_name,e_values) 						\
	string JOIN(e_name,ToString)(const e_name& e){     		\
		switch(e){											\
		BOOST_PP_SEQ_FOR_EACH(CXRX,~,e_values)		        \
		default: return STRINGIFY(JOIN(e_name,Unknown));	\
		}													\
	}


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// helper macros for sequences, assignment, and token processing
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
#define WRAP(a) a
#define MAPIFY(x)            (x, STRINGIFY(x))
#define MAPIFY_WRAP(u1,u2,x) MAPIFY(x)
#define STRINGMAP_SEQ(s)     BOOST_PP_SEQ_FOR_EACH(MAPIFY_WRAP,~,s)


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// helper macros for singleton class creation
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
#define SINGLETON_CLASS_HEADER_PUBLIC(classname) \
	static classname* get(){\
		SERIALIZE_ACCESS_ST();\
		if (! initialized)	initialize();\
		return instance;\
	}\
	static void kill(){ \
		SERIALIZE_ACCESS_ST();\
		if (initialized){\
			if (instance!= NULL) {\
				delete instance;\
				instance = NULL;\
			}\
		}\
	}\
	static void reanimate(){\
		SERIALIZE_ACCESS_ST();\
		if (initialized && (instance == NULL)){\
			initialized=false;\
		}\
	}\
	static int SINGLETON_DUMMY_IGNORED

#define SINGLETON_CLASS_HEADER_PRIVATE(classname)\
	static void initialize(){\
		SERIALIZE_ACCESS_ST();\
		if (! initialized){\
			instance = new classname();\
			if (instance == NULL){\
				printf("Failed to create singleton" #classname ".\nExiting...\n");\
				exit(-1);\
			} else {\
				initialized = true;\
			}\
		}\
	}\
	DECLARE_ACCESS_MUTEX_ST();\
	static classname* instance;\
	static bool initialized;\
	classname();\
	virtual ~classname()

#define SINGLETON_CLASS_SOURCE(classname)\
	classname* WRAP(WRAP(classname)::)instance = NULL;\
	bool WRAP(WRAP(classname)::)initialized= false;\
	INIT_ACCESS_MUTEX_ST(classname)

#define SINGLETON_CLASS_CONSTRUCTOR(classname) WRAP(WRAP(classname)::)classname()
#define SINGLETON_CLASS_DESTRUCTOR(classname) WRAP(WRAP(classname)::)~classname()
// macro to wrap object methods in static wrapper
#define SAFE_STATIC_GETTER(classname, type, fn, def_val)\
	static type BOOST_PP_CAT(__,fn)(){\
	classname* s = classname::get();\
		if (s != NULL){\
			return s->fn();\
		} else {\
			LOGC(L_WARNING, "Static resource '" BOOST_PP_STRINGIZE(classname) "' getter unavailable, using default argument.", PROGRAM_ERROR_COLORS);\
			return def_val;\
		}\
	}

#define SAFE_STATIC_SETTER(classname, type, fn)\
	static void BOOST_PP_CAT(__,fn)(const type& val){\
	classname* s = classname::get();\
		if (s != NULL){\
			s->fn(val);\
		} else {\
			LOGC(L_WARNING, "Static resource '" BOOST_PP_STRINGIZE(classname) "' setter unavailable, dropping request.", PROGRAM_ERROR_COLORS);\
		}\
	}

// macro to use the safe static wrapper
#define SAFE_GET(classname, fn) BOOST_PP_CAT(WRAP(classname)::__,fn)()
#define SAFE_SET(classname, fn, val) BOOST_PP_CAT(WRAP(classname)::__,fn)(val)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EZ_THREAD(...)                                                                                          \
	boost::thread* t = new boost::thread(__VA_ARGS__);												            \
	if (!t){																									\
		LOGC(L_ERROR, "[EZ_THREAD] : " BOOST_PP_STRINGIZE(__VA_ARGS__) " failed allocation", SECURITY_COLORS);	\
	} else {																									\
		t->detach();																							\
	}

#define EZ_THREAD_GENERIC(method)	               EZ_THREAD(method)
#define EZ_THREAD_MEMBER(classname, method)        EZ_THREAD(boost::bind(WRAP(WRAP(WRAP(&)classname)::)method), this)
#define EZ_THREAD_STATIC_MEMBER(classname, method) EZ_THREAD(boost::bind(WRAP(WRAP(WRAP(&)classname)::)method))

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif /* PATTERNS_H_ */