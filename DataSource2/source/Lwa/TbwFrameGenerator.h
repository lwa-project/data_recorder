/*
 * TbwFrameGenerator.h
 *
 *  Created on: Oct 23, 2012
 *      Author: chwolfe2
 */

#ifndef TBWFRAMEGENERATOR_H_
#define TBWFRAMEGENERATOR_H_
#include "TbwFrame.h"
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

class TbwFrameGenerator {
public:
	static void fixByteOrder(TbwFrame* frame);
	static void unfixByteOrder(TbwFrame* frame);
	TbwFrameGenerator(
			bool     _bitPattern,
			bool	 _correlatorTest,
			bool	 _useComplex,
			uint64_t _numFrames,
			SignalGenerator* _sig
	);
	void generate();
	TbwFrame * next();
	void resetTimeTag(uint64_t start);

	virtual ~TbwFrameGenerator();
private:
	TbwFrame*			frames;
	UnpackedSample 		samples[TBW_SAMPLES_PER_FRAME_4BIT];
	bool     bitPattern;
	bool     correlatorTest;
	bool	 useComplex;
	uint64_t numFrames;
	SignalGenerator* sig;
	uint64_t			start;
	void __pack(UnpackedSample* u, PackedSample4* p);
	void __printFrame(TbwFrame* f, bool compact=false, bool single=false);
};

#endif /* TBWFRAMEGENERATOR_H_ */
