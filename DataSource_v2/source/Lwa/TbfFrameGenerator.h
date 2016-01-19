/*
 * TbfFrameGenerator.h
 *
 *  Created on: Oct 23, 2012
 *      Author: chwolfe2
 */

#ifndef TBFFRAMEGENERATOR_H_
#define TBFFRAMEGENERATOR_H_
#include "TbfFrame.h"
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

class TbfFrameGenerator {
public:
	static void fixByteOrder(TbfFrame* frame);
	static void unfixByteOrder(TbfFrame* frame);
	TbfFrameGenerator(
			bool	 _bitPattern,
			bool	 _correlatorTest,
			bool	 _useComplex,
			uint64_t _numFrames,
			SignalGenerator* _sig
	);
	void generate();
	TbfFrame * next();
	void resetTimeTag(uint64_t start);

	virtual ~TbfFrameGenerator();
private:
	TbfFrame*			frames;
	UnpackedSample 		samples[TBF_SAMPLES_PER_FRAME];
	bool     bitPattern;
	bool     correlatorTest;
	bool	 useComplex;
	uint64_t numFrames;
	SignalGenerator* sig;
	uint64_t			start;
	void __pack(UnpackedSample* u, PackedSample4* p);
	void __printFrame(TbfFrame* f, bool compact=false, bool single=false);
};

#endif /* TBFFRAMEGENERATOR_H_ */
