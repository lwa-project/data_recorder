/*
 * TbtFrameGenerator.h
 *
 *  Created on: Oct 23, 2012
 *      Author: chwolfe2
 */

#ifndef TBTFRAMEGENERATOR_H_
#define TBTFRAMEGENERATOR_H_
#include "LWA.h"
#include "TbtFrame.h"
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

class TbtFrameGenerator {
public:
	static void fixByteOrder(TbtFrame* frame);
	static void unfixByteOrder(TbtFrame* frame);
	TbtFrameGenerator(
			bool	 _bitPattern,
			bool	 _correlatorTest,
			bool	 _useComplex,
			uint64_t _numFrames,
			SignalGenerator* _sig
	);
	void generate();
	TbtFrame * next();
	void resetTimeTag(uint64_t start);

	virtual ~TbtFrameGenerator();
private:
	TbtFrame*			frames;
	UnpackedSample 		samples[TBT_SAMPLES_PER_FRAME];
	bool     bitPattern;
	bool     correlatorTest;
	bool	 useComplex;
	uint64_t numFrames;
	SignalGenerator* sig;
	uint64_t			start;
	void __pack(UnpackedSample* u, PackedSample4* p);
	void __printFrame(TbtFrame* f, bool compact=false, bool single=false);
};

#endif /* TBTFRAMEGENERATOR_H_ */
