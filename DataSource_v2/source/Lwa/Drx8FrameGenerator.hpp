/*
 * Drx8FrameGenerator.h
 *
 *  Created on: Oct 20, 2015
 *      Author: J. Dowell
 */

#ifndef DRX8FRAMEGENERATOR_H_
#define DRX8FRAMEGENERATOR_H_
#include "Drx8Frame.h"
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

class Drx8FrameGenerator {
public:
	static void fixByteOrder(Drx8Frame* frame);
	static void unfixByteOrder(Drx8Frame* frame);
	static double getFrequency(uint64_t fs, uint32_t freqCode);
	Drx8FrameGenerator(
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
	Drx8Frame * next();
	void resetTimeTag(uint64_t start);

	virtual ~Drx8FrameGenerator();
private:
	Drx8Frame*			frames[DRX8_TUNINGS];
	UnpackedSample 		samples[DRX8_SAMPLES_PER_FRAME];
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
	SignalGenerator* sig[DRX8_TUNINGS];
	uint64_t			curFrame;
	uint64_t			start;
	void __pack(UnpackedSample* u, PackedSample8* p);
	void __printFrame(Drx8Frame* f, bool compact=false, bool single=false);
};

#endif /* DRX8FRAMEGENERATOR_H_ */
