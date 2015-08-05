/*
 * DataTypes.h
 *
 *  Created on: May 29, 2012
 *      Author: chwolfe2
 */

#ifndef DATATYPES_H_
#define DATATYPES_H_
#include <fftw3.h>
#include <stdint.h>

// define the type of datums (float)
typedef fftwf_complex 	 ComplexType;
typedef float			 RealType;

// define union type for a packed sample
typedef union __PackedSample{
	struct {
		int8_t q:4;
		int8_t i:4;
	};
	struct {
		int8_t im:4;
		int8_t re:4;
	};
	uint8_t packed;
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


#endif /* DATATYPES_H_ */
