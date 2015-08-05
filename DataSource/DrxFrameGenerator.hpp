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
#include "SignalAdder.hpp"
#include "Noise.hpp"
#include "SineGenerator.hpp"
#include "SignalGenerator.hpp"
#include "GaussianGenerator.hpp"
#include "ChirpGenerator.h"
using namespace std;

class DrxFrameGenerator {
public:
	static void fixByteOrder(DrxFrame* frame);
	static void unfixByteOrder(DrxFrame* frame);
	static RealType getFrequency(uint64_t fs, uint32_t freqCode);
	DrxFrameGenerator(
			bool     correlatorTest,
			bool	 useComplex,
			uint64_t numFrames,
			RealType fs,
			uint16_t decFactor,
			uint8_t beam,
			uint16_t timeOffset,
			uint32_t statusFlags,
			uint32_t freqCode0,
			uint32_t freqCode1,

			RealType f0, RealType m0,
			RealType f1, RealType m1,
			RealType f2, RealType m2,
			RealType cf0,
			RealType cf1,
			RealType cf2,
			RealType cm,
			RealType cg,
			bool     csin,
			RealType scale,
			RealType max
	);
	void generate();
	DrxFrame * next();
	void resetTimeTag(uint64_t start);

	virtual ~DrxFrameGenerator();
private:
	DrxFrame*			frames;
	UnpackedSample 		samples[4096];
	bool     correlatorTest;
	bool	 useComplex;
	uint64_t numFrames;
	RealType fs;
	uint16_t decFactor;
	uint8_t beam;
	uint16_t timeOffset;
	uint32_t statusFlags;
	uint32_t freqCode0;
	uint32_t freqCode1;

	RealType f0; RealType m0;
	RealType f1; RealType m1;
	RealType f2; RealType m2;
	RealType cf0;
	RealType cf1;
	RealType cf2;
	RealType cm;
	RealType cg;
	bool     csin;
	RealType scale;
	RealType max;
	SineGenerator 		sine0;
	SineGenerator 		sine1;
	SineGenerator 		sine2;
	GaussianGenerator 	gauss;
	ChirpGenerator		chirp;
	SignalAdder			adder;

	uint64_t			curFrame;
	uint64_t			start;

	void __pack(UnpackedSample* u, PackedSample* p);
	void __printFrame(DrxFrame* f, bool compact=false, bool single=false);
};

#endif /* DRXFRAMEGENERATOR_H_ */
