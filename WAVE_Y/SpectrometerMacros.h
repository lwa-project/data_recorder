/*
 * SpectrometerMacros.h
 *
 *  Created on: Feb 3, 2012
 *      Author: chwolfe2
 */

#ifndef SPECTROMETERMACROS_H_
#define SPECTROMETERMACROS_H_
// G_ prefix means from supplied geometry
#define G_TRANSFORM_SIZE_SAMPLES(n)       (n)
#define G_INTEGRATION_SIZE_SAMPLES(n, k)  (G_TRANSFORM_SIZE_SAMPLES(n) * k)

#define G_IDATA_SIZE_SAMPLES(n,k)         (G_INTEGRATION_SIZE_SAMPLES(n,k))
#define G_ODATA_SIZE_SAMPLES(n,k)         (G_IDATA_SIZE_SAMPLES(n,k))
#define G_ACC_SIZE_SAMPLES(n)			  (G_TRANSFORM_SIZE_SAMPLES(n))

#define G_IDATA_SIZE_BYTES(n,k)           (G_IDATA_SIZE_SAMPLES(n,k) * sizeof(UnpackedSample))
#define G_ODATA_SIZE_BYTES(n,k)           (G_IDATA_SIZE_BYTES(n,k))
#define G_ACC_SIZE_BYTES(n)				  (G_TRANSFORM_SIZE_SAMPLES(n) * sizeof(RealType))

#define G_IDATA_BLOCKSIZE_BYTES(n,k)      (G_IDATA_SIZE_BYTES(n,k) * NUM_STREAMS)
#define G_ODATA_BLOCKSIZE_BYTES(n,k)      (G_IDATA_BLOCKSIZE_BYTES(n,k))
#define G_ACC_BLOCKSIZE_BYTES(n)		  (G_ACC_SIZE_BYTES(n) * NUM_STREAMS)

#define G_IDATA_START_INDEX(t,p,n,k)      (G_IDATA_SIZE_SAMPLES(n,k) * STREAM_INDEX(t,p))
#define G_ODATA_START_INDEX(t,p,n,k)      G_IDATA_START_INDEX(t,p,n,k)
#define G_ACCUMULATOR_START_INDEX(t,p,n)  (G_ACC_SIZE_SAMPLES(n) * STREAM_INDEX(t,p))

// O_ prefix means from supplied SpectrometerOptions* pointer
#define O_TRANSFORM_SIZE_SAMPLES(o)       (o->nFreqChan)
#define O_INTEGRATION_SIZE_SAMPLES(o)     (O_TRANSFORM_SIZE_SAMPLES(o) * o->nInts)

#define O_IDATA_SIZE_SAMPLES(o)           (O_INTEGRATION_SIZE_SAMPLES(o))
#define O_ODATA_SIZE_SAMPLES(o)           (O_IDATA_SIZE_SAMPLES(o))
#define O_ACC_SIZE_SAMPLES(o)			  (O_TRANSFORM_SIZE_SAMPLES(o))

#define O_IDATA_SIZE_BYTES(o)             (O_IDATA_SIZE_SAMPLES(o) * sizeof(UnpackedSample))
#define O_ODATA_SIZE_BYTES(o)             (O_IDATA_SIZE_BYTES(o))
#define O_ACC_SIZE_BYTES(o)				  (O_TRANSFORM_SIZE_SAMPLES(o) * sizeof(RealType))

#define O_IDATA_BLOCKSIZE_BYTES(o)        (O_IDATA_SIZE_BYTES(o) * NUM_STREAMS)
#define O_ODATA_BLOCKSIZE_BYTES(o)        (O_IDATA_BLOCKSIZE_BYTES(o))
#define O_ACC_BLOCKSIZE_BYTES(o)		  (O_ACC_SIZE_BYTES(o) * NUM_STREAMS)

#define O_IDATA_START_INDEX(t,p,o)        (O_IDATA_SIZE_SAMPLES(o) * STREAM_INDEX(t,p))
#define O_ODATA_START_INDEX(t,p,o)        O_IDATA_START_INDEX(t,p,o)
#define O_ACCUMULATOR_START_INDEX(t,p,o)  (O_ACC_SIZE_SAMPLES(o) * STREAM_INDEX(t,p))


