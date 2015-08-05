/*
 * Profiler.c
 *
 *  Created on: Feb 1, 2012
 *      Author: chwolfe2
 */

#include "Profiler.h"
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>

typedef struct __Timer{
	int running;
	int multiplier;
	int maxmultiplier;
	struct timeval start;
	double total;
	double count;
} Timer;

const char * timerNames[] = {
		"Spec   : create         ",
		"Spec   : insert         ",
		"Spec   : monitor        ",
		"Spec   : next result    ",
		"Spec   : release result ",
		"Spec   : destroy        ",
		"OpSpec : init           ",
		"OpSpec : run            ",
		"OpSpec : cleanup        ",
		"OpSpec : receive        ",
		"OpSpec : verify         ",
		"OpSpec : insert         ",
		"OpSpec : monitor        ",
		"OpSpec : next           ",
		"OpSpec : prep           ",
		"OpSpec : write          ",
		"OpSpec : release        ",
		"Extra  : unpack         ",
		"Extra  : OP LIFETIME    ",
		"FFT    : THREAD0        ",
		"FFT    : THREAD1        ",
		"FFT    : THREAD2        ",
		"FFT    : THREAD3        ",
};

#define NUM_TIMERS 256

Timer timers[NUM_TIMERS];

#define UNLOCKED 0
#define LOCKED   1

volatile int listMutex = UNLOCKED;

static inline void lockList(){
	while (!__sync_bool_compare_and_swap(&listMutex, UNLOCKED, LOCKED)){}

}
static inline void unlockList(){
	while (!__sync_bool_compare_and_swap(&listMutex, LOCKED, UNLOCKED)){
		printf("Locking error detected !!!!.\n");
	}

}
void profilerReset(){
	bzero((void*)timers, NUM_TIMERS * sizeof(Timer));

}

static inline double elapsed(struct timeval* start,struct timeval* stop){
	double a = (((double)start->tv_sec) * 1000000.0f) + ((double)start->tv_usec);
	double b = (((double)stop->tv_sec) * 1000000.0f) + ((double)stop->tv_usec);
	return ((b-a) / 1000000.0f);
}


void profilerStart(int i){
	struct timeval stop;
	lockList();
	if (timers[i].running){
		gettimeofday(&stop, NULL);
		timers[i].total += elapsed(&timers[i].start,&stop) * (double) timers[i].multiplier;
		timers[i].multiplier ++;
		if (timers[i].multiplier > timers[i].maxmultiplier) timers[i].maxmultiplier = timers[i].multiplier;
		timers[i].start = stop;
	} else {
		gettimeofday(&timers[i].start, NULL);
		timers[i].running    = 1;
		timers[i].multiplier = 1;
		if (timers[i].multiplier > timers[i].maxmultiplier) timers[i].maxmultiplier = timers[i].multiplier;
	}
	unlockList();
}
void profilerStop(int i){
	lockList();
	struct timeval stop;
	if (timers[i].running){
		gettimeofday(&stop, NULL);
		timers[i].total     += elapsed(&timers[i].start,&stop) * (double) timers[i].multiplier;
		timers[i].multiplier--;
		if (timers[i].multiplier == 0){
			timers[i].running = 0;
		}
		timers[i].count ++;
		unlockList();
		return;
	} else {
		printf("Profiler error: timer already stopped.\n");
		unlockList();
		return;
	}
	unlockList();
}
void profilerReport(int i){
	lockList();
	printf ("Profiler - Time report: '%s' %9.9f s, %8.0f hits, %6d depth.\n",timerNames[i],timers[i].total,timers[i].count,timers[i].maxmultiplier);
	unlockList();
	return;


}
void profilerReportAll(){
	lockList();
	int i =0;
	for(; i<=LAST_TIMER; i++){
		printf ("Profiler - Time report: '%s' %9.9f s, %8.0f hits, %6d depth, average = %9.9f s.\n",timerNames[i],timers[i].total,timers[i].count,timers[i].maxmultiplier,(timers[i].total/timers[i].count));
	}
	unlockList();
}






