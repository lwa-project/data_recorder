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
	SineGenerator(bool useComplex, double mag, double f0): SignalGenerator(mag), useComplex(useComplex), mag((double)mag), f0((double)f0) {
		// do nothing
		cout << "Created: Sine generator: Frequency "<<f0<<" Hz, Amplitude "<<mag<<"."<<endl;
	}

	UnpackedSample sample(double t){
		double i = mag * sin(f0 * t * M_PIl * 2.0L);
		double r = mag * cos(f0 * t * M_PIl * 2.0L);
		UnpackedSample c;
		c.re = (RealType)r;
		c.im = useComplex ? (RealType)i : 0.0L;
		return c;
	}

	virtual ~SineGenerator(){
		// do nothing
	}
private:
	SineGenerator();
	bool useComplex;
	double mag;
	double f0;
};

#endif /* SINEGENERATOR_H_ */
