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
#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <pthread.h>
#include "TimeKeeper.h"
#include "Socket.h"
#include <list>
#include <string.h>
#include <stdio.h>
#include <cstdlib>
#include "Signals/Complex.h"
#include "Lwa/DrxFrameGenerator.hpp"
#include "Lwa/TbnFrameGenerator.h"
#include "Lwa/TbwFrameGenerator.h"
#include "Lwa/TbfFrameGenerator.h"
#include "Lwa/CorFrameGenerator.h"
#include "Signals/TestPatternGenerator.h"

using namespace std;
#include "Lwa/LWA.h"

void usage(string errorMsg){

	cout << "[Error]  " << errorMsg << endl;
	cout <<
	"Usage: " << endl <<
	"\tDataSource " << endl <<
	"\t                                                                         \n"
	"\tParameter  Format           Description                 Unit      DRX/TBN/TBW Default      RAW Mode Default\n"
	"\t=========  ===============  ==========================  ========  =======================  =======================\n"
	"\n"
	"\t-DRX       n/a (flag)       Generate 4-bit DRX data streams  N/A                                                  \n"
	"\t-TBN       n/a (flag)       Generate TBN data streams        N/A                                                  \n"
	"\t-TBW       n/a (flag)       Generate TBW data streams        N/A                                                  \n"
	"\t-TBF       n/a (flag)       Generate TBF data streams        N/A                                                  \n"
	"\t-COR       n/a (flag)       Generate COR data streams        N/A                                                  \n"
	"\t-RAW       n/a (flag)       Generate RAW data streams        N/A                                                  \n"
	"\t-po        n/a (flag)       fill packets with binary         N/A                                                  \n"
	"\t                              pattern. (No sine waves)                                                            \n"
	"\t-ro        integer (+)      Rate override                bytes/s  <optional>               <optional> \n"
	"\t-i         aaa.bbb.ccc.ddd  Destination IP address           N/A  <required>               <required> \n"
	"\t-p         integer (+)      Destination port                 N/A  <required>               <required> \n"
	"\t-s         integer (+)      Packet size                    bytes  ignored, 4128            <required> \n"
	"\t-r         integer (+)      Data Rate                    bytes/s  ignored, by filt. code   <required> \n"
	"\t-d         integer (0,+)    Duration                          ms  0, forever               0, forever \n"
	"\t-k         integer (+)      Key                              N/A  <ignored>                0xFEEDDEADBEEF2DAD\n"
	"\t-fc        integer (+)      DRX/TBN filter code             #1-7  7                        <ignored>\n"
	"\t                              Determines data rate, \n"
	"\t                              decimation factor, etc.\n"
	"\t                              DRX/TBN mode.\n"

	"\t-f0        float            Sine wave 0 frequency             Hz                           <ignored>\n"
	"\t-m0        float            Sine wave 0 amplitude           arb.  6.0                      <ignored>\n"
	"\t-f1        float            Sine wave 1 frequency             Hz                           <ignored>\n"
	"\t-m1        float            Sine wave 1 amplitude           arb.  4.0                      <ignored>\n"
	"\t-f2        float            Sine wave 2 frequency             Hz                           <ignored>\n"
	"\t-m2        float            Sine wave 2 amplitude           arb.  1.0                      <ignored>\n"
	"\t-cf0       float            Chirp frequency low               Hz  f0 * 3/8                 <ignored>\n"
	"\t-cf1       float            Chirp frequency high              Hz  f0 * 3/4                 <ignored>\n"
	"\t-cf2       float            Chirp sweep frequency             Hz  1/2                      <ignored>\n"
	"\t-cm        float            Chirp magnitude                 arb.  4.0                      <ignored>\n"
	"\t-cg        float            Chirp gamma                     arb.  10                       <ignored>\n"
	"\t-SAW       flag (no val.)   sweep saw instead of sin    --------  not set                  <ignored>\n"
	"\t-sig       float            Gaussian Noise Variance         arb.  1.0                      <ignored>\n"
	"\t-sc        float            scale                           arb.  8.0                      <ignored>\n"
	"\t-N         integer (+)      # frames to pre-generate      frames  4s worth of frames       <ignored>\n"
	"\n";
	exit(EXIT_FAILURE);
}

