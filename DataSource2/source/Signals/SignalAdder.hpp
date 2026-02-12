/*
 * SignalAdder.h
 *
 *  Created on: Jan 29, 2012
 *      Author: chwolfe2
 */

#ifndef SIGNALADDER_H_
#define SIGNALADDER_H_

#include "SignalGenerator.hpp"
#include <list>
#include <iostream>
using namespace std;

class SignalAdder: public SignalGenerator {
public:
	SignalAdder(std::list<SignalGenerator*> signals): SignalGenerator(),signals(signals){
		// do nothing
	};
	SignalAdder():SignalGenerator(),signals(){
		// do nothing
	};
	UnpackedSample sample(double t){
		UnpackedSample c,temp;
		c.re=0;
		c.im=0;
		temp.re=0;
		temp.im=0;
		for (
				std::list<SignalGenerator*>::iterator iter = signals.begin();
				iter!=signals.end();
				iter++
				){
			if (*iter){
				temp=(*iter)->sample(t);
				c.re+=temp.re;
				c.im+=temp.im;
			}
		}
		return c;
	}
	void add(SignalGenerator* sig){
		signals.push_back(sig);
	}
	virtual double getDynamicRange()const{
		std::cout << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%55\n\n\n";
		double dynamicRange=0;
		for (
				std::list<SignalGenerator*>::const_iterator iter = signals.begin();
				iter!=signals.end();
				iter++
				){
			if (*iter){
				if ((*iter)->getDynamicRange() > dynamicRange){
					dynamicRange = (*iter)->getDynamicRange();
				}
			}
		}
		return dynamicRange;
	}

private:
	std::list<SignalGenerator*> signals;
};

#endif /* SIGNALADDER_H_ */
