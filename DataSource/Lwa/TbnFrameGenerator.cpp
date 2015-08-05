/*
 * TbnFrameGenerator.cpp
 *
 *  Created on: Oct 22, 2012
 *      Author: chwolfe2
 */

#include "TbnFrameGenerator.h"

#include <assert.h>
#include <math.h>
#include "../TimeKeeper.h"
#include "LWA.h"

TbnFrameGenerator::TbnFrameGenerator(
		bool     _bitPattern,
		bool	 _correlatorTest,
		bool	 _useComplex,
		uint64_t _numFrames,
		uint16_t _decFactor,
		SignalGenerator* _sig
) :
	frames(NULL),
	samples(),
	bitPattern(_bitPattern),
	correlatorTest(_correlatorTest),
	useComplex(_useComplex),
	numFrames(_numFrames),
	decFactor(_decFactor),
	sig(_sig),
	start(TimeKeeper::getTT())
{
	frames = (TbnFrame*) malloc(numFrames* sizeof(TbnFrame));
	assert(frames!=NULL);
	bzero((void*) frames, numFrames* sizeof(TbnFrame));
	cout << "allocated " << (numFrames* sizeof(TbnFrame)) << " bytes of storage (" << numFrames << " frames x " << sizeof(TbnFrame) << " bytes/frame.\n";

	generate();
}

void TbnFrameGenerator::__pack(UnpackedSample* u, PackedSample8* p){
	int _i = (int)round(u->re);
	if(_i>127) _i=127;
	if(_i<-128) _i=-128;
	int _q = (int)round(u->im);
	if(_q>127) _q=127;
	if(_q<-128) _q=-128;
	p->i =  (((int8_t) _i) & 0xff);
	p->q =  (((int8_t) _q) & 0xff);
}
void TbnFrameGenerator::__printFrame(TbnFrame* f, bool compact, bool single){
	if (compact){
		for(int i=0; i<TBN_SAMPLES_PER_FRAME; i++){
			printf("%3hd %3hd ",(int)f->samples[i].i,(int)f->samples[i].q);
			if (((i & 0xf) == 0xf) || single){
				cout << endl;
			}
		}
	} else {
		TbnFrameGenerator::unfixByteOrder(f);
		cout << "=============================================================================="   << endl;
		cout << "== Polarization:      " << ((f->header.tbn_stand_pol & 1lu) ? "X" : "Y")          << endl;
		cout << "== Time Tag:          " << (long int) f->header.timeTag                    << dec << endl;
		cout << "== Stand:             " << (int)      (((f->header.tbn_stand_pol-1)>>1)+1) << dec << endl;
		cout << "==============================================================================" << endl;
		for(int i=0; i<256; i++){
			printf("%02hhx ",(int)f->samples[i].packed);
			if (((i & 0xf) == 0xf) || single){
				cout << endl;
			}
		}
		cout << " ... << additional data truncated >> " << endl;
		cout << "==============================================================================" << endl;
		cout << "<<< frame size = "<<sizeof(TbnFrame)<<dec<<">>>\n";
		TbnFrameGenerator::fixByteOrder(f);
	}
}

