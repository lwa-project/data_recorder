/*
 * DrxFrameGenerator.cpp
 *
 *  Created on: Jan 29, 2012
 *      Author: chwolfe2
 */

#include "DrxFrameGenerator.hpp"
#include <assert.h>
#include <math.h>
#include "TimeKeeper.h"
/*
 *
#define fs    4096.0f
#define fs_2  (fs/2.0)
#define fs_4  (fs_2/2.0)
#define fs_8  (fs_4/2.0)
*/
RealType DrxFrameGenerator::getFrequency(uint64_t fs, uint32_t freqCode){
	return ((RealType) freqCode) * ((RealType)fs) /((RealType)0x100000000);
}

DrxFrameGenerator::DrxFrameGenerator(
	bool	 	correlatorTest,
	bool	 useComplex,
	uint64_t numFrames,
	RealType fs,
	uint16_t decFactor,
	uint8_t  beam,
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
) :
	frames(NULL),
	samples(),
	correlatorTest(correlatorTest),
	useComplex(useComplex),
	numFrames(numFrames),
	fs(fs),					decFactor(decFactor),
	beam(beam),				timeOffset(timeOffset),		statusFlags(statusFlags),
	freqCode0(freqCode0),	freqCode1(freqCode1),
	f0(f0),					m0(m0),
	f1(f1),					m1(m1),
	f2(f2),					m2(m2),
	cf0(cf0),		cf1(cf1),		cf2(cf2),		cm(cm),		cg(cg),		csin(csin),
	scale(scale),			max(max),
	sine0(useComplex,m0,f0),
	sine1(useComplex,m1,f1),
	sine2(useComplex,m2,f2),
	gauss(useComplex,scale),
	chirp(useComplex,cm,cf0,cf1,cf2,cg,csin),
	adder(),
	curFrame(0),
	start(TimeKeeper::getTT())
{
	adder.add(&sine0);
	adder.add(&sine1);
	adder.add(&sine2);
	adder.add(&gauss);
	adder.add(&chirp);
	frames = (DrxFrame*) malloc(numFrames* sizeof(DrxFrame));
	assert(frames!=NULL);
	cout << "allocated " << (numFrames* sizeof(DrxFrame)) << " bytes of storage (" << numFrames << " frames x " << sizeof(DrxFrame) << " bytes/frame.\n";

	generate();
}

void DrxFrameGenerator::__pack(UnpackedSample* u, PackedSample* p){
	int _i = (int)round(u->re);
	if(_i>7) _i=7;
	if(_i<-8) _i=-8;
	int _q = (int)round(u->im);
	if(_q>7) _q=7;
	if(_q<-8) _q=-8;
	p->i =  (((int8_t) _i) & 0xf);
	p->q =  (((int8_t) _q) & 0xf);
}
void DrxFrameGenerator::__printFrame(DrxFrame* f, bool compact, bool single){
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
		for(int i=0; i<1024; i++){
			printf("%02hhx ",(int)f->samples[i].packed);
			if (((i & 0xf) == 0xf) || single){
				cout << endl;
			}
		}
		//cout << " ... << additional data truncated >> " << endl;
		cout << "==============================================================================" << endl;
		cout << "<<< frame size = "<<sizeof(DrxFrame)<<dec<<">>>\n";
		DrxFrameGenerator::fixByteOrder(f);
	}
}

