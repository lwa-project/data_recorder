/*
 * DrxFrameGenerator.cpp
 *
 *  Created on: Jan 29, 2012
 *      Author: chwolfe2
 */

#include "DrxFrameGenerator.hpp"
#include <assert.h>
#include <math.h>
#include "../TimeKeeper.h"
#include "LWA.h"

double DrxFrameGenerator::getFrequency(uint64_t fs, uint32_t freqCode){
	return ((double) freqCode) * ((double)fs) /((double)0x100000000);
}

DrxFrameGenerator::DrxFrameGenerator(
	bool     _bitPattern,
	bool	 _correlatorTest,
	bool	 _useComplex,
	uint64_t _numFrames,
	uint16_t _decFactor,
	uint8_t  _beam,
	uint16_t _timeOffset,
	uint32_t _statusFlags,
	uint32_t _freqCode0,
	uint32_t _freqCode1,
	SignalGenerator* _sig0,
	SignalGenerator* _sig1
) :
	samples(),
	bitPattern(_bitPattern),
	correlatorTest(_correlatorTest),
	useComplex(_useComplex),
	numFrames(_numFrames),
	decFactor(_decFactor),
	beam(_beam),				timeOffset(_timeOffset),		statusFlags(_statusFlags),
	freqCode0(_freqCode0),	freqCode1(_freqCode1),
	curFrame(0),
	start(TimeKeeper::getTT())
{
	/*
	adder.add(&sine0);
	adder.add(&sine1);
	adder.add(&sine2);
	adder.add(&gauss);
	adder.add(&chirp);
	*/
	sig[0]=_sig0;
	sig[1]=_sig1;
	for (int j=0; j<DRX_TUNINGS; j++){

		frames[j] = (DrxFrame*) malloc(numFrames* sizeof(DrxFrame));
		assert(frames[j]!=NULL);
		bzero((void*) frames[j], numFrames* sizeof(DrxFrame));
		cout << "allocated " << (numFrames* sizeof(DrxFrame)) << " bytes of storage (" << numFrames << " frames x " << sizeof(DrxFrame) << " bytes/frame.\n";
	}
	generate();
}

