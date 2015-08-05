/*
 * Profiler.h
 *
 *  Created on: Feb 1, 2012
 *      Author: chwolfe2
 */

#ifndef PROFILER_H_
#define PROFILER_H_


//#define ENABLE_PROFILER
#ifdef ENABLE_PROFILER
	#define PROFILE_START(x) profilerStart(x)
	#define PROFILE_STOP(x)  profilerStop(x)
	#define PROFILE_BEGIN(x)  {profilerReset(); profilerStart(TMR_OP_LIFE);}
	#define PROFILE_END(x)    {profilerStop(TMR_OP_LIFE); profilerReportAll();}
#else
	#define PROFILE_START(x)
	#define PROFILE_STOP(x)
	#define PROFILE_BEGIN(x)
	#define PROFILE_END(x)
#endif


void profilerStart(int i);
void profilerStop(int i);
void profilerReport(int i);
void profilerReportAll();
void profilerReset();

extern const char * timerNames[];
#define TMR_SPEC_CREATE 		0
#define TMR_SPEC_INSERT			1
#define TMR_SPEC_MONITOR		2
#define TMR_SPEC_NEXT			3
#define TMR_SPEC_RELEASE		4
#define TMR_SPEC_DESTROY		5
#define TMR_OPSPEC_INIT			6
#define TMR_OPSPEC_RUN			7
#define TMR_OPSPEC_CLEANUP		8
#define TMR_OPSPEC_RECEIVE		9
#define TMR_OPSPEC_VERIFY		10
#define TMR_OPSPEC_INSERT		11
#define TMR_OPSPEC_MONITOR		12
#define TMR_OPSPEC_NEXT			13
#define TMR_OPSPEC_PREP			14
#define TMR_OPSPEC_WRITE		15
#define TMR_OPSPEC_RELEASE		16
#define TMR_X_UNPACK			17
#define TMR_OP_LIFE				18
#define TMR_THREAD_0			19
#define TMR_THREAD_1			20
#define TMR_THREAD_2			21
#define TMR_THREAD_3			22
#define LAST_TIMER				22




#endif /* PROFILER_H_ */
