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
	SignalGenerator();
	virtual UnpackedSample next(double dt);
	virtual ~SignalGenerator();
};

#endif /* SIGNALGENERATOR_H_ */
