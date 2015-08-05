#include "TbwFrameGenerator.h"

#include <assert.h>
#include <math.h>
#include "../TimeKeeper.h"
#include "LWA.h"

TbwFrameGenerator::TbwFrameGenerator(
		bool     _bitPattern,
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
	frames = (TbwFrame*) malloc(numFrames* sizeof(TbwFrame));
	assert(frames!=NULL);
	bzero((void*) frames, numFrames* sizeof(TbwFrame));
	cout << "allocated " << (numFrames* sizeof(TbwFrame)) << " bytes of storage (" << numFrames << " frames x " << sizeof(TbwFrame) << " bytes/frame.\n";

	generate();
}

void TbwFrameGenerator::__pack(UnpackedSample* u, PackedSample4* p){
	int _i = (int)round(u->re);
	if(_i>7) _i=7;
	if(_i<-8) _i=-8;
	int _q = (int)round(u->im);
	if(_q>7) _q=7;
	if(_q<-8) _q=-8;
	p->i =  (((int8_t) _i) & 0xf);
	p->q =  (((int8_t) _q) & 0xf);
}
void TbwFrameGenerator::__printFrame(TbwFrame* f, bool compact, bool single){
	if (compact){
		for(int i=0; i<TBW_SAMPLES_PER_FRAME_4BIT; i++){
			printf("%3hd %3hd ",(int)f->samples_4bit[i].i,(int)f->samples_4bit[i].q);
			if (((i & 0xf) == 0xf) || single){
				cout << endl;
			}
		}
	} else {
		TbwFrameGenerator::unfixByteOrder(f);
		cout << "==============================================================================" << endl;
		cout << "== Time Tag:          " << (long int) f->header.timeTag                  << dec << endl;
		cout << "== Stand:             " << (int)      f->header.tbw_stand                << dec << endl;
		cout << "==============================================================================" << endl;
		for(int i=0; i<TBW_SAMPLES_PER_FRAME_4BIT; i++){
			printf("%02hhx ",(int)f->samples_4bit[i].packed);
			if (((i & 0xf) == 0xf) || single){
				cout << endl;
			}
		}
		//cout << " ... << additional data truncated >> " << endl;
		cout << "==============================================================================" << endl;
		cout << "<<< frame size = "<<sizeof(TbwFrame)<<dec<<">>>\n";
		TbwFrameGenerator::fixByteOrder(f);
	}
}

void TbwFrameGenerator::generate(){
	cout << "Beginning generation of " << numFrames << " frames.\n";
	try{
	size_t rptcnt = numFrames / 20;
	for(size_t frm = 0; frm <numFrames; frm++){
		if (frm > rptcnt){
			rptcnt += numFrames / 20;
			cout << (double) frm * 100.0f / (double) numFrames << "% complete. \n";
		}
		if 	(bitPattern){
			for(size_t j=0; j<TBW_SAMPLES_PER_FRAME_4BIT; j++){
				frames[frm].samples_4bit[j].packed = (uint8_t) ((j+frm) & 0xffll);
			}
		} else {
			for(size_t j=0; j<TBW_SAMPLES_PER_FRAME_4BIT; j+=2){
				size_t sampleNumber=((frm >> 2) * TBW_SAMPLES_PER_FRAME_4BIT + j);
				double t = ((double)sampleNumber)/((double)DP_BASE_FREQ_HZ);
				if (!correlatorTest){
					UnpackedSample* temp = &samples[j];
					//*temp =adder.next(dt);
					*temp = sig->sample(t);
					temp->i = temp->i / (sig->getDynamicRange() / 8.0);
					temp->q = temp->q / (sig->getDynamicRange() / 8.0);
					__pack(&samples[j],&frames[frm].samples_4bit[j]);
					__pack(&samples[j],&frames[frm].samples_4bit[j+1]);
				} else {
					// generate y orthogonal to x, so correlation will be pure imaginary
					//UnpackedSample x = adder.next(dt);
					UnpackedSample x = sig->sample(t);
					UnpackedSample y;
					y.re = -x.im / (sig->getDynamicRange() / 8.0);
					y.im =  x.re / (sig->getDynamicRange() / 8.0);
					x.re =  x.re / (sig->getDynamicRange() / 8.0);
					x.im =  x.im / (sig->getDynamicRange() / 8.0);
					__pack(&x,&frames[frm].samples_4bit[j]);
					__pack(&y,&frames[frm].samples_4bit[j+1]);
				}
			}
		}

		frames[frm].header.syncCode 	  		= 0x5CDEC0DE;
		frames[frm].header.frameCount   		= 0;
		frames[frm].header.timeTag				= 0; // fixup on next()
		frames[frm].header.secondsCount         = 0;
		frames[frm].header.id                   = 0;
		frames[frm].header.tbw_id               = 0;
		fixByteOrder(&frames[frm]);
	}

	cout << "Generation of " << numFrames << " frames complete ...\n";
	} catch(std::exception& e){
		cout << "Exception caught: '" << e.what() << "'\n";
		exit(-1);
	}
}

void TbwFrameGenerator::resetTimeTag(uint64_t start){
	this->start=start;
}
TbwFrame * TbwFrameGenerator::next(){
	static size_t sid = 0;
	static size_t cur_stand = 0; // 0 - 259
	static size_t cur_frame = 0;
	size_t actualFrames = numFrames >> 1;

	size_t index = cur_frame % numFrames;
	if (index>numFrames){
		cout << "index exceeds numFrames " << index << " > " << numFrames<<". ( cur frame = "<<cur_frame<<")\n";
		exit(-1);
	}
	frames[index].header.tbw_stand         = (cur_stand*2)+1;
	frames[index].header.tbw_tbw_bit       = 1;
	frames[index].header.tbw_4bit          = 1;
	frames[index].header.timeTag           = cur_frame * TBW_SAMPLES_PER_FRAME_4BIT + start;
	fixByteOrder(&frames[index]);
	if (bitPattern){
		((size_t*)(&frames[index].samples_4bit[0]))[0] = sid++;
	}
	cur_stand++;
	if (cur_stand == 260){
		cur_stand = 0;
		cur_frame++;
		if (cur_frame == actualFrames+1){
			cur_frame = 0;
		}
	}
	return &frames[index];
}

void TbwFrameGenerator::fixByteOrder(TbwFrame* frame){
	frame->header.tbw_id = (frame->header.tbw_id << 8) | (frame->header.tbw_id >> 8);
	frame->header.timeTag    = __builtin_bswap64(frame->header.timeTag);
}
void TbwFrameGenerator::unfixByteOrder(TbwFrame* frame){
	fixByteOrder(frame);
}

TbwFrameGenerator::~TbwFrameGenerator(){
	if(frames) free(frames);
}
