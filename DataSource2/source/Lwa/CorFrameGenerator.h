/*
 * CorFrameGenerator.h
 *
 *  Created on: Oct 23, 2012
 *      Author: chwolfe2
 */

#ifndef CORFRAMEGENERATOR_H_
#define CORFRAMEGENERATOR_H_
#include "CorFrame.h"
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

class CorFrameGenerator {
public:
	static void fixByteOrder(CorFrame* frame);
	static void unfixByteOrder(CorFrame* frame);
	CorFrameGenerator(
			bool	 _bitPattern,
			bool	 _correlatorTest,
			bool	 _useComplex,
			uint64_t _numFrames,
			SignalGenerator* _sig
	);
	void generate();
	CorFrame * next();
	void resetTimeTag(uint64_t start);

	virtual ~CorFrameGenerator();
private:
	CorFrame*			frames;
	UnpackedSample 		samples[COR_SAMPLES_PER_FRAME];
	bool     bitPattern;
	bool     correlatorTest;
	bool	 useComplex;
	uint64_t numFrames;
	SignalGenerator* sig;
	uint64_t			start;
	void __pack(UnpackedSample* u, PackedSample64* p);
	void __printFrame(CorFrame* f, bool compact=false, bool single=false);
};

#endif /* CORFRAMEGENERATOR_H_ */
