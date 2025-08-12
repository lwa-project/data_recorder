/*
 * TbnFrameGenerator.h
 *
 *  Created on: Oct 22, 2012
 *      Author: chwolfe2
 */

#ifndef TBNFRAMEGENERATOR_H_
#define TBNFRAMEGENERATOR_H_


#include "TbnFrame.h"
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

class TbnFrameGenerator {
public:
	static void fixByteOrder(TbnFrame* frame);
	static void unfixByteOrder(TbnFrame* frame);
	TbnFrameGenerator(
			bool     _bitPattern,
			bool	 _correlatorTest,
			bool	 _useComplex,
			uint64_t _numFrames,
			uint16_t _decFactor,
			SignalGenerator* _sig
	);
	void generate();
	TbnFrame * next();
	void resetTimeTag(uint64_t start);

	virtual ~TbnFrameGenerator();
private:
	TbnFrame*			frames;
	UnpackedSample 		samples[TBN_SAMPLES_PER_FRAME];
	bool     bitPattern;
	bool     correlatorTest;
	bool	 useComplex;
	uint64_t numFrames;
	uint16_t decFactor;

	SignalGenerator* sig;

	uint64_t			start;

	void __pack(UnpackedSample* u, PackedSample8* p);
	void __printFrame(TbnFrame* f, bool compact=false, bool single=false);
};

#endif /* TBNFRAMEGENERATOR_H_ */