void DrxFrameGenerator::generate(){
	cout << "Beginning generation of " << numFrames << " frames.\n";
	cout << "debug: fc0=" << dec << freqCode0 << "; fc1=" << dec << freqCode1 << endl;
	size_t rptcnt = numFrames / 20;
	for(size_t frm = 0; frm <numFrames; frm++){
		uint8_t  tuning =  (uint8_t) ((frm >> 1) & 0x01lu);
		uint8_t  pol =     (uint8_t) ((frm >> 0) & 0x01lu);
		if (frm > rptcnt){
			rptcnt += numFrames / 20;
			cout << (double) frm * 100.0f / (double) numFrames << "% complete. \n";
		}
		RealType dt = 1.0/fs;
		uint8_t stream = pol + (2*tuning);
		switch(stream){
		case 0: /*1X*/
			if (!correlatorTest){
				for(size_t j=0; j<DRX_SAMPLES_PER_FRAME; j++){
					UnpackedSample* temp = &samples[j];
					*temp =adder.next(dt);
					temp->i = temp->i / (max / 8.0);
					temp->q = temp->q / (max / 8.0);
					__pack(&samples[j],&frames[frm].samples[j]);
				}
			} else {
				for(size_t j=0; j<DRX_SAMPLES_PER_FRAME; j++){
					// generate y orthogonal to x, so correlation will be pure imaginary
					UnpackedSample x = adder.next(dt);
					UnpackedSample y;
					y.re = -x.im  / (max / 8.0);
					y.im = x.re  / (max / 8.0);
					x.re = x.re / (max / 8.0);
					x.im = x.im / (max / 8.0);
					__pack(&y,&frames[frm + 1].samples[j]);
					__pack(&x,&frames[frm].samples[j]);
				}
			}
			break;
		case 1: /*1Y*/
			if (!correlatorTest){
				// when not in correlator mode, Y=X
				memcpy((void*)&frames[frm].samples[0], (void*)&frames[ frm - 1 ].samples[0], DRX_SAMPLES_PER_FRAME*sizeof(PackedSample));
			} else {
				// do nothing, because we prepared this Y frame when X was being generated
			}
			break;
		case 2: /*2X*/ // fall through, 2nd tuning duplicates the first
		case 3: /*2Y*/
			memcpy((void*)&frames[frm].samples[0], (void*)&frames[ frm - 2 ].samples[0], DRX_SAMPLES_PER_FRAME*sizeof(PackedSample));
			break;
		}
		uint64_t timetag = ((frm >> 2) * DRX_SAMPLES_PER_FRAME * ((uint64_t)decFactor));
		frames[frm].header.syncCode 	  		= 0x5CDEC0DE;
		frames[frm].header.frameCount   		= 0;
		frames[frm].header.drx_beam 	  		= this->beam + 1;
		frames[frm].header.drx_tuning   		= tuning + 1;
		frames[frm].header.drx_polarization 	= pol        + 0;
		frames[frm].header.decFactor	  		= this->decFactor;
		frames[frm].header.timeOffset   		= this->timeOffset;
		frames[frm].header.timeTag				= timetag;
		frames[frm].header.statusFlags			= this->statusFlags;
		frames[frm].header.freqCode     		= (!tuning) ? freqCode0 : freqCode1;
		fixByteOrder(&frames[frm]);
	}
	cout << "Generation of " << numFrames << " frames complete ...\n";
}

void DrxFrameGenerator::resetTimeTag(uint64_t start){
	this->start=start;
}
DrxFrame * DrxFrameGenerator::next(){
	size_t index = curFrame % numFrames;
	if (index>numFrames){
		cout << "index exceeds numFrames " << index << " > " << numFrames<<". ( cur frame = "<<curFrame<<")\n";
		exit(-1);
	}
	frames[index].header.timeTag = __builtin_bswap64(((curFrame>>2) * DRX_SAMPLES_PER_FRAME * ((uint64_t)decFactor)+start));
	curFrame++;
	return &frames[index];
}

void DrxFrameGenerator::fixByteOrder(DrxFrame* frame){
	frame->header.freqCode   = __builtin_bswap32(frame->header.freqCode);
	frame->header.decFactor  = (frame->header.decFactor << 8)  | (frame->header.decFactor >> 8);
	frame->header.timeOffset = (frame->header.timeOffset << 8) | (frame->header.timeOffset >> 8);
	frame->header.timeTag    = __builtin_bswap64(frame->header.timeTag);
}
void DrxFrameGenerator::unfixByteOrder(DrxFrame* frame){
	fixByteOrder(frame);
}

DrxFrameGenerator::~DrxFrameGenerator(){
	if(frames) free(frames);
}
