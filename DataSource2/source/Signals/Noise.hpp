#ifndef __NOISE_H
#define __NOISE_H

#include <stdlib.h>
#include <time.h>
#include "Complex.h"
#include <math.h>

class Noise{
public:
	Noise(){
		srand ( time(NULL) );
	}
	virtual ~Noise(){

		//do nothing
	}
	UnpackedSample next(){
		// Box-Muller polar form from http://www.taygeta.com/random/gaussian.html
		// we use the default random number generator b/c it's "good enough" for testing
		UnpackedSample rv;
		double x1, x2, w;
 		do {
			x1 = 2.0 * (((double)rand()) / ((double)RAND_MAX)) - 1.0;
			x2 = 2.0 * (((double)rand()) / ((double)RAND_MAX)) - 1.0;
			w = x1 * x1 + x2 * x2;
		} while ( w >= 1.0 );

		w = sqrt( (-2.0 * log( w ) ) / w );
		rv.re = x1 * w;
		rv.im = x2 * w;
		return rv;
	}
private:

};
#endif // __NOISE_H
