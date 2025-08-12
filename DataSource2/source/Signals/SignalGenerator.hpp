/*
 * SignalGenerator.h
 *
 *  Created on: Jan 29, 2012
 *      Author: chwolfe2
 */

#ifndef SIGNALGENERATOR_H_
#define SIGNALGENERATOR_H_

#include "Complex.h"

class SignalGenerator {

public:
	SignalGenerator(double dynamicRange=8.0):dynamicRange(dynamicRange){
		// do nothing
	}

	virtual UnpackedSample sample(double t){
		UnpackedSample c;
		c.re=0;
		c.im=0;
		return c;
	}

	virtual ~SignalGenerator(){

	}
	virtual double getDynamicRange()const{
		return dynamicRange;
	}


private:
	double dynamicRange;
};

#endif /* SIGNALGENERATOR_H_ */