void DrxFrameGenerator::__pack(UnpackedSample* u, PackedSample8* p){
	int _i = (int)round(u->re);
	int _q = (int)round(u->im);
	p->i =  (((int8_t) _i) & 0xff);
	p->q =  (((int8_t) _q) & 0xff);
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
			printf("%02hhx %02hhx",(int)f->samples[i].i,(int)f->samples[i].q);
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
	try{
	size_t rptcnt = numFrames / 20;
	for (int tuning=0; tuning<DRX_TUNINGS; tuning++){
		for(size_t frm = 0; frm <numFrames; frm++){
			uint8_t  pol =     (uint8_t) (frm & 0x01lu);
			if (frm > rptcnt){
				rptcnt += numFrames / 20;
				cout << (double) frm * 100.0f / (double) numFrames << "% complete. \n";
			}
			if (bitPattern){
				for(size_t j=0; j<DRX_SAMPLES_PER_FRAME; j++){
					frames[tuning][frm].samples[j].packed = (uint8_t) ((j+frm) & 0xffll);
				}
			} else {
				switch(pol){
				case 0: /*X*/
					if (!correlatorTest){
						for(size_t j=0; j<DRX_SAMPLES_PER_FRAME; j++){
							size_t sampleNumber=((frm >> 2) * DRX_SAMPLES_PER_FRAME + j);
							double t = ((double)sampleNumber * (double)decFactor)/((double)DP_BASE_FREQ_HZ);
							UnpackedSample* temp = &samples[j];
							//*temp =adder.next(dt);
							*temp = sig[tuning]->sample(t);
							temp->i = temp->i / (sig[tuning]->getDynamicRange() / 128.0);
							temp->q = temp->q / (sig[tuning]->getDynamicRange() / 128.0);
							__pack(&samples[j],&frames[tuning][frm].samples[j]);
						}
					} else {
						for(size_t j=0; j<DRX_SAMPLES_PER_FRAME; j++){
							size_t sampleNumber=((frm >> 2) * DRX_SAMPLES_PER_FRAME + j);
							double t = ((double)sampleNumber * (double)decFactor)/((double)DP_BASE_FREQ_HZ);
							// generate y orthogonal to x, so correlation will be pure imaginary
							//UnpackedSample x = adder.next(dt);
							UnpackedSample x = sig[tuning]->sample(t);
							UnpackedSample y;
							y.re = -x.im  / (sig[tuning]->getDynamicRange() / 128.0);
							y.im = x.re  / (sig[tuning]->getDynamicRange() / 128.0);
							x.re = x.re / (sig[tuning]->getDynamicRange() / 128.0);
							x.im = x.im / (sig[tuning]->getDynamicRange() / 128.0);
							__pack(&y,&frames[tuning][frm + 1].samples[j]);
							__pack(&x,&frames[tuning][frm].samples[j]);
						}
					}
					break;
				case 1: /*Y*/
					if (!correlatorTest){
						// when not in correlator mode, Y=X
						memcpy((void*)&frames[tuning][frm].samples[0], (void*)&frames[tuning][ frm - 1 ].samples[0], DRX_FRAME_SIZE);
					} else {
						// do nothing, because we prepared this Y frame when X was being generated
					}
					break;
				}
			}

			uint64_t timetag = ((frm >> 2) * DRX_SAMPLES_PER_FRAME * ((uint64_t)decFactor));
			frames[tuning][frm].header.syncCode 	  		= 0x5CDEC0DE;
			frames[tuning][frm].header.frameCount   		= 0;
			frames[tuning][frm].header.drx_beam 	  		= this->beam + 1;
			frames[tuning][frm].header.drx_tuning   		= tuning + 1;
			frames[tuning][frm].header.drx_polarization 	= pol        + 0;
			frames[tuning][frm].header.decFactor	  		= this->decFactor;
			frames[tuning][frm].header.timeOffset   		= this->timeOffset;
			frames[tuning][frm].header.timeTag			= timetag;
			frames[tuning][frm].header.statusFlags		= this->statusFlags;
			frames[tuning][frm].header.freqCode     		= (!tuning) ? freqCode0 : freqCode1;
			fixByteOrder(&frames[tuning][frm]);
		}
	}
	cout << "Generation of " << numFrames << " frames complete ...\n";
	} catch(std::exception& e){
		cout << "Exception caught: '" << e.what() << "'\n";
		exit(-1);
	}
}

void DrxFrameGenerator::resetTimeTag(uint64_t start){
	this->start=start;
}
DrxFrame * DrxFrameGenerator::next(){
	static size_t sid = 0;
	static size_t cur_tuning = 0;
	static size_t cur_pol    = 0;
	static size_t cur_index  = 0;



	size_t index = ((cur_index << 1)+cur_pol) % numFrames;

	if (index>numFrames){
		cout << "index exceeds numFrames " << numFrames << " > " << numFrames<<". ( cur frame = "<<index<<")\n";
		exit(-1);
	}
	if (bitPattern){
		((size_t*)(&frames[cur_tuning][index].samples[0]))[0] = sid++;
	}

	frames[cur_tuning][index].header.timeTag = __builtin_bswap64(((curFrame>>2) * DRX_SAMPLES_PER_FRAME * ((uint64_t)decFactor))+start);

	DrxFrame * rv = &frames[cur_tuning][index];
	cur_pol++;
	if (cur_pol == DRX_POLARIZATIONS){
		cur_pol = 0;
		cur_tuning++;
		if (cur_tuning == DRX_TUNINGS){
			cur_tuning = 0;
			cur_index ++;
			if (cur_index == (numFrames/2)){
				cur_index = 0;
			}
		}
	}
	curFrame++;
	return rv;
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
