/*
 * SineGenerator.h
 *
 *  Created on: Jan 29, 2012
 *      Author: chwolfe2
 */

#ifndef SINEGENERATOR_H_
#define SINEGENERATOR_H_
#include "SignalGenerator.hpp"
#include <math.h>

class SineGenerator: public SignalGenerator {
public:
	SineGenerator(bool useComplex, RealType mag, RealType f0);
	UnpackedSample next(double dt);
	virtual ~SineGenerator();
private:
	SineGenerator();
	bool useComplex;
	double mag;
	double f0;
	double t;
};

#endif /* SINEGENERATOR_H_ */
