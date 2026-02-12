/*
 * TestPatternGenerator.h
 *
 *  Created on: Oct 22, 2012
 *      Author: chwolfe2
 */

#ifndef TESTPATTERNGENERATOR_H_
#define TESTPATTERNGENERATOR_H_

#include "SignalGenerator.hpp"
#include "SignalAdder.hpp"
#include "Noise.hpp"
#include "SineGenerator.hpp"
#include "GaussianGenerator.hpp"
#include "ChirpGenerator.h"
using namespace std;

class TestPatternGenerator: public SignalGenerator {
public:
	TestPatternGenerator(
			bool	 useComplex,
			double f0, double m0,
			double f1, double m1,
			double f2, double m2,
			double cf0,
			double cf1,
			double cf2,
			double cm,
			double cg,
			bool     csin,
			double scale,
			double max):
				SignalGenerator(),
				useComplex(useComplex),
				f0(f0),					m0(m0),
				f1(f1),					m1(m1),
				f2(f2),					m2(m2),
				cf0(cf0),		cf1(cf1),		cf2(cf2),		cm(cm),		cg(cg),		csin(csin),
				scale(scale),			max(max),
				sine0(useComplex,m0,f0),
				sine1(useComplex,m1,f1),
				sine2(useComplex,m2,f2),
				gauss(useComplex,scale),
				chirp(useComplex,cm,cf0,cf1,cf2,cg,csin),
				adder() {
			adder.add(&sine0);
			adder.add(&sine1);
			adder.add(&sine2);
			adder.add(&gauss);
			adder.add(&chirp);

		}
	UnpackedSample sample(double t){
		return adder.sample(t);
	}
	virtual ~TestPatternGenerator(){}
private:
	bool useComplex;
	double f0; double m0;
	double f1; double m1;
	double f2; double m2;
	double cf0;
	double cf1;
	double cf2;
	double cm;
	double cg;
	bool     csin;
	double scale;
	double max;
	SineGenerator 		sine0;
	SineGenerator 		sine1;
	SineGenerator 		sine2;
	GaussianGenerator 	gauss;
	ChirpGenerator		chirp;
	SignalAdder			adder;

};

#endif /* TESTPATTERNGENERATOR_H_ */