// S_ prefix means from supplied Spectrometer*
#define O_OF_S (&((s)->options))
#define S_TRANSFORM_SIZE_SAMPLES(s)       O_TRANSFORM_SIZE_SAMPLES(O_OF_S)
#define S_INTEGRATION_SIZE_SAMPLES(s)     O_INTEGRATION_SIZE_SAMPLES(O_OF_S)

#define S_IDATA_SIZE_SAMPLES(s)           O_IDATA_SIZE_SAMPLES(O_OF_S)
#define S_ODATA_SIZE_SAMPLES(s)           O_ODATA_SIZE_SAMPLES(O_OF_S)
#define S_ACC_SIZE_SAMPLES(s)			  O_ACC_SIZE_SAMPLES(O_OF_S)

#define S_IDATA_SIZE_BYTES(s)             O_IDATA_SIZE_BYTES(O_OF_S)
#define S_ODATA_SIZE_BYTES(s)             O_ODATA_SIZE_BYTES(O_OF_S)
#define S_ACC_SIZE_BYTES(s)				  O_ACC_SIZE_BYTES(O_OF_S)

#define S_IDATA_BLOCKSIZE_BYTES(s)        O_IDATA_BLOCKSIZE_BYTES(O_OF_S)
#define S_ODATA_BLOCKSIZE_BYTES(s)        O_ODATA_BLOCKSIZE_BYTES(O_OF_S)
#define S_ACC_BLOCKSIZE_BYTES(s)		  O_ACC_BLOCKSIZE_BYTES(O_OF_S)

#define S_IDATA_START_INDEX(t,p,s)        O_IDATA_START_INDEX(t,p,O_OF_S)
#define S_ODATA_START_INDEX(t,p,s)        O_ODATA_START_INDEX(t,p,O_OF_S)
#define S_ACCUMULATOR_START_INDEX(t,p,s)  O_ACCUMULATOR_START_INDEX(t,p,O_OF_S)

// some indexing macros
#define T_IDX(t) ((t)-1)
#define P_IDX(p) (p)
#define TP_IDX(t,p) (((T_IDX(t)) << 1) | (P_IDX(p)))
#define TP_MASK(t,p) (1 << TP_IDX((t),(p)))
#define STREAM_INDEX(t,p)		TP_IDX(t,p)
#define F_STREAM_INDEX(f)		TP_IDX((f)->header.drx_tuning, (f)->header.drx_polarization)

#define STEP_PHASE(tag,dec,spf) ( ((uint64_t)tag) % (((uint64_t) dec) * ((uint64_t) spf)) )
#define STEP_PHASE_DRX(tag,dec) ( STEP_PHASE(tag,dec,DRX_SAMPLES_PER_FRAME) )
#define SAMPLES_PER_BLOCK(opts) (opts->nFreqChan * opts->nInts)
#define FRAMES_PER_BLOCK(opts) ((SAMPLES_PER_BLOCK(opts)) / DRX_SAMPLES_PER_FRAME)

#define IDATA_OFFSET(t,t0,d)    ((t-t0)/d)
#define B_IDATA_OFFSET(t,b)     (IDATA_OFFSET(t,(b)->setup.timeTag0,(b)->setup.decFactor))
#define BF_IDATA_OFFSET(f,b)    B_IDATA_OFFSET(f->header.timeTag, b)
#define SBF_IDATA_OFFSET(s,b,f) BF_IDATA_OFFSET(f, (&s->blocks[b]) )

#define __cellDone(c) (c.state != CS_RUNNING)
#define __cellError(c) (c.state == CS_ERROR)
#define __blockDone(b) ( __cellDone(b.cells[0]) && __cellDone(b.cells[1]) && __cellDone(b.cells[2]) && __cellDone(b.cells[3]))
#define __blockAnyError(b) ( __cellError(b.cells[0]) || __cellError(b.cells[1]) || __cellError(b.cells[2]) || __cellError(b.cells[3]))
#define __blockAllError(b) ( __cellError(b.cells[0]) && __cellError(b.cells[1]) && __cellError(b.cells[2]) && __cellError(b.cells[3]))


#endif /* SPECTROMETERMACROS_H_ */

