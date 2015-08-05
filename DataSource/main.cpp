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

#include "Complex.h"
#include "DrxFrameGenerator.hpp"

using namespace std;
/*
int8_t floatToFourBit(float val){
	int i=(int) val;
	if (i>)
}

PackedSample pack(UnpackedSample u){
	RealType i = u.i;
	RealType q = u.q;
	PackedSample p;
	p.i = floatToFourBit(i);
	p.q = floatToFourBit(q);
}
*/
void usage(string errorMsg){

	cout << "[Error]  " << errorMsg << endl;
	cout <<
	"Usage: " << endl <<
	"\tDataSource " << endl <<
	"\t                                                                         \n"
	"\tParameter  Format           Description                 Unit      DRX Mode Default         Legacy Mode Default\n"
	"\t=========  ===============  ==========================  ========  =======================  =======================\n"
	"\n"
	"\t-i         aaa.bbb.ccc.ddd  Destination IP address           N/A  <required>               <required> \n"
	"\t-p         integer (+)      Destination port                 N/A  <required>               <required> \n"
	"\t-s         integer (+)      Packet size                    bytes  ignored, 4128            <required> \n"
	"\t-r         integer (+)      Data Rate                    bytes/s  ignored, by filt. code   <required> \n"
	"\t-d         integer (0,+)    Duration                          ms  0, forever               0, forever \n"
	"\t-k         integer (+)      Key                              N/A  <ignored>                0xFEEDDEADBEEF2DAD\n"
	"\t-fc        integer (+)      DRX filter code                 #1-7  7                        <ignored>\n"
	"\t                              Determines data rate, \n"
	"\t                              decimation factor, etc.\n"
	"\t                              DRX mode.\n"
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
#define DP_BASE_FREQ_HZ         196e6
#define DRX_SAMPLES_PER_FRAME   4096
#define FREQ_CODE_FACTOR        (DP_BASE_FREQ_HZ / (double)0x100000000)
#define FREQ_CODE(f)            ((f*1e6*(double)0x100000000)/(DP_BASE_FREQ_HZ))

#define SAMPLES_PER_SECOND 196000000l
#define DECFACTOR          10l
#define STREAMS            4l
#define SECONDS_TO_COVER   4l
#define SAMPLES_PER_FRAME  ((uint64_t) DRX_SAMPLES_PER_FRAME)
//#define fs 32

int main(int argc, char * argv[]){
/*
	bzero((void*) &f, sizeof(DrxFrame));
	for(int i=0; i< 5; i++){
		fg.next(1,0,1,i*DRX_SAMPLES_PER_FRAME,i,&f);
		printFrame(&f,true,true);
	}
	return 0;

*/
	size_t totalSent=0;


	unsigned short DestinationPort=0;
	string DestinationIpAddress;
	size_t DataSize=0;
	size_t Duration=0;
	size_t Rate=0;
	size_t Key=0;
	bool DestinationIpAddressSpecified=false;
	bool DestinationPortSpecified=false;
	bool DataSizeSpecified=false;
	bool DurationSpecified=false;
	bool RateSpecified=false;
	bool KeySpecified=false;
	bool fcSpecified=false;
	bool DrxMode=false;
	double rptTime=0.5;
	uint64_t N           = (SAMPLES_PER_SECOND * STREAMS * SECONDS_TO_COVER /  (SAMPLES_PER_FRAME * DECFACTOR)) & (~0x0000000000000003l);
	double   fs          = 32;
	uint16_t decFactor   = 10;
	uint32_t fc0         = FREQ_CODE(20);
	uint32_t fc1		 = FREQ_CODE(20);
	uint32_t statusFlags = 0;
	uint8_t  beam        = 1;
	uint16_t timeOffset  = 0;
	uint16_t filterCode  = 7;

	cout << "debug: fc0=" << dec << fc0 << "; fc1=" << dec << fc1 << endl;


	float f0 = -fs/2;
	float f1 = fs/4;
	float f2 = fs/2;
	float m0 = 6.0;
	float m1 = m0-2.0;
	float m2 = m1-2.0;
	float sig = 1.0;
	float scale = 8.0;

	float cf0  = -fs/2;
	float cf1  = fs/2;
	float cf2  = fs * 1e-6 ;
	float cm   = 4.0;
	float cg   = 1.0;
	bool  csin = true;
	//bool  to_stdio = false;
	bool useComplex=false;
	bool correlatorTest=false;
	//bool  looptest = false;




	int i=1;
	int p=argc-1;
	while (i<argc){
		if ((strcmp(argv[i],"-DestinationIpAddress")==0) || (strcmp(argv[i],"-i"))==0){
			if (i<p){	if (strlen(argv[i+1])<=15){		DestinationIpAddress=string(argv[i+1]);		DestinationIpAddressSpecified=true;			i++;
			} else { usage("Invalid Source length. Must not be more than 15 characters"); }		} else { usage("Missing operand: Source"); }
		}else if ((strcmp(argv[i],"-DestinationPort")==0) || (strcmp(argv[i],"-p")==0)){
			if (i<p){	if (strlen(argv[i+1])<=5){		DestinationPort=(unsigned short)atoi(argv[i+1]);		DestinationPortSpecified=true;			i++;
			} else { usage("Invalid DestinationPort length. Must not be more than 5 characters"); }		} else { usage("Missing operand: DestinationPort"); }
		}else if ((strcmp(argv[i],"-DataSize")==0)||(strcmp(argv[i],"-s")==0)){
			if (i<p){	if (strlen(argv[i+1])<=4){		DataSize=strtoul(argv[i+1],NULL,10);		DataSizeSpecified=true;			i++;
			} else { usage("Invalid DataSize length. Must not be more than 4 characters"); }		} else { usage("Missing operand: DataSize"); }
		}else if ((strcmp(argv[i],"-Duration")==0)||(strcmp(argv[i],"-d")==0)){
			if (i<p){	if (strlen(argv[i+1])<=10){		Duration=strtoul(argv[i+1],NULL,10);		DurationSpecified=true;			i++;
			} else { usage("Invalid Duration length. Must not be more than 10 characters"); }		} else { usage("Missing operand: Duration"); }
		}else if ((strcmp(argv[i],"-Rate")==0)||(strcmp(argv[i],"-r")==0)){
			if (i<p){	if (strlen(argv[i+1])<=15){		Rate=strtoul(argv[i+1],NULL,10);		RateSpecified=true;			i++;
			} else { usage("Invalid Rate Length. Must not be more than 15 characters"); }		} else { usage("Missing operand: Rate"); }
		}else if ((strcmp(argv[i],"-Key")==0) || (strcmp(argv[i],"-k")==0)){
			if (i<p){	if (strlen(argv[i+1])<=15){		Key=strtoul(argv[i+1],NULL,10);		KeySpecified=true;			i++;
			} else { usage("Invalid Key length. Must not be more than 15 characters"); }		} else { usage("Missing operand: Key"); }
/*		}else if ((strcmp(argv[i],"-filter_code")==0) || (strcmp(argv[i],"-fc")==0)){
			if (i<p){	if (strlen(argv[i+1])==1){		filterCode=(uint16_t) strtoul(argv[i+1],NULL,10);		fcSpecified=true;			i++;
			} else { usage("Invalid filter code length. Must not be more than 1 character"); }		} else { usage("Missing operand: filter code"); }
*/		}else if (strcmp(argv[i],"-DRX")==0){
			DrxMode = true;
		}else if (strcmp(argv[i],"-XCP")==0){
			correlatorTest = true;
			DrxMode = true; //implied
		}else if (strcmp(argv[i],"-f")==0){
			if (i<p) f0=atof(argv[++i]); else  usage("Missing operand: f0");
			if (i<p) f1=atof(argv[++i]); else  usage("Missing operand: f1");
			if (i<p) f2=atof(argv[++i]); else  usage("Missing operand: f2");
		}else if (strcmp(argv[i],"-m")==0){
			if (i<p) m0=atof(argv[++i]); else  usage("Missing operand: m0");
			if (i<p) m1=atof(argv[++i]); else  usage("Missing operand: m1");
			if (i<p) m2=atof(argv[++i]); else  usage("Missing operand: m2");

		}else if (strcmp(argv[i],"-f0")==0){if (i<p) f0=atof(argv[++i]); else  usage("Missing operand: f0");
		}else if (strcmp(argv[i],"-f1")==0){if (i<p) f1=atof(argv[++i]); else  usage("Missing operand: f1");
		}else if (strcmp(argv[i],"-f2")==0){if (i<p) f2=atof(argv[++i]); else  usage("Missing operand: f2");
		}else if (strcmp(argv[i],"-m0")==0){if (i<p) m0=atof(argv[++i]); else  usage("Missing operand: m0");
		}else if (strcmp(argv[i],"-m1")==0){if (i<p) m1=atof(argv[++i]); else  usage("Missing operand: m1");
		}else if (strcmp(argv[i],"-m2")==0){if (i<p) m2=atof(argv[++i]); else  usage("Missing operand: m2");
		}else if (strcmp(argv[i],"-sig")==0){if (i<p) sig=atof(argv[++i]); else  usage("Missing operand: sigma");
		}else if (strcmp(argv[i],"-sc")==0){if (i<p) scale=atof(argv[++i]); else  usage("Missing operand: scale");
		}else if (strcmp(argv[i],"-cf0")==0){if (i<p) cf0=atof(argv[++i]); else  usage("Missing operand: cf0");
		}else if (strcmp(argv[i],"-cf1")==0){if (i<p) cf1=atof(argv[++i]); else  usage("Missing operand: cf1");
		}else if (strcmp(argv[i],"-cf2")==0){if (i<p) cf2=atof(argv[++i]); else  usage("Missing operand: cf2");
		}else if (strcmp(argv[i],"-cm")==0){if (i<p) cm=atof(argv[++i]); else  usage("Missing operand: cm");
		}else if (strcmp(argv[i],"-cg")==0){if (i<p) cg=atof(argv[++i]); else  usage("Missing operand: cg");
		}else if (strcmp(argv[i],"-SAW")==0){csin=false;
		}else if (strcmp(argv[i],"-SIN")==0){csin=false;
		}else if (strcmp(argv[i],"-useComplex")==0){
			if (i<p) useComplex=(strtoul(argv[++i],NULL,10)!=0); else  usage("Missing operand: useComplex");
		/*}else if (strcmp(argv[i],"-X")==0){
			to_stdio=true;
		}else if (strcmp(argv[i],"-LT")==0){
			looptest=true;*/
		}else if (strcmp(argv[i],"-N")==0){
			if (i<p) N=strtoul(argv[++i],NULL,10)& (~0x0000000000000003l); else  usage("Missing operand: N");
		}else if (strcmp(argv[i],"-fs")==0){
			if (i<p) fs=strtoul(argv[++i],NULL,10); else  usage("Missing operand: fs");
		}else if (strcmp(argv[i],"-df")==0){
			if (i<p) decFactor=(uint16_t)strtoul(argv[++i],NULL,10); else  usage("Missing operand: df");

		}else if (strcmp(argv[i],"-fc0")==0){
			if (i<p) fc0=(uint32_t)strtoul(argv[++i],NULL,10); else  usage("Missing operand: fc0");
		}else if (strcmp(argv[i],"-fc1")==0){
			if (i<p) fc1=(uint32_t)strtoul(argv[++i],NULL,10); else  usage("Missing operand: fc1");
		}else {
			usage(string("Unknown commandline option:")+argv[i]);
		}
		i++;
	}
	/*
	if (looptest){
		DestinationIpAddress = "127.0.0.1";
	}*/

	if (DrxMode || !DrxMode){
		if (! DurationSpecified){
			cout << "Warning: No duration specified, will run until terminated. (Press CTRL + C to exit)\n";
			Duration = 0;
		}

		if (!DestinationIpAddressSpecified || !DestinationPortSpecified){
			usage("You must specify destination IP address and port number");
		}
	}

	if (DrxMode){
		if (!fcSpecified){
			cout << "Warning: No filter code specified, will default to 7.\n";
			filterCode = 7;
		}
		if (filterCode > 7){
			cout << "Warning: Filter code exceeds maximum value, will default to 7.\n";
			filterCode = 7;
		}
		if (filterCode < 1){
			cout << "Warning: Filter code exceeds minimum value, will default to 1.\n";
			filterCode = 1;
		}
		if (DataSizeSpecified || RateSpecified){
			cout << "Warning: Rate and DataSize options are ignored in DRX mode.\n";
		}
		if (KeySpecified){
			cout << "Warning: Key value is ignored in DRX mode.\n";
		}

	}
	if (!DrxMode) {
		if (!DataSizeSpecified || !RateSpecified){
			usage("You must specify data size and rate when not in DRX mode");
		}
		if (! KeySpecified){
			cout << "Warning: No key specified, will default to 0xFEED.DEAD.BEEF.2.DAD.\n";
		}
	}



	/*
	DrxFrameGenerator(
			bool	 useComplex,
			uint64_t numFrames,
			RealType fs,
			uint16_t decFactor,
			uint8_t beam,
			uint16_t timeOffset,
			uint32_t statusFlags,
			uint32_t freqCode0,
			uint32_t freqCode1,

			RealType f0, RealType m0,
			RealType f1, RealType m1,
			RealType f2, RealType m2,
			RealType cf0,
			RealType cf1,
			RealType cf2,
			RealType cm,
			RealType cg,
			bool     csin,
			RealType scale,
			RealType max
	);


	*/


	DrxFrameGenerator fg(
			correlatorTest,
			useComplex,
			N,
			fs, decFactor,
			beam, timeOffset, statusFlags, fc0, fc1, f0,m0,f1,m1,f2,m2,cf0,cf1,cf2,cm,cg,csin,sig,scale);
	DrxFrame* f;


	char * ptr = new char [DataSize] ;
	for(unsigned int i=0;i<DataSize;i++){
		ptr[i]=(char)(i&0xff);
	}
	size_t * ts  =&(((size_t *)ptr)[0]);
	size_t * sid =&(((size_t *)ptr)[1]);
	size_t * key =&(((size_t *)ptr)[2]);


	Socket mySocket;
	mySocket.openOutbound(DestinationIpAddress.c_str(), DestinationPort);
	if (mySocket.outboundConnectionIsOpen() ){
		cout << "Send socket opened Successfully." << endl;
	} else {
		cout << "Could not open send socket to "<< DestinationIpAddress << " on port " << DestinationPort << endl;
		exit (EXIT_FAILURE);
	}
	/*
	if (looptest){
		mySocket.openInbound(DestinationPort);
		if (mySocket.inboundConnectionIsOpen()){
			cout << "Receive socket opened Successfully." << endl;
		} else {
			cout << "Could not open receive socket on port " << DestinationPort << endl;
			exit (EXIT_FAILURE);
		}
	}
*/
	TimeKeeper::resetRuntime();
	*sid = 0;
	*key = Key;
	uint64_t lastTimeTag=0;
	//uint64_t fcount=0;
	size_t bs = 0;
	fg.resetTimeTag(TimeKeeper::getTT());
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
			if(!DrxMode){
				cout << endl;
			}else{
				cout << "(timetag = " << humanReadable(lastTimeTag) << " ["<<lastTimeTag<<"] )" << endl;
			}
		}
		if(!DrxMode){
			*ts=TimeKeeper::getMPM();
			totalSent+=mySocket.send(ptr,DataSize);
			(*sid)++;
		} else {
			f = fg.next();
			//cout <<"freq code: " << dec << f->header.freqCode << endl;
			//checkIsCVframe(f);
			lastTimeTag = __builtin_bswap64(f->header.timeTag);
			bs = mySocket.send((char*)f,sizeof(DrxFrame));
			if (bs!=sizeof(DrxFrame)){
				cout << "Error in send.\n";
				return -1;
			}
			totalSent+=bs;
			/*
			if (looptest){
				bs = mySocket.receive((char*)&f_check,sizeof(DrxFrame));
				if (bs!=sizeof(DrxFrame)){
					cout << "Error in receive.\n";
					return -1;
				} else {
					if (memcmp((void*)f,(void*)&f_check,sizeof(DrxFrame))){
						cout << "Transmission error in loopback. frames not identical.\n";
						return -1;
					}
				}
			}*/


		}
		while ((((double)totalSent) /TimeKeeper::getRuntime()) > (double)Rate ){
			//wait
		}
	}

}
