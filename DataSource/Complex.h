/*
 * Complex.h
 *
 *  Created on: Jan 29, 2012
 *      Author: chwolfe2
 */

#ifndef COMPLEX_H_
#define COMPLEX_H_

#ifdef __cplusplus
extern "C"{
#endif
#include <stdint.h>
#include <fftw3.h>

// define the type of datums (float)
typedef fftwf_complex 	 ComplexType;
typedef float			 RealType;
typedef union __PackedSample{
	struct {
		int8_t q;
		int8_t i;
	};
	struct {
		int8_t im;
		int8_t re;
	};
	uint16_t packed;
}__attribute__((packed)) PackedSample;

// define union type for an unpacked sample
typedef union __UnpackedSample{
	struct {
		RealType i;
		RealType q;
	};
	struct {
		RealType re;
		RealType im;
	};
	ComplexType packed;
}__attribute__((packed)) UnpackedSample;

#ifdef __cplusplus
}
#endif

#endif /* COMPLEX_H_ */