void TbnFrameGenerator::generate(){
	cout << "Beginning generation of " << numFrames << " frames.\n";
	try{
	size_t rptcnt = numFrames / 20;
	for(size_t frm = 0; frm <numFrames; frm++){
		uint8_t  pol = (uint8_t) (frm & 0x01lu);
		if (frm > rptcnt){
			rptcnt += numFrames / 20;
			cout << (double) frm * 100.0f / (double) numFrames << "% complete. \n";
		}
		if(bitPattern){
			for(size_t j=0; j<TBN_SAMPLES_PER_FRAME; j++){
				frames[frm].samples[j].packed = (uint8_t) ((j+frm) & 0xffll);
			}
		}else{
			uint8_t stream = pol;
			switch(stream){
			case 0: /*X*/
				if (!correlatorTest){
					for(size_t j=0; j<TBN_SAMPLES_PER_FRAME; j++){
						size_t sampleNumber=((frm >> 2) * TBN_SAMPLES_PER_FRAME + j);
						double t = ((double)sampleNumber * (double)decFactor)/((double)DP_BASE_FREQ_HZ);
						UnpackedSample* temp = &samples[j];
						//*temp =adder.next(dt);
						*temp = sig->sample(t);
						temp->i = temp->i / (sig->getDynamicRange() / 128.0);
						temp->q = temp->q / (sig->getDynamicRange() / 128.0);
						__pack(&samples[j],&frames[frm].samples[j]);
					}
				} else {
					for(size_t j=0; j<TBN_SAMPLES_PER_FRAME; j++){
						size_t sampleNumber=((frm >> 2) * TBN_SAMPLES_PER_FRAME + j);
						double t = ((double)sampleNumber * (double)decFactor)/((double)DP_BASE_FREQ_HZ);
						// generate y orthogonal to x, so correlation will be pure imaginary
						//UnpackedSample x = adder.next(dt);
						UnpackedSample x = sig->sample(t);
						UnpackedSample y;
						y.re = -x.im / (sig->getDynamicRange() / 128.0);
						y.im =  x.re / (sig->getDynamicRange() / 128.0);
						x.re =  x.re / (sig->getDynamicRange() / 128.0);
						x.im =  x.im / (sig->getDynamicRange() / 128.0);
						__pack( &y, &frames[frm+1].samples[j]);
						__pack( &x, &frames[frm].samples[j]  );
					}
				}
				break;
			case 1: /*Y*/
				if (!correlatorTest){
					// when not in correlator mode, Y=X
					memcpy((void*)&frames[frm].samples[0], (void*)&frames[ frm - 1 ].samples[0], TBN_FRAME_SIZE);
				} else {
					// do nothing, because we prepared this Y frame when X was being generated
				}
				break;
			}
		}
		frames[frm].header.syncCode 	  		= 0x5CDEC0DE;
		frames[frm].header.frameCount   		= 0;
		frames[frm].header.timeTag				= 0; // fixup on next()
		frames[frm].header.secondsCount         = 0;
		frames[frm].header.id                   = 0;
		frames[frm].header.tbn_id               = 0;
		frames[frm].header.tbn_tbn_bit_n        = 0;
		frames[frm].header.tbn_stand_pol        = 0; // fixup on next()
		fixByteOrder(&frames[frm]);
	}

	cout << "Generation of " << numFrames << " frames complete ...\n";
	} catch(std::exception& e){
		cout << "Exception caught: '" << e.what() << "'\n";
		exit(-1);
	}
}

void TbnFrameGenerator::resetTimeTag(uint64_t start){
	this->start=start;
}
TbnFrame * TbnFrameGenerator::next(){

	static size_t sid       = 0; // 0 - 259
	static size_t cur_stand = 0; // 0 - 259
	static size_t cur_pol   = 0; // 0 for x, 1 for y
	static size_t cur_frame = 0;
	size_t actualFrames = numFrames >> 1;

	size_t index = ((cur_frame << 1)+cur_pol) % numFrames;
	if (index>numFrames){
		cout << "index exceeds numFrames " << index << " > " << numFrames<<". ( cur frame = "<<cur_frame<<")\n";
		exit(-1);
	}
	frames[index].header.tbn_stand_pol     = (cur_stand*2+(cur_pol))+1;
	frames[index].header.tbn_tbn_bit_n     = 0;
	frames[index].header.timeTag           = cur_frame * TBN_SAMPLES_PER_FRAME * ((uint64_t)decFactor) + start;
	fixByteOrder(&frames[index]);
	if (bitPattern){
		((size_t*)(&frames[index].samples[0]))[0] = sid++;
	}
	cur_pol++;
	if (cur_pol == 2){
		cur_pol = 0;
		cur_stand++;
		if (cur_stand == 260){
			cur_stand = 0;
			cur_frame++;
			if (cur_frame == actualFrames+1){
				cur_frame = 0;
			}
		}
	}
	return &frames[index];
}

void TbnFrameGenerator::fixByteOrder(TbnFrame* frame){
	frame->header.tbn_id = (frame->header.tbn_id << 8) | (frame->header.tbn_id >> 8);
	frame->header.timeTag    = __builtin_bswap64(frame->header.timeTag);
}
void TbnFrameGenerator::unfixByteOrder(TbnFrame* frame){
	fixByteOrder(frame);
}

TbnFrameGenerator::~TbnFrameGenerator(){
	if(frames) free(frames);
}
