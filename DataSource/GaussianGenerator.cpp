/*
 * GaussianGenerator.cpp
 *
 *  Created on: Jan 29, 2012
 *      Author: chwolfe2
 */

#include "GaussianGenerator.hpp"
#include <iostream>
using namespace std;

GaussianGenerator::GaussianGenerator(bool useComplex, RealType scale): useComplex(useComplex), scale(scale), n() {
	// do nothing
	cout << "Created: Gaussian generator: Scale "<<scale<<"."<<endl;
}
GaussianGenerator::GaussianGenerator(bool useComplex): useComplex(useComplex), scale(1), n() {
	// do nothing
}
UnpackedSample GaussianGenerator::next(double dt){
	UnpackedSample c = n.next();
	c.re = (c.re * scale);
	c.im = useComplex ? (c.im * scale) : 0.0f;
	return c;
}
GaussianGenerator::~GaussianGenerator(){
	// do nothing
}
