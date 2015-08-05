/*
 * SineGenerator.cpp
 *
 *  Created on: Jan 29, 2012
 *      Author: chwolfe2
 */

#include "SineGenerator.hpp"
#include <iostream>
using namespace std;
SineGenerator::SineGenerator(bool useComplex,RealType mag, RealType f0): useComplex(useComplex), mag((double)mag), f0((double)f0),t(0.0f) {
	// do nothing
	cout << "Created: Sine generator: Frequency "<<f0<<" Hz, Amplitude "<<mag<<"."<<endl;
}

UnpackedSample SineGenerator::next(double dt){
	t=t+dt;
	double i = mag * sin(f0 * t * M_PIl * 2.0L);
	double r = mag * cos(f0 * t * M_PIl * 2.0L);
	UnpackedSample c;
	c.re = (RealType)r;
	c.im = useComplex ? (RealType)i : 0.0f;
	return c;
}

SineGenerator::~SineGenerator(){
	// do nothing
}
