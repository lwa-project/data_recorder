/*
 * ChirpGenerator.h
 *
 *  Created on: Feb 6, 2012
 *      Author: chwolfe2
 */

#ifndef CHIRPGENERATOR_H_
#define CHIRPGENERATOR_H_

#include "SignalGenerator.hpp"

class ChirpGenerator: public SignalGenerator {
public:
	ChirpGenerator(bool useComplex, RealType mag, RealType f0, RealType f1, RealType f2, RealType gamma, bool sinusoidal);
	UnpackedSample next(double dt);
	virtual ~ChirpGenerator();
private:
	ChirpGenerator();
	bool useComplex;
	double mag;
	double f0;
	double f1;
	double f2;
	double gamma;
	bool sinusoidal;
	double t;

};

#endif /* CHIRPGENERATOR_H_ */
