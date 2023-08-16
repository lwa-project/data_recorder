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

#include <iostream>
#include <exception>
#include <cstdlib>
#include <csignal>
#include "../Common/Messaging.h"
#include "../Common/Threading.h"
#include "../Common/Common.h"
#include "../Common/Actors.h"
#include "../Common/Primitives.h"
#ifndef DROS_LIVE_BUFFER
  #include "../Spectrometer/Spectrometer.h"
#endif
#include "../System/Config.h"



void signalHandler(int sig){
	if (System::get()){
		System::get()->handleSignal(sig);
	}
}

int main(int argc, char* argv[]) {
	//DrosCoreApi_UT();
	//exit(-1);
	Log::initializeLog();
	Log::setLogLevel(L_DEBUG);
	CpuInfo::getCpuCount();                                             // trigger module loading
	HddInfo::getDeviceCount();                                          // pre-cache device/directory info
	Shell::run("sysctl -e -p "DEFAULT_TUNING_FILE);  // network tuning
	ThreadManager::getInstance()->announceSelf("System");
	signal(SIGTERM, signalHandler);
	signal(SIGUSR1, signalHandler);
	signal(SIGINT,  signalHandler);
	while (System::get() && System::get()->Run()){
		System::kill();
		System::reanimate();
	}
	Log::finalizeLog();
	return 0;
}

/*
void doSpecTest(){
	const unsigned nf         = 32;
	const unsigned nints      = 6144;
	const unsigned iterations = 100;
	const unsigned blocks     = (1024*6144*4)/(nf*nints);
	float* adata = (float*)malloc(nf*nints*blocks*sizeof(float));

	TimeStamp t0 = Time::now();
	Spectrometer spec(nf,nints,2,blocks,XandY,Noninterleaved,XXYY);
	if (!spec.isValid()){
		LOGC(L_FATAL, "Can't initialize spectrometer: " + spec.getReason(), FATAL2_COLORS);
		return;
	}
	TimeStamp t1 = Time::now();
	cout << "Initialized...\n";
	for (unsigned it=0; it< iterations; it++){
		for (unsigned i=0; i<(blocks/2); i++){
			spec.startBlock((i*2),&adata[i*2*nf*nints]);
			spec.startBlock((i*2)+1,&adata[((i*2)+1)*nf*nints]);
			spec.waitBlock((i*2));
			spec.waitBlock((i*2)+1);
			//spec.computeBlock(i);
			//cout << it << ":" << i << endl;
			//cout << "batch...\n";
		}
	}
	TimeStamp t2 = Time::now();

	millisecond_delta d1 = Time::getTimeStampDelta_ms(t0,t1);
	millisecond_delta d2 = Time::getTimeStampDelta_ms(t1,t2);
	cout << "Spectrometer dimensions: " << nf << " x " << nints << "("<<blocks<<" blocks, "<<iterations<<" iter.)"<< endl;
	cout << "Allocated in " << d1 << " milliseconds.\n";
	cout << "Computed in " << d2 << " milliseconds.\n";
	double blk_per_s = (double)(blocks * iterations * 1000)/(double)(d2);
	cout << "Computed " << blk_per_s << " blocks/s.\n";

}
*/
//#include "../Exceptions/SignalExceptionListener.h"

//void doSpecTest();

/*
void registerSignalListeners(){
	SignalExceptionListener::get()->addHandler(SIGINT);
	SignalExceptionListener::get()->addHandler(SIGQUIT);
	SignalExceptionListener::get()->addHandler(SIGILL);
	//SignalExceptionListener::get()->addHandler(SIGABRT);
	SignalExceptionListener::get()->addHandler(SIGFPE);
	SignalExceptionListener::get()->addHandler(SIGKILL);
	SignalExceptionListener::get()->addHandler(SIGSEGV);
	SignalExceptionListener::get()->addHandler(SIGPIPE);
	//SignalExceptionListener::get()->addHandler(SIGALRM);
	//SignalExceptionListener::get()->addHandler(SIGTERM);
	SignalExceptionListener::get()->addHandler(SIGUSR1);
	SignalExceptionListener::get()->addHandler(SIGUSR2);
	//SignalExceptionListener::get()->addHandler(SIGCHLD);
	SignalExceptionListener::get()->addHandler(SIGCONT);
	//SignalExceptionListener::get()->addHandler(SIGSTOP);
	SignalExceptionListener::get()->addHandler(SIGTSTP);
	//SignalExceptionListener::get()->addHandler(SIGTTIN);
	//SignalExceptionListener::get()->addHandler(SIGTTOU);
	//SignalExceptionListener::get()->addHandler(SIGPOLL);
	//SignalExceptionListener::get()->addHandler(SIGPROF);
	SignalExceptionListener::get()->addHandler(SIGSYS);
	SignalExceptionListener::get()->addHandler(SIGTRAP);
	//SignalExceptionListener::get()->addHandler(SIGURG);
	//SignalExceptionListener::get()->addHandler(SIGVTALRM);
	SignalExceptionListener::get()->addHandler(SIGXCPU);
	SignalExceptionListener::get()->addHandler(SIGXFSZ);
	SignalExceptionListener::get()->addHandler(SIGSTKFLT);
	SignalExceptionListener::get()->addHandler(SIGPWR);
	SignalExceptionListener::get()->addHandler(SIGWINCH);
}
*/