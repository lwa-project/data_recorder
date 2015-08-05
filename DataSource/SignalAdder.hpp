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
//using namespace std;

class SignalAdder: public SignalGenerator {
public:
	SignalAdder(std::list<SignalGenerator*> signals): signals(signals){
		// do nothing
	};
	SignalAdder():signals(){
		// do nothing
	};
	UnpackedSample next(double dt){
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
				temp=(*iter)->next(dt);
				c.re+=temp.re;
				c.im+=temp.im;
			}
		}
		return c;
	}
	void add(SignalGenerator* sig){
		signals.push_back(sig);
	}
private:
	std::list<SignalGenerator*> signals;
};

#endif /* SIGNALADDER_H_ */
