#ifndef LWA_H_
#define LWA_H_
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>

#define DP_BASE_FREQ_HZ        196e6
#define FREQ_CODE_FACTOR       (DP_BASE_FREQ_HZ / (double)0x100000000)
#define FREQ_CODE(f)           ((f*1e6*(double)0x100000000)/(DP_BASE_FREQ_HZ))
#define FREQ_FROM_FREQ_CODE(f) (((double)f) * FREQ_CODE_FACTOR)
#define SAMPLES_PER_SECOND 	196000000l

enum Mode {DRX, TBN, TBW, RAW};


#ifndef TBNFRAME_H_
	#include "TbnFrame.h"
#endif

#ifndef TBwFRAME_H_
	#include "TbwFrame.h"
#endif

#ifndef DRXFRAME_H_
	#include "DrxFrame.h"
#endif



#endif /* LWA_H_ */