void printFrame(DrxFrame* f, bool compact=false, bool single=false){
	if (compact){
		for(int i=0; i<DRX_SAMPLES_PER_FRAME; i++){
			printf("%3hd %3hd ",(int)f->samples[i].i,(int)f->samples[i].q);
			if (((i & 0xf) == 0xf) || single){
				cout << endl;
			}
		}
	} else {
		DrxFrameGenerator::unfixByteOrder(f);
		cout << "==============================================================================" << endl;
		cout << "== Beam:              " << (int)f->header.drx_beam << dec << endl;
		cout << "== Tuning:            " << (int)f->header.drx_tuning << dec << endl;
		cout << "== Polarization:      " << (int)f->header.drx_polarization << dec << endl;
		cout << "== Frequency Code:    " << (int)f->header.freqCode << dec << endl;
		cout << "== Decimation Factor: " << (int)f->header.decFactor << dec << endl;
		cout << "== Time Offset:       " << (int)f->header.timeOffset << dec << endl;
		cout << "== Time Tag:          " << (long int)f->header.timeTag << dec << endl;
		cout << "== Status Flags:      " << f->header.statusFlags << hex << endl;
		cout << "==============================================================================" << endl;
		for(int i=0; i<128; i++){
			printf("%02hhx ",(int)f->samples[i].packed);
			if (((i & 0xf) == 0xf) || single){
				cout << endl;
			}
		}
		cout << " ... << additional data truncated >> " << endl;
		cout << "==============================================================================" << endl;
		DrxFrameGenerator::fixByteOrder(f);
	}
}
void checkIsCVframe(DrxFrame*f){
	int i=1;
	while((i<4096) && (f->samples[i].packed == f->samples[0].packed)){
		i++;
	}
	if (i==4096){
		cout << "Constant value frame detected.\n";
		printFrame(f,false,false);
		exit(-1);
	}

}
char * humanReadable(uint64_t timetag, uint64_t Fs = 196000000){
	static char buf[64];
	double tt = (double) timetag;
	double fs = (double) Fs;
	double fsm  = fs * 60 ;
	double fsh  = fsm * 60;
	double fsd  = fsh * 24;

	tt -= __builtin_floor(tt/fsd)*fsd; // get rid of days
	double H  = __builtin_floor(tt / fsh); 	tt -= (H*fsh);
	double M  = __builtin_floor(tt / fsm); 	tt -= (M*fsm);
	double S  = tt / fs;
	sprintf(buf,"%02.0f:%02.0f:%05.3f",H,M,S);
	return buf;
}


#define SECONDS_TO_COVER      4l
#define DEFAULT_DRX_DECFACTOR 10ll
#define DEFAULT_N             (SAMPLES_PER_SECOND * DRX_POLARIZATIONS * SECONDS_TO_COVER /  (DRX_SAMPLES_PER_FRAME * DEFAULT_DRX_DECFACTOR)) & (~0x0000000000000003l)


