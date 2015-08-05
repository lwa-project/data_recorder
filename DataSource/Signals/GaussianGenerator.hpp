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
	GaussianGenerator(bool useComplex, double scale=1.0): SignalGenerator(scale), useComplex(useComplex), scale(scale), n() {
		// do nothing
		cout << "Created: Gaussian generator: Scale " << scale << "." <<endl;
	}

	UnpackedSample sample(double t){
		UnpackedSample c = n.next();
		c.re = (c.re * scale);
		c.im = useComplex ? (c.im * scale) : 0.0f;
		return c;
	}
	virtual ~GaussianGenerator(){
		// do nothing
	}




private:
	bool useComplex;
	double scale;
	Noise	 n;
};

#endif /* GAUSSIANGENERATOR_H_ */
