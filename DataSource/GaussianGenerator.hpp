/*
 * SineGenerator.h
 *
 *  Created on: Jan 29, 2012
 *      Author: chwolfe2
 */

#ifndef GAUSSIANGENERATOR_H_
#define GAUSSIANGENERATOR_H_
#include "SignalGenerator.hpp"
#include "Noise.hpp"
#include <math.h>

class GaussianGenerator: public SignalGenerator {
public:
	GaussianGenerator(bool useComplex,RealType scale);
	GaussianGenerator(bool useComplex);
	UnpackedSample next(double dt);
	virtual ~GaussianGenerator();
private:
	bool useComplex;
	RealType scale;
	Noise	 n;
};

#endif /* GAUSSIANGENERATOR_H_ */