#define TEST_OPT_STRING(op_long, op_short, min_length, max_length, var, flag)                        \
	if (!strcmp(argv[i],"-" #op_long) || !strcmp(argv[i],"-" #op_short)){                            \
		if (i<p){                                                                                    \
			if ((strlen(argv[i+1])>=min_length) && (strlen(argv[i+1]) <= max_length)){               \
				var=string(argv[i+1]);                                                               \
				flag = true;                                                                		 \
				i+=2;                                                                                \
			} else {                                                                                 \
				usage("Invalid parameter '" #op_long "'. Must be between " #min_length " and " #max_length " characters");\
			}                                                                                        \
		} else {                                                                                     \
			usage("Missing parameter value '" #op_long "'.");                                        \
		}                                                                                            \
		continue;                                                                                    \
	}

#define TEST_OPT_DOUBLE(op_long, op_short, var, flag)                                                \
	if (!strcmp(argv[i],"-" #op_long) || !strcmp(argv[i],"-" #op_short)){                            \
		if (i<p){                                                                                    \
			var=atof(argv[i+1]);                                                                     \
			flag = true;                                                                        	 \
			i+=2;                                                                                    \
		} else {                                                                                     \
			usage("Missing parameter value '" #op_long "'.");                                        \
		}                                                                                            \
		continue;                                                                                    \
	}

#define TEST_OPT_SIZE_T(op_long, op_short, var, flag)                                                \
	if (!strcmp(argv[i],"-" #op_long) || !strcmp(argv[i],"-" #op_short)){                            \
		if (i<p){                                                                                    \
			var=strtoul(argv[i+1],NULL,10);                                                          \
			flag = true;                                                                    		 \
			i+=2;                                                                                    \
		} else {                                                                                     \
			usage("Missing parameter value '" #op_long "'.");                                        \
		}                                                                                            \
		continue;                                                                                    \
	}
#define TEST_OPT_USHORT(op_long, op_short, var, flag)                                                \
	if (!strcmp(argv[i],"-" #op_long) || !strcmp(argv[i],"-" #op_short)){                            \
		if (i<p){                                                                                    \
			var=(unsigned short)atoi(argv[i+1]);                                                     \
			flag = true;                                                                    		 \
			i+=2;                                                                                    \
		} else {                                                                                     \
			usage("Missing parameter value '" #op_long "'.");                                        \
		}                                                                                            \
		continue;                                                                                    \
	}

#define TEST_OPT_SETTABLE(op_long, op_short, var, val, flag)                                         \
	if (!strcmp(argv[i],"-" #op_long) || !strcmp(argv[i],"-" #op_short)){                            \
		var=val;                                                                                     \
		flag = true;                                                                        		 \
		i++;                                                                                        \
		continue;                                                                                    \
	}




int main(int argc, char * argv[]){

	cout << "TBN:  " << sizeof(TbnFrame)  << endl;
	cout << "TBW:  " << sizeof(TbwFrame)  << endl;
	cout << "DRX:  " << sizeof(DrxFrame)  << endl;
	cout << "TBF:  " << sizeof(TbfFrame)  << endl;
	cout << "COR:  " << sizeof(CorFrame)  << endl;


	try{
	size_t totalSent=0;


	unsigned short DestinationPort=0;
	string DestinationIpAddress;
	size_t DataSize=0;
	size_t Duration=0;
	size_t Rate=0;
	size_t RateOverride=0;
	size_t Key=0;
	bool DestinationIpAddressSpecified=false;
	bool DestinationPortSpecified=false;
	bool DataSizeSpecified=false;
	bool DurationSpecified=false;
	bool RateSpecified=false;
	bool RateOverrideSpecified=false;
	bool KeySpecified=false;
	bool fcSpecified=false;
	Mode mode = RAW;

	double rptTime=0.5;



	//common to multiple formats
	uint16_t filterCode      = 7;         // used by TBN, DRX, TBF
	uint64_t N               = DEFAULT_N; // used by TBN, TBW, DRX
	bool     useComplex      = false;     // used by TBN, TBW, DRX
	bool     correlatorTest  = false;     // used by TBN, TBW, DRX
	bool     bitPattern      = false;     // used by ALL
	uint16_t decFactor       = 10;        // used by TBN, DRX

	// set up a pattern that will be visible in spectrograms
	double   baseBandWidth   = 32.0;
	double   baseFreq        = 0.0;
	double   sineWaveFreq[3] = {-5.0, 5.0, 14.0};
	double   sineWaveMag[3]  = {2.0, 4.0, 6.0};
	double   noiseSpread     = 1.0;
	double   noiseScale      = 8.0;

	double   chirpGamma      = 1.0;
	double   chirpMag        = 4.0;
	double   chirpLowFreq    = -baseBandWidth / 2;
	double   chirpHighFreq   = baseBandWidth / 2;
	double   chirpSlewFreq   = 1e-6;   // 1/slew rate
	bool     chirpSinusoidal = true; // false=>sawtooth


	// tuning settings for each DRX tuning
	uint32_t freqCode[DRX_TUNINGS];       // what's the center freq's freq code
	double   tuningFreq[DRX_TUNINGS];     // where's it centered
	double   tuningBw[DRX_TUNINGS];       // what's the width from min to max
	__attribute__((unused)) double   tuningMinFreq[DRX_TUNINGS];  // what's the minimum frequency observable
	__attribute__((unused)) double   tuningMaxFreq[DRX_TUNINGS];  // what's the maximum frequency observable

	// tune the pattern iaw with above tuning settings
	double   adj_sineWaveFreq[DRX_TUNINGS][3];
	double   adj_chirpLowFreq[DRX_TUNINGS];
	double   adj_chirpHighFreq[DRX_TUNINGS];
	double   adj_chirpSlewFreq[DRX_TUNINGS];   // 1/slew rate

	// set defaults
	freqCode[0]            = FREQ_CODE(20);
	freqCode[1]            = FREQ_CODE(40);
	tuningBw[0]            = 10e6; // 10MHz
	tuningBw[0]            = 20e6; // 20MHz

	bool useAbsFreq = false;


	int i=1;
	int p=argc-1;
	__attribute__((unused)) bool ignoredFlag = false;
	while (i<argc){
		TEST_OPT_STRING(   DestinationIpAddress, i,   1, 15, DestinationIpAddress,  DestinationIpAddressSpecified);
		TEST_OPT_USHORT(   DestinationPort,      p,          DestinationPort,       DestinationPortSpecified);
		TEST_OPT_SIZE_T(   DataSize,             s,          DataSize,              DataSizeSpecified);
		TEST_OPT_SIZE_T(   Duration,             d,          Duration,              DurationSpecified);
		TEST_OPT_SIZE_T(   Rate,                 r,          Rate,                  RateSpecified);
		TEST_OPT_SIZE_T(   RateOverride,         ro,         RateOverride,          RateOverrideSpecified);
		TEST_OPT_SIZE_T(   Key,                  k,          Key,                   KeySpecified);
		TEST_OPT_SIZE_T(   filter_code,          fc,         filterCode,            fcSpecified);
		TEST_OPT_SETTABLE( DRX,                  DRX,        mode,DRX,              ignoredFlag);
		TEST_OPT_SETTABLE( TBN,                  TBN,        mode,TBN,              ignoredFlag);
		TEST_OPT_SETTABLE( TBW,                  TBW,        mode,TBW,              ignoredFlag);
		TEST_OPT_SETTABLE( COR,                  COR,        mode,COR,              ignoredFlag);
		TEST_OPT_SETTABLE( TBF,                  TBF,        mode,TBF,              ignoredFlag);
		TEST_OPT_SETTABLE( RAW,                  RAW,        mode,RAW,              ignoredFlag);
		TEST_OPT_SETTABLE( XCP,                  XCP,        correlatorTest,true,   ignoredFlag);
		TEST_OPT_SETTABLE( pattern-only,         po,         bitPattern,true,       ignoredFlag);
		TEST_OPT_DOUBLE(   sineWaveFreq0,        f0,         sineWaveFreq[0],       ignoredFlag);
		TEST_OPT_DOUBLE(   sineWaveFreq1,        f1,         sineWaveFreq[1],       ignoredFlag);
		TEST_OPT_DOUBLE(   sineWaveFreq2,        f2,         sineWaveFreq[2],       ignoredFlag);
		TEST_OPT_DOUBLE(   sineWaveMag0,         m0,         sineWaveMag[0],        ignoredFlag);
		TEST_OPT_DOUBLE(   sineWaveMag1,         m1,         sineWaveMag[1],        ignoredFlag);
		TEST_OPT_DOUBLE(   sineWaveMag2,         m2,         sineWaveMag[2],        ignoredFlag);
		TEST_OPT_DOUBLE(   noiseScale,           sc,         noiseScale,            ignoredFlag);
		TEST_OPT_DOUBLE(   noiseSpread,          sig,        noiseSpread,           ignoredFlag);
		TEST_OPT_DOUBLE(   chirpLowFreq,         cf0,        chirpLowFreq,          ignoredFlag);
		TEST_OPT_DOUBLE(   chirpHighFreq,        cf1,        chirpHighFreq,         ignoredFlag);
		TEST_OPT_DOUBLE(   chirpSlewFreq,        cf2,        chirpSlewFreq,         ignoredFlag);
		TEST_OPT_DOUBLE(   chirpMag,             cm,         chirpMag,              ignoredFlag);
		TEST_OPT_DOUBLE(   chirpGamma,           cg,         chirpGamma,            ignoredFlag);
		TEST_OPT_SETTABLE( SAW,                  SAW,        chirpSinusoidal,false, ignoredFlag);
		TEST_OPT_SETTABLE( SIN,                  SIN,        chirpSinusoidal,true,  ignoredFlag);
		TEST_OPT_USHORT(   useComplex,           uc,         useComplex,            ignoredFlag);
		TEST_OPT_SIZE_T(   N,                    N,          N,                     ignoredFlag);
		TEST_OPT_DOUBLE(   baseBandWidth,        bb,         baseBandWidth,         ignoredFlag);
		TEST_OPT_DOUBLE(   baseBandWidth,        fs,         baseBandWidth,         ignoredFlag);
		TEST_OPT_DOUBLE(   baseFreq,             bf,         baseBandWidth,         ignoredFlag);
		TEST_OPT_SIZE_T(   freqCode0,            fc0,        freqCode[0],           ignoredFlag);
		TEST_OPT_SIZE_T(   freqCode1,            fc1,        freqCode[1],           ignoredFlag);
		TEST_OPT_USHORT(   decFactor,            df,         decFactor,             ignoredFlag);
		TEST_OPT_SETTABLE( useAbsFreq,           uaf,        useAbsFreq,true,       ignoredFlag);

		if (strcmp(argv[i],"-f")==0){
			if (i<p) sineWaveFreq[0]=atof(argv[++i]); else  usage("Missing operand: f0");
			if (i<p) sineWaveFreq[1]=atof(argv[++i]); else  usage("Missing operand: f1");
			if (i<p) sineWaveFreq[2]=atof(argv[++i]); else  usage("Missing operand: f2");
			i++; continue;
		}else if (strcmp(argv[i],"-m")==0){
			if (i<p) sineWaveMag[0]=atof(argv[++i]); else  usage("Missing operand: m0");
			if (i<p) sineWaveMag[1]=atof(argv[++i]); else  usage("Missing operand: m1");
			if (i<p) sineWaveMag[2]=atof(argv[++i]); else  usage("Missing operand: m2");
			i++; continue;
		}
		usage(string("Unknown commandline option:")+argv[i]);
	}

	if (!DestinationIpAddressSpecified || !DestinationPortSpecified){
		usage("You must specify destination IP address and port number");
	}
	if (! DurationSpecified){
		cout << "Warning: No duration specified, will run until terminated. (Press CTRL + C to exit)\n";
		Duration = 0;
	}
	
	switch(mode){
	case DRX:
		if (!fcSpecified){
			cout << "Warning: No filter code specified, will default to 7.\n";
			filterCode = 7;
		} else {
			if (filterCode > 7){
				cout << "Warning: Filter code exceeds maximum value, will default to 7.\n";
				filterCode = 7;
			}
			if (filterCode < 1){
				cout << "Warning: Filter code exceeds minimum value, will default to 1.\n";
				filterCode = 1;
			}
		}
		Rate=DrxDataRates[filterCode-1];
		decFactor = DrxDecFactors[filterCode-1];
		if (DataSizeSpecified || RateSpecified){
			cout << "Warning: Rate and DataSize options are ignored in DRX mode.\n";
		}
		if (KeySpecified){
			cout << "Warning: Key value is ignored in DRX mode.\n";
		}
		break;
	case TBN:
		if (!fcSpecified){
			cout << "Warning: No filter code specified, will default to 7.\n";
			filterCode = 7;
		} else {
			if (filterCode > 7){
				cout << "Warning: Filter code exceeds maximum value, will default to 7.\n";
				filterCode = 7;
			}
			if (filterCode < 1){
				cout << "Warning: Filter code exceeds minimum value, will default to 1.\n";
				filterCode = 1;
			}
		}
		Rate=TbnDataRates[filterCode-1];
		decFactor = TbnDecFactors[filterCode-1];
		if (DataSizeSpecified || RateSpecified){
			cout << "Warning: Rate and DataSize options are ignored in DRX mode.\n";
		}
		if (KeySpecified){
			cout << "Warning: Key value is ignored in DRX mode.\n";
		}
		break;
	case TBW:
		Rate=104857600.0;
		break;
	case TBF:
		Rate=104857600.0;
		break;
	case COR:
		Rate=104857600.0;
		break;
	case RAW:
		if (!DataSizeSpecified || !RateSpecified){
			usage("You must specify data size and rate when not in DRX, TBN, or TBW modes");
		}
		if (! KeySpecified){
			cout << "Warning: No key specified, will default to 0xFEED.DEAD.BEEF.2.DAD.\n";
		}
		break;
	default:
		cout << "Unknown mode: " << mode << endl;
		exit(-1);
	}

	if (RateOverrideSpecified){
		cout << "Warning: Rate being overridden to "<<RateOverride<<endl;
		Rate=RateOverride;
	}

	// compute the adjusted frequencies for DRX tunings
	if (!useAbsFreq){
		for (int j=0; j<DRX_TUNINGS; j++){
			tuningFreq[j]          = FREQ_FROM_FREQ_CODE(freqCode[j]);
			tuningMinFreq[j]       = tuningFreq[j] - (tuningBw[j] / 2.0);
			tuningMaxFreq[j]       = tuningFreq[j] + (tuningBw[j] / 2.0);
			for (int i=0; i< 3; i++){
				adj_sineWaveFreq[j][i] = (((sineWaveFreq[i] - baseFreq) / baseBandWidth) * tuningBw[j]) +  (tuningFreq[j]);
			}
			adj_chirpLowFreq[j]  = (((chirpLowFreq  - baseFreq) / baseBandWidth) * tuningBw[j]) +  (tuningFreq[j]);
			adj_chirpHighFreq[j] = (((chirpHighFreq - baseFreq) / baseBandWidth) * tuningBw[j]) +  (tuningFreq[j]);
			adj_chirpSlewFreq[j] = (chirpSlewFreq - baseFreq  +  tuningFreq[j]);
		}
	} else {
		for (int j=0; j<DRX_TUNINGS; j++){
			tuningFreq[j]          = FREQ_FROM_FREQ_CODE(freqCode[j]);
			tuningMinFreq[j]       = tuningFreq[j] - (tuningBw[j] / 2.0);
			tuningMaxFreq[j]       = tuningFreq[j] + (tuningBw[j] / 2.0);
			for (int i=0; i< 3; i++){
				adj_sineWaveFreq[j][i] = sineWaveFreq[i];
			}
			adj_chirpLowFreq[j]  = chirpLowFreq;
			adj_chirpHighFreq[j] = chirpHighFreq;
			adj_chirpSlewFreq[j] = chirpSlewFreq;
		}
	}








	TestPatternGenerator* tp[DRX_TUNINGS];

	for (int j=0; j<DRX_TUNINGS; j++){
		if ((!bitPattern) && (mode!=RAW)){
			tp[j] = new TestPatternGenerator(
					useComplex,
					adj_sineWaveFreq[j][0], sineWaveMag[0],
					adj_sineWaveFreq[j][1], sineWaveMag[1],
					adj_sineWaveFreq[j][2], sineWaveMag[2],
					adj_chirpLowFreq[j], adj_chirpHighFreq[j], adj_chirpSlewFreq[j],
					chirpMag, chirpGamma, chirpSinusoidal,
					noiseSpread, noiseScale
			);
		} else {
			tp[j] = NULL;
		}
	}

	void*   fg  = NULL;
	char*   ptr = NULL;
	size_t* ts  = NULL;
	size_t* sid = NULL;
	size_t* key = NULL;
	DrxFrame*  f_drx;
	TbnFrame*  f_tbn;
	TbwFrame*  f_tbw;
	CorFrame*  f_cor;
	TbfFrame*  f_tbf;

	switch(mode){
		case DRX:
			fg = (void*) new DrxFrameGenerator(bitPattern, correlatorTest, useComplex, N, decFactor, 0, 0, 0, freqCode[0], freqCode[1], tp[0], tp[1]);
			if (!fg){
				cout << "Error: failed to allocate Drx Frame Generator.\n";
			}
			break;
		case TBN:
			fg = (void*) new TbnFrameGenerator(bitPattern, correlatorTest, useComplex, N, decFactor, tp[0]);
			if (!fg){
				cout << "Error: failed to allocate Tbn Frame Generator.\n";
			}
			break;
		case TBW:
			fg = (void*) new TbwFrameGenerator(bitPattern, correlatorTest, useComplex, N, tp[0]);
			if (!fg){
				cout << "Error: failed to allocate Tbw Frame Generator.\n";
			}
			break;
		case TBF:
			fg = (void*) new TbfFrameGenerator(bitPattern, correlatorTest, useComplex, N, tp[0]);
			if (!fg){
				cout << "Error: failed to allocate Tbf Frame Generator.\n";
			}
			break;
		case COR:
			fg = (void*) new CorFrameGenerator(bitPattern, correlatorTest, useComplex, N, tp[0]);
			if (!fg){
				cout << "Error: failed to allocate Cor Frame Generator.\n";
			}
			break;
		case RAW:
			ptr = new char [DataSize] ;
			if (!ptr){
				cout << "Error: failed to allocate raw packet.\n";
			}
			for(unsigned int i=0;i<DataSize;i++){
				ptr[i]=(char)(i&0xff);
			}
			ts  =&(((size_t *)ptr)[0]);
			sid =&(((size_t *)ptr)[1]);
			key =&(((size_t *)ptr)[2]);
			*sid = 0;
			*key = Key;

			break;
		default:
			cout << "Unknown mode: " << mode << endl;
			exit(-1);
		}


	Socket mySocket;
	mySocket.openOutbound(DestinationIpAddress.c_str(), DestinationPort);
	if (mySocket.outboundConnectionIsOpen() ){
		cout << "Send socket opened Successfully." << endl;
	} else {
		cout << "Could not open send socket to "<< DestinationIpAddress << " on port " << DestinationPort << endl;
		exit (EXIT_FAILURE);
	}

	TimeKeeper::resetRuntime();
	uint64_t lastTimeTag=0;
	//uint64_t fcount=0;
	uint64_t bs = 0;

	switch(mode){
			case TBN:
				((TbnFrameGenerator*)fg)->resetTimeTag(TimeKeeper::getTT());
				break;
			case TBW:
				((TbwFrameGenerator*)fg)->resetTimeTag(TimeKeeper::getTT());
				break;
			case DRX:
				((DrxFrameGenerator*)fg)->resetTimeTag(TimeKeeper::getTT());
				break;
			case TBF:
				((TbfFrameGenerator*)fg)->resetTimeTag(TimeKeeper::getTT());
				break;
			case COR:
				((CorFrameGenerator*)fg)->resetTimeTag(TimeKeeper::getTT());
				break;
			case RAW:
				break;
			default:
				cout << "Unknown mode: " << mode << endl;
				exit(-1);
			}

	while (
			((Duration!=0) && (TimeKeeper::getRuntime()<((double)Duration)/1000.0)) ||
			(Duration == 0)
		)
	{
		fflush(stdout);
		if (TimeKeeper::getRuntime() > rptTime) {
			rptTime += 0.5;
			double curRate =(((double)totalSent) /TimeKeeper::getRuntime());
			cout << "Status: sent "<< totalSent<<" bytes so far. (" << (uint64_t) curRate << " b/s) ";
			if(mode == RAW){
				cout << endl;
			}else{
				cout << "(timetag = " << humanReadable(lastTimeTag) << " ["<<lastTimeTag<<"] )" << endl;
			}
		}
		switch(mode){
		case TBN:
			f_tbn = ((TbnFrameGenerator*)fg)->next();
			lastTimeTag = __builtin_bswap64(f_tbn->header.timeTag);
			bs = mySocket.send((char*)f_tbn,sizeof(TbnFrame));
			if (bs!=sizeof(TbnFrame)){
				cout << "Error in send.\n";
				return -1;
			}
			totalSent+=bs;
			break;
		case TBW:
			f_tbw = ((TbwFrameGenerator*)fg)->next();
			lastTimeTag = __builtin_bswap64(f_tbw->header.timeTag);
			bs = mySocket.send((char*)f_tbw,sizeof(TbwFrame));
			if (bs!=sizeof(TbwFrame)){
				cout << "Error in send.\n";
				return -1;
			}
			totalSent+=bs;
			break;
		case DRX:
			f_drx = ((DrxFrameGenerator*)fg)->next();
			lastTimeTag = __builtin_bswap64(f_drx->header.timeTag);
			bs = mySocket.send((char*)f_drx,sizeof(DrxFrame));
			if (bs!=sizeof(DrxFrame)){
				cout << "Error in send.\n";
				return -1;
			}
			totalSent+=bs;
			break;
		case TBF:
			f_tbf = ((TbfFrameGenerator*)fg)->next();
			lastTimeTag = __builtin_bswap64(f_tbf->header.timeTag);
			bs = mySocket.send((char*)f_tbf,sizeof(TbfFrame));
			if (bs!=sizeof(TbfFrame)){
				cout << "Error in send.\n";
				return -1;
			}
			totalSent+=bs;
			break;
		case COR:
			f_cor = ((CorFrameGenerator*)fg)->next();
			lastTimeTag = __builtin_bswap64(f_cor->header.timeTag);
			bs = mySocket.send((char*)f_cor,sizeof(CorFrame));
			if (bs!=sizeof(CorFrame)){
				cout << "Error in send.\n";
				return -1;
			}
			totalSent+=bs;
			break;
		case RAW:
			*ts=TimeKeeper::getMPM();
			totalSent+=mySocket.send(ptr,DataSize);
			(*sid)++;
			break;
		default:
			cout << "Unknown mode: " << mode << endl;
			exit(-1);
		}
		// govern rate
		while ((((double)totalSent) /TimeKeeper::getRuntime()) > (double)Rate ){
			//wait
		}
	}
	} catch(std::exception& e){
			cout << "Exception caught: '" << e.what() << "'\n";
			exit(-1);
		}
}
