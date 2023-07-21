/*
 * DrxFrameGenerator.h
 *
 *  Created on: Jan 29, 2012
 *      Author: chwolfe2
 */

#ifndef DRXFRAMEGENERATOR_H_
#define DRXFRAMEGENERATOR_H_
#include "DrxFrame.h"
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

class DrxFrameGenerator {
public:
	static void fixByteOrder(DrxFrame* frame);
	static void unfixByteOrder(DrxFrame* frame);
	static double getFrequency(uint64_t fs, uint32_t freqCode);
	DrxFrameGenerator(
			bool     _bitPattern,
			bool     _correlatorTest,
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
	);
	void generate();
	DrxFrame * next();
	void resetTimeTag(uint64_t start);

	virtual ~DrxFrameGenerator();
private:
	DrxFrame*			frames[DRX_TUNINGS];
	UnpackedSample 		samples[DRX_SAMPLES_PER_FRAME];
	bool     bitPattern;
	bool     correlatorTest;
	bool	 useComplex;
	uint64_t numFrames;
	uint16_t decFactor;
	uint8_t beam;
	uint16_t timeOffset;
	uint32_t statusFlags;
	uint32_t freqCode0;
	uint32_t freqCode1;
	SignalGenerator* sig[DRX_TUNINGS];
	uint64_t			curFrame;
	uint64_t			start;
	void __pack(UnpackedSample* u, PackedSample8* p);
	void __printFrame(DrxFrame* f, bool compact=false, bool single=false);
};

#endif /* DRXFRAMEGENERATOR_H_ */
