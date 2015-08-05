/*
 * SignalGenerator.cpp
 *
 *  Created on: Jan 29, 2012
 *      Author: chwolfe2
 */

#include "SignalGenerator.hpp"
SignalGenerator::SignalGenerator(){
	// do nothing
}

UnpackedSample SignalGenerator::next(double dt){
	UnpackedSample c;
	c.re=0;
	c.im=0;
	return c;
}

SignalGenerator::~SignalGenerator(){
	// do nothing
}
