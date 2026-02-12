/*
 * TbsFrameGenerator.h
 *
 *  Created on: Oct 23, 2012
 *      Author: chwolfe2
 */

#ifndef TBSFRAMEGENERATOR_H_
#define TBSFRAMEGENERATOR_H_
#include "LWA.h"
#include "TbsFrame.h"
#include <stdint.h>
#include <iostream>
#include <cstring>
#include <malloc.h>
#include "../Signals/SignalAdder.hpp"
#include "../Signals/Noise.hpp"
#include "../Signals/SineGenerator.hpp"
#include "../Signals/SignalGenerator.hpp"
#include "../Signals/GaussianGenerator.hpp"
#include "../Signals/ChirpGenerator.h"
using namespace std;

class TbsFrameGenerator {
public:
	static void fixByteOrder(TbsFrame* frame);
	static void unfixByteOrder(TbsFrame* frame);
	TbsFrameGenerator(
			bool	 _bitPattern,
			bool	 _correlatorTest,
			bool	 _useComplex,
			uint64_t _numFrames,
			SignalGenerator* _sig
	);
	void generate();
	TbsFrame * next();
	void resetTimeTag(uint64_t start);

	virtual ~TbsFrameGenerator();
private:
	TbsFrame*			frames;
	UnpackedSample 		samples[TBS_SAMPLES_PER_FRAME];
	bool     bitPattern;
	bool     correlatorTest;
	bool	 useComplex;
	uint64_t numFrames;
	SignalGenerator* sig;
	uint64_t			start;
	void __pack(UnpackedSample* u, PackedSample4* p);
	void __printFrame(TbsFrame* f, bool compact=false, bool single=false);
};

#endif /* TBSFRAMEGENERATOR_H_ */
