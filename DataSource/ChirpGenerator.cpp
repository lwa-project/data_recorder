/*
 * ChirpGenerator.cpp
 *
 *  Created on: Feb 6, 2012
 *      Author: chwolfe2
 */

#include <cmath>
#include <math.h>
#include <cstdlib>
#include <iostream>
using namespace std;

#include "ChirpGenerator.h"

ChirpGenerator::ChirpGenerator(bool useComplex, RealType mag, RealType f0, RealType f1, RealType f2, RealType gamma, bool sinusoidal):
		useComplex(useComplex), mag((double)mag), f0((double)f0), f1((double)f1), f2((double)f2), gamma((double)gamma), sinusoidal(sinusoidal), t(0.0f){
	cout << "Created: Chirp generator: "       <<
			"\n\tFrequency_0 " << f0 <<" Hz, " <<
			"\n\tFrequency_1 " << f1 <<" Hz, " <<
			"\n\tFrequency_2 " << f2 <<" Hz, " <<
			"\n\tAmplitude " << mag;
			if (sinusoidal){
				cout << "\n\tType : sinusoidal\n";
			} else {
				cout << "\n\tType : sawtooth (Gamma=" << gamma << ")\n";
			}

}
UnpackedSample ChirpGenerator::next(double dt){
	t=t+dt;
	UnpackedSample c;
	double i;
	double r;

	if (sinusoidal){
		double frange  = abs(f0-f1)/2;
		double fcenter = (f0+f1)/2;
		double ft      = fcenter + (frange * cos(f2 * t * M_PIl * 2.0L));
		if (!((f0<=ft)&(ft<=f1)) && !((f0<=ft)&(ft<=f1)))
			cout << "Error ft range: " << f0 << " " <<ft << " " <<f1<<endl;
		r =  mag * cos(ft * t * M_PIl * 2.0L);
		i =  mag * sin(ft * t * M_PIl * 2.0L);
	} else {
		double T = (1.0L/f2);
		double phase  = abs(fmod(t,T)/T);
		double g_adj  = pow(phase,gamma);
		double ft     = f0 + (g_adj * (f1-f0));
		if (!((f0<=ft)&(ft<=f1)) && !((f0<=ft)&(ft<=f1)))
			cout << "Error ft range: " << f0 << " " <<ft << " " <<f1<<endl;
		r =  mag * cos(ft * t * M_PIl * 2.0L);
		i =  mag * sin(ft * t * M_PIl * 2.0L);
	}
	c.re = (RealType)r;
	c.im = useComplex ? (RealType)i : 0.0f;
	return c;
}

ChirpGenerator::~ChirpGenerator(){
	// do nothing
}
