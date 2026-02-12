#include "TbsFrameGenerator.hpp"

#include <assert.h>
#include <math.h>
#include "../TimeKeeper.h"
#include "LWA.h"


static inline unsigned short __builtin_bswap16(unsigned short a)
{
    return (a<<8)|(a>>8);
}


TbsFrameGenerator::TbsFrameGenerator(
		bool	 _bitPattern,
		bool	 _correlatorTest,
		bool	 _useComplex,
		uint64_t _numFrames,
		SignalGenerator* _sig
) :
	frames(NULL),
	samples(),
	bitPattern(_bitPattern),
	correlatorTest(_correlatorTest),
	useComplex(_useComplex),
	numFrames(_numFrames),
	sig(_sig),
	start(TimeKeeper::getTT())
{
	frames = (TbsFrame*) malloc(numFrames* sizeof(TbsFrame));
	assert(frames!=NULL);
	bzero((void*) frames, numFrames* sizeof(TbsFrame));
	cout << "allocated " << (numFrames* sizeof(TbsFrame)) << " bytes of storage (" << numFrames << " frames x " << sizeof(TbsFrame) << " bytes/frame.\n";

	generate();
}

void TbsFrameGenerator::__pack(UnpackedSample* u, PackedSample4* p){
	int _i = (int)round(u->re);
	if(_i>7) _i=7;
	if(_i<-8) _i=-8;
	int _q = (int)round(u->im);
	if(_q>7) _q=7;
	if(_q<-8) _q=-8;
	p->i =  (((int8_t) _i) & 0xf);
	p->q =  (((int8_t) _q) & 0xf);
}
void TbsFrameGenerator::__printFrame(TbsFrame* f, bool compact, bool single){
	if (compact){
		for(int i=0; i<TBS_SAMPLES_PER_FRAME; i++){
			printf("%3hd %3hd ",(int)f->samples[i].i,(int)f->samples[i].q);
			if (((i & 0xf) == 0xf) || single){
				cout << endl;
			}
		}
	} else {
		TbsFrameGenerator::unfixByteOrder(f);
		cout << "==============================================================================" << endl;
		cout << "== Time Tag:          " << (long int) f->header.timeTag                  << dec << endl;
		cout << "== Channel:           " << (int)      f->header.freq_chan                << dec << endl;
		cout << "==============================================================================" << endl;
		for(int i=0; i<TBS_SAMPLES_PER_FRAME; i++){
			printf("%02hhx ",(int)f->samples[i].packed);
			if (((i & 0xf) == 0xf) || single){
				cout << endl;
			}
		}
		//cout << " ... << additional data truncated >> " << endl;
		cout << "==============================================================================" << endl;
		cout << "<<< frame size = "<<sizeof(TbsFrame)<<dec<<">>>\n";
		TbsFrameGenerator::fixByteOrder(f);
	}
}

void TbsFrameGenerator::generate(){
	cout << "Beginning generation of " << numFrames << " frames.\n";
	try{
	size_t rptcnt = numFrames / 20;
	for(size_t frm = 0; frm <numFrames; frm++){
		if (frm > rptcnt){
			rptcnt += numFrames / 20;
			cout << (double) frm * 100.0f / (double) numFrames << "% complete. \n";
		}
		if 	(bitPattern){
			for(size_t j=0; j<TBS_SAMPLES_PER_FRAME; j++){
				frames[frm].samples[j].packed = (uint8_t) ((j+frm) & 0xffll);
			}
		} else {
			for(size_t j=0; j<TBS_SAMPLES_PER_FRAME; j+=2){
				size_t sampleNumber=((frm >> 2) * TBS_SAMPLES_PER_FRAME + j);
				double t = ((double)sampleNumber)/((double)DP_BASE_FREQ_HZ);
				if (!correlatorTest){
					UnpackedSample* temp = &samples[j];
					//*temp =adder.next(dt);
					*temp = sig->sample(t);
					temp->i = temp->i / (sig->getDynamicRange() / 8.0);
					temp->q = temp->q / (sig->getDynamicRange() / 8.0);
					__pack(&samples[j],&frames[frm].samples[j]);
					__pack(&samples[j],&frames[frm].samples[j+1]);
				} else {
					// generate y orthogonal to x, so correlation will be pure imaginary
					//UnpackedSample x = adder.next(dt);
					UnpackedSample x = sig->sample(t);
					UnpackedSample y;
					y.re = -x.im / (sig->getDynamicRange() / 8.0);
					y.im =  x.re / (sig->getDynamicRange() / 8.0);
					x.re =  x.re / (sig->getDynamicRange() / 8.0);
					x.im =  x.im / (sig->getDynamicRange() / 8.0);
					__pack(&x,&frames[frm].samples[j]);
					__pack(&y,&frames[frm].samples[j+1]);
				}
			}
		}

		frames[frm].header.syncCode 	  		= 0x5CDEC0DE;
		frames[frm].header.frameCount   		= 0; // fixup on next()
		frames[frm].header.timeTag			= 0; // fixup on next()
		frames[frm].header.secondsCount		= 0; // fixup on next()
		frames[frm].header.id				= 0; // fixup on next()
		frames[frm].header.freq_chan			= 0; // fixup on next()
		fixByteOrder(&frames[frm]);
	}

	cout << "Generation of " << numFrames << " frames complete ...\n";
	} catch(std::exception& e){
		cout << "Exception caught: '" << e.what() << "'\n";
		exit(-1);
	}
}

void TbsFrameGenerator::resetTimeTag(uint64_t start){
	this->start=start;
}
TbsFrame * TbsFrameGenerator::next(){
	static size_t sid = 0;
	static size_t cur_band = 0; // 0 - 127
	static size_t cur_frame = 0;
	size_t actualFrames = numFrames >> 1;
	
	size_t index = cur_frame % numFrames;
	if (index>numFrames){
		cout << "index exceeds numFrames " << index << " > " << numFrames<<". ( cur frame = "<<cur_frame<<")\n";
		exit(-1);
	}
	
	frames[index].header.frameCount        = cur_frame | (1<<24);
	frames[index].header.secondsCount      = (cur_frame * 8192*1960/2048 + start) / 196000000;
	frames[index].header.freq_chan         = cur_band*12;
	frames[index].header.timeTag           = cur_frame * 8192*1960/2048 + start;
	fixByteOrder(&frames[index]);
	if (bitPattern){
		((size_t*)(&frames[index].samples[0]))[0] = sid++;
	}
	cur_band++;
	if (cur_band == 128){
		cur_band = 0;
		cur_frame++;
		if (cur_frame == actualFrames+1){
			cur_frame = 0;
		}
	}
	return &frames[index];
}

void TbsFrameGenerator::fixByteOrder(TbsFrame* frame){
	frame->header.freq_chan  = __builtin_bswap16(frame->header.freq_chan);
	frame->header.frameCount = __builtin_bswap32(frame->header.frameCount);
	frame->header.secondsCount = __builtin_bswap32(frame->header.secondsCount);
	frame->header.timeTag    = __builtin_bswap64(frame->header.timeTag);
}
void TbsFrameGenerator::unfixByteOrder(TbsFrame* frame){
	fixByteOrder(frame);
}

TbsFrameGenerator::~TbsFrameGenerator(){
	if(frames) free(frames);
}
