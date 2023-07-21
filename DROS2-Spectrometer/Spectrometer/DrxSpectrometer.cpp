// ========================= DROSv2 License preamble===========================
// Software for DROSv2 is covered jointly under GPLv3 and Boost 1.0 licenses,
// to the extent required by included Boost sources and GPL sources, and to the
// more restrictive case pertaining thereunto, as defined herebelow. Beyond those
// requirements, any code not explicitly restricted by either of thowse two license
// models shall be deemed to be licensed under the GPLv3 license and subject to
// those restrictions.
//
// Copyright 2012, Virginia Tech, University of New Mexico, and Christopher Wolfe
//
// ========================= Boost License ====================================
// Boost Software License - Version 1.0 - August 17th, 2003
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
//
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
// =============================== GPL V3 ====================================
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "DrxSpectrometer.h"
#include "../System/LoggableAssert.h"

unsigned char DrxSpectrometer::SatLUT[256];
bool          DrxSpectrometer::LUT_initialized=false;

void DrxSpectrometer::initLookUpTables(){
	if (LUT_initialized) return;
	for (int byte=0; byte<256; byte++){
		float magSquared = (float) byte * byte;
		SatLUT[byte] = ( magSquared >= 16129 );
	}
	LUT_initialized=true;
}

TB_Geometry DrxSpectrometer::getDrxBufferSize(unsigned K, ProductType OutT){
	TB_Geometry rv;
	rv.size                 = 32ll * 1048576ll; /* 32 MiB */
	size_t ideal_block_size = 256ll * 1024ll; /*256 KiB */
	rv.framesize            = (OutT.numberOutputProducts() * K * DRX_TUNINGS * sizeof(float)) + sizeof(DrxSpectraHeader);
	rv.framesPerTicket      = ideal_block_size/ rv.framesize;
	size_t tsize            = rv.framesize * rv.framesPerTicket;
	rv.size                 = (((rv.size / tsize)+1)*tsize)-tsize;
	return rv;
}

DrxSpectrometer::DrxSpectrometer(
		unsigned K,
		unsigned L,
		unsigned Nb,
		ProductType _OutT,
		TicketBuffer*& _source
		):
			Plugin("DrxSpectrometer",_source, getDrxBufferSize(K,_OutT)),
			valid(false),
			doneReceiving(false),
			doneProducing(false),
			master_stopped(false),
			slave_stopped(false),
			blocks(NULL),
			blocks_free(Nb+1,1),
			blocks_startable(Nb+1,1),
			blocks_dropped((2*Nb)+1,1),  // double the size so that we can always drop blocks
			blocks_computing(MAX_SIMULTANEOUS_BLOCKS_PROCESSING+1,1),

			blocks_filling(),
			lastFilledBlockSetup(),
			lastFilledBlockSetupGood(false),
			spc(NULL),
			source(_source),
			freqCount_or_samp_per_frame(K),
			intCount(L),
			blockCount(Nb),
			OutT(_OutT),
			minTimeTag(0),
			rptTimer(DRX_SPECTROMETER_REPORT_INTERVAL),
			outputHeaderSize(sizeof(DrxSpectraHeader)),
			outputDataSize(OutT.numberOutputProducts() * K * DRX_TUNINGS * sizeof(float)),
			outputBlockSize(outputHeaderSize+outputDataSize),

			inputFramesPerCell((K * L) / DRX_SAMPLES_PER_FRAME),
			inputFramesPerBlock(inputFramesPerCell * 4)


			{
	initLookUpTables();

	if (!blocks_free.isValid()){
		LOGC(L_ERROR, "["+getObjName()+"]: can't initialize blocks_free", ACTOR_ERROR_COLORS);
		return;
	}
	if (!blocks_startable.isValid()){
		LOGC(L_ERROR, "["+getObjName()+"]: can't initialize blocks_startable ", ACTOR_ERROR_COLORS);
		return;
	}
	if (!blocks_dropped.isValid()){
		LOGC(L_ERROR, "["+getObjName()+"]: can't initialize blocks_dropped ", ACTOR_ERROR_COLORS);
		return;
	}
	if (!blocks_computing.isValid()){
		LOGC(L_ERROR, "["+getObjName()+"]: can't initialize blocks_computing ", ACTOR_ERROR_COLORS);
		return;
	}

	blocks = (DrxBlockSetup*) malloc(Nb * sizeof(DrxBlockSetup));
	if (!blocks){
		LOGC(L_ERROR, "["+getObjName()+"]: can't initialize blocks buffer", ACTOR_ERROR_COLORS);
	}

	spc = new (nothrow) Spectrometer(K,L,2,Nb,XandY,Noninterleaved,OutT);
	if (!spc){
		free(blocks); blocks = NULL;
		LOGC(L_ERROR, "["+getObjName()+"]: can't allocate spectrometer", ACTOR_ERROR_COLORS);
		return;
	}

	for (unsigned b=0; b<Nb; b++){
		DrxBlockSetup* block = &blocks[b];
		bzero((void*)block,sizeof(DrxBlockSetup));
		block->bIdx = b;
		spc->resetBlock(b);
		DrxBlockSetup** freeptr = blocks_free.nextIn();
		if (!freeptr){
			LOGC(L_ERROR, "["+getObjName()+"]: can't fill blocks_free", ACTOR_ERROR_COLORS);
			delete(spc); spc=NULL; free(blocks); blocks = NULL;
			return;
		}
		*freeptr = block;
		blocks_free.doneIn(freeptr);
	}

	if (!spc->isValid()){
		LOGC(L_ERROR, "["+getObjName()+"]: can't initialize spectrometer : " + spc->getReason(), ACTOR_ERROR_COLORS);
		delete(spc); spc=NULL; free(blocks); blocks = NULL;
		return;
	}

	bzero((void*) &counters, sizeof(SpectrometerCounters));
	valid = true;
}


DrxSpectrometer::~DrxSpectrometer(){
	if (spc){ delete(spc); spc=NULL; }
	if (blocks){ free(blocks); blocks=NULL; }
}



void DrxSpectrometer::stopProducing(){
	doneProducing = true;
}
void DrxSpectrometer::stopReceiving(){
	doneReceiving = true;
}
bool DrxSpectrometer::isShutDown(){
	return !valid || (master_stopped && slave_stopped);
}

bool DrxSpectrometer::isValid(){
	return valid;
}
bool DrxSpectrometer::frameIsLegal(DrxFrame* f){
	return
			(f->header.decFactor != 0) &&
			(f->header.timeTag != 0) &&
			(
					(f->header.drx_tuning == 1) ||
					(f->header.drx_tuning == 2)
			);
}







void DrxSpectrometer::run_master(){
	LOGC(L_INFO, "["+getObjName()+"] Master thread started", ACTOR_COLORS);
	DrxFrame* curFrame = NULL;
	while (!isInterrupted() && (spc != NULL) && (blocks != NULL)){
		if (curFrame == NULL && !doneReceiving){
			curFrame = (DrxFrame*) Plugin::getNextIn(DRX_FRAME_SIZE);
			if (curFrame != NULL)
				counters.framesReceived++;
		}
		if (curFrame != NULL){
			curFrame->fixByteOrder();
			if (frameIsLegal(curFrame)){
				if (insert(curFrame)){
					Plugin::doneIn(doneReceiving);
					curFrame=NULL;
				} else {
					curFrame->unfixByteOrder();
				}
			} else {
				counters.illegalFrames++;
				Plugin::doneIn(doneReceiving);
				curFrame=NULL;
			}
		}
		if (doneReceiving){
			if (curFrame != NULL){
				Plugin::doneIn(true);
				curFrame=NULL;
			}
			break;
		}

		if (rptTimer.isTimeToReport()){
			LOGC(L_INFO, specReport(), ACTOR_COLORS);
		}

	}
	master_stopped = true;
	LOGC(L_INFO, "["+getObjName()+"] Master thread finished", ACTOR_COLORS);
}


void DrxSpectrometer::run_slave(){
	LOGC(L_INFO, "["+getObjName()+"] Slave thread started", ACTOR_COLORS);

	while (
			((spc != NULL) && (blocks != NULL)) &&        // mandatory data structures
			(
					(blocks_computing.used() !=0) ||      // don't stop while computing in progress
					(!isInterrupted() && !doneProducing)  // if possible, stop when interrupted or no longer producing
			)
		){

		// check if we can start any blocks (if we're still producing)
		if (canMove(&blocks_startable, &blocks_computing) && !doneProducing){
			void * outFramePtr = Plugin::getNextOut(outputBlockSize);
			if (outFramePtr != NULL){
				DrxBlockSetup*    bs    = *blocks_startable.nextOut();
				DrxSpectraHeader* dsh   = (DrxSpectraHeader*) outFramePtr;
				float*            aData = (float*) &(((char*)outFramePtr)[sizeof(DrxSpectraHeader)]);
				startBlock(bs, dsh, aData);
				doMove(&blocks_startable, &blocks_computing);
				//LOGC(L_ERROR, "STARTED A BLOCK", FATAL_COLORS);

			} else {
				counters.checkBlockStart_WaitOutbuf++;
			}
		}

		// check if we can have done/error blocks (previously started)
		if (canMove(&blocks_computing, &blocks_free)){
			DrxBlockSetup*    bs    = *blocks_computing.nextOut();
			LOG_ASSERT(bs != NULL);
			LOG_ASSERT(bs->state == BS_PROCESSING);
			if (isDoneOrError(bs)){
				if (isError(bs)){
					counters.checkBlockDone_Error++;
					for (unsigned i=0; i<4; i++){
						bs->header->errors[i] = (uint8_t) true;
					}
					bzero(bs->data, outputDataSize);
				} else {
					counters.checkBlockDone_Done++;
				}
				spc->waitBlock(bs->bIdx);
				spc->resetBlock(bs->bIdx);
				bs->state = BS_DONE;
				resetBlockSetup(bs);
				doMove(&blocks_computing, &blocks_free);
				Plugin::doneOut(doneProducing);
				counters.blocksCompleted++;
			} else {
				counters.checkBlockDone_NotDone++;
			}
		} else {
			if (canRemove(&blocks_computing))
				counters.checkBlockDone_WaitFree++;
			else
				counters.checkBlockDone_NotComputing++;
		}

		// check if we can have error blocks (not previously started)
		if (canMove(&blocks_dropped, &blocks_free)){
			DrxBlockSetup* bs = *blocks_dropped.nextOut();
			LOG_ASSERT(bs != NULL);
			LOG_ASSERT(bs->state == BS_DROPPED);
			spc->resetBlock(bs->bIdx);
			resetBlockSetup(bs);
			doMove(&blocks_dropped, &blocks_free);
			counters.blocksDropped++;
		}
	}
	slave_stopped = true;
	LOGC(L_INFO, "["+getObjName()+"] Slave thread stopped", ACTOR_COLORS);
}

void DrxSpectrometer::resetBlockSetup(DrxBlockSetup* bs){
	LOG_ASSERT(spc!=NULL);
	LOG_ASSERT(blocks!=NULL);
	LOG_ASSERT(bs!=NULL);
	LOG_ASSERT((bs->state == BS_DONE) || (bs->state == BS_DROPPED));
	size_t bIdx = bs->bIdx;
	bzero(bs, sizeof(DrxBlockSetup));
	bs->bIdx = bIdx;
}

void DrxSpectrometer::startBlock(DrxBlockSetup* bs, DrxSpectraHeader* dsh, float* aData){
	LOG_ASSERT(spc!=NULL);
	LOG_ASSERT(blocks!=NULL);
	LOG_ASSERT(bs!=NULL);
	LOG_ASSERT(dsh!=NULL);
	LOG_ASSERT(aData!=NULL);
	LOG_ASSERT(bs->state == BS_STARTABLE);
	initSpectraHeader(bs,dsh);
	spc->startBlock(bs->bIdx, aData);
	bs->state = BS_PROCESSING;
	counters.blocksStarted++;
}

bool DrxSpectrometer::isDoneOrError(DrxBlockSetup* bs){
	LOG_ASSERT(spc!=NULL);
	LOG_ASSERT(blocks!=NULL);
	LOG_ASSERT(bs!=NULL);
	LOG_ASSERT(bs->state == BS_PROCESSING);
	return (spc->isBlockDone(bs->bIdx) || spc->isBlockError(bs->bIdx));
}

bool DrxSpectrometer::isError(DrxBlockSetup* bs){
	LOG_ASSERT(spc!=NULL);
	LOG_ASSERT(blocks!=NULL);
	LOG_ASSERT(bs!=NULL);
	return (spc->isBlockError(bs->bIdx));
}

void DrxSpectrometer::initSpectraHeader(DrxBlockSetup* bs, DrxSpectraHeader* dsh){
	LOG_ASSERT(spc!=NULL);
	LOG_ASSERT(blocks!=NULL);
	LOG_ASSERT(bs!=NULL);
	LOG_ASSERT(dsh!=NULL);
	size_t cell0index = spc->cellIndex(bs->bIdx, 0, 0);
	dsh->MAGIC1        = DRX_SPECTRA_MAGIC1;
	dsh->MAGIC2        = DRX_SPECTRA_MAGIC2;
	dsh->beam          = bs->beam;
	dsh->decFactor     = bs->decFactor;
	dsh->errors[0]     = 0;
	dsh->errors[1]     = 0;
	dsh->errors[2]     = 0;
	dsh->errors[3]     = 0;
	dsh->fills[0]      = *spc->getCellFills(cell0index+0);
	dsh->fills[1]      = *spc->getCellFills(cell0index+1);
	dsh->fills[2]      = *spc->getCellFills(cell0index+2);
	dsh->fills[3]      = *spc->getCellFills(cell0index+3);
	dsh->freqCode[0]   = bs->freqCode[0];
	dsh->freqCode[1]   = bs->freqCode[1];
	dsh->nInts         = intCount;
	dsh->reserved      = 0;
	dsh->satCount[0]   = *spc->getCellSatCounts(cell0index+0);
	dsh->satCount[1]   = *spc->getCellSatCounts(cell0index+1);
	dsh->satCount[2]   = *spc->getCellSatCounts(cell0index+2);
	dsh->satCount[3]   = *spc->getCellSatCounts(cell0index+3);
	dsh->spec_version  = 0x02;
	dsh->flag_xcp      = 0;
	dsh->stokes_format = (uint8_t) OutT.toStokes();
	dsh->nFreqs        = freqCount_or_samp_per_frame;
	dsh->timeOffset    = bs->timeOffset;
	dsh->timeTag0      = bs->timeTag0;
}

uint64_t DrxSpectrometer::nextTimeTagAfterBlock(DrxBlockSetup* bs){
	LOG_ASSERT(spc!=NULL);
	LOG_ASSERT(blocks!=NULL);
	LOG_ASSERT(bs!=NULL);
	return bs->timeTagN + ((bs->decFactor * freqCount_or_samp_per_frame * intCount) / DRX_SAMPLES_PER_FRAME);
}

void DrxSpectrometer::initBlockSetup(DrxBlockSetup* toPrepare, DrxFrame* f, DrxBlockSetup* predecessor){
	LOG_ASSERT(toPrepare->state == BS_UNUSED);
	if (predecessor != NULL){
		counters.framesInsertedStale++;
		toPrepare->beam      		= predecessor->beam;
		toPrepare->decFactor 		= predecessor->decFactor;
		toPrepare->timeOffset   	= predecessor->timeOffset;
		toPrepare->timeTagStep  	= predecessor->timeTagStep;

		toPrepare->timeTag0			=
				((((((((f->header.timeTag - predecessor->timeTag0)/(predecessor->timeTagStep))/inputFramesPerCell)+1)*inputFramesPerCell)-inputFramesPerCell)*predecessor->timeTagStep)+predecessor->timeTag0);
		toPrepare->timeTagN			= toPrepare->timeTag0 + (predecessor->timeTagStep * (inputFramesPerCell-1));
		toPrepare->freqCode[0]  	= predecessor->freqCode[0];
		toPrepare->freqCode[1]  	= predecessor->freqCode[1];
		toPrepare->stepPhase    	= predecessor->stepPhase;
	} else {
		counters.framesInsertedFresh++;
		toPrepare->beam      		= f->header.drx_beam;
		toPrepare->decFactor 		= f->header.decFactor;
		toPrepare->timeOffset   	= f->header.timeOffset;
		toPrepare->timeTagStep  	= f->header.decFactor*DRX_SAMPLES_PER_FRAME;
		toPrepare->timeTag0			= f->header.timeTag;
		toPrepare->timeTagN			= f->header.timeTag + (toPrepare->timeTagStep * (((freqCount_or_samp_per_frame * intCount) / DRX_SAMPLES_PER_FRAME)-1));
		toPrepare->freqCode[0]  	= (f->header.drx_tuning == DRX_TUN_0) ? f->header.freqCode : FREQ_CODE_UNINITIALIZED;
		toPrepare->freqCode[1]  	= (f->header.drx_tuning != DRX_TUN_0) ? f->header.freqCode : FREQ_CODE_UNINITIALIZED;
		toPrepare->stepPhase    	= (((uint64_t)f->header.timeTag) % (((uint64_t) f->header.decFactor) * ((uint64_t) DRX_SAMPLES_PER_FRAME)));
	}
	toPrepare->state            = BS_FILLING;
	toPrepare->insertionCount   = 0;
	toPrepare->header           = NULL;
	toPrepare->data             = NULL;
}

bool DrxSpectrometer::blockMatch(DrxFrame* f, DrxBlockSetup* bs){
	LOG_ASSERT(spc!=NULL);
	LOG_ASSERT(blocks!=NULL);
	LOG_ASSERT(bs!=NULL);
	LOG_ASSERT(f!=NULL);
	int    t              = f->header.drx_tuning - 1;
	size_t frameStepPhase = (((uint64_t)f->header.timeTag) % (((uint64_t) f->header.decFactor) * ((uint64_t) DRX_SAMPLES_PER_FRAME)));
	bool compatible =
		(
			(bs->beam == f->header.drx_beam) &&
			(bs->decFactor == f->header.decFactor) &&
			((bs->freqCode[t] == f->header.freqCode) || (bs->freqCode[t] == FREQ_CODE_UNINITIALIZED)) &&
			(frameStepPhase == bs->stepPhase)
		);
	return compatible;
}

int DrxSpectrometer::compare(DrxFrame* f, DrxBlockSetup* bs){
	LOG_ASSERT(spc!=NULL);
	LOG_ASSERT(blocks!=NULL);
	LOG_ASSERT(bs!=NULL);
	LOG_ASSERT(f!=NULL);
	if (f->header.timeTag < bs->timeTag0)
		return -1;
	if (f->header.timeTag > bs->timeTagN)
		return 1;
	return 0;
}



// we can drop the frame or insert it, or take no action; return true for the first two, false otherwise
bool DrxSpectrometer::insert(DrxFrame* f){
	LOG_ASSERT(spc!=NULL);
	LOG_ASSERT(blocks!=NULL);
	LOG_ASSERT(f!=NULL);
	if (f->header.timeTag < minTimeTag){
		counters.framesDroppedArrivedLate++;
		return true;
	}

	// scan the currently filling list for a block which matches
	bool inserted        = false;
	bool blockStartable  = false;
	bool canStartABlock  = canInsert(&blocks_startable);
	bool canFillNewBlock = canRemove(&blocks_free);

	list<DrxBlockSetup*>::iterator listStart = blocks_filling.begin();
	list<DrxBlockSetup*>::iterator searchPos = listStart;
	list<DrxBlockSetup*>::iterator listEnd   = blocks_filling.end();
	list<DrxBlockSetup*>::iterator clearPos  = listStart;
	DrxBlockSetup* bs                        = NULL;
	DrxBlockSetup* lastCompatibleBlock       = NULL;
	while(
			!inserted &&
			(searchPos != listEnd)
	){
		bs = *searchPos;
		LOG_ASSERT(bs!=NULL);
		LOG_ASSERT(bs->state == BS_FILLING);

		int  order        = compare(f,bs);
		bool compatible   = blockMatch(f,bs);
		if (compatible){
			lastCompatibleBlock = bs;
		} else {
			counters.incompatibleFrames++;
			if ((counters.incompatibleFrames & 0x3fff)==0){
				LOGC(L_ERROR, "INCOMPATIBLE", FATAL2_COLORS);
			}
		}
		switch(order){
		case 0:
			if (!compatible){
				counters.framesDroppedIncompatible++;
				return true;
			} else {
				if (canStartABlock){
					blockStartable = unpack(f,bs);
					counters.framesInsertedExisting++;
					inserted = true;
					continue;
				} else {
					counters.framesStalledBlockUnstartable++;
					return false;
				}
			}
			break;
		case (-1):
			counters.framesDroppedArrivedLate_alt++;
			return true;
			break;
		case 1:
			searchPos++;
			continue;
			break;
		default:
			LOGC(L_ERROR, "Program error: reached unreachable code" __FILE__ + LXS(__LINE__), PROGRAM_ERROR_COLORS);
			break;
		}
	} // while loop

	if (inserted){
		if (blockStartable){
			LOG_ASSERT(canStartABlock);
			LOG_ASSERT(bs!=NULL);
			// flush up to the started block
			while(clearPos != searchPos){
				DrxBlockSetup** mvptr = blocks_dropped.nextIn();
				LOG_ASSERT(mvptr!=NULL);
				*mvptr=*clearPos;
				(*mvptr)->state = BS_DROPPED;
				blocks_dropped.doneIn(mvptr);
				clearPos++;
			}

			// advance the minimum legal timetag
			minTimeTag = nextTimeTagAfterBlock(bs);

			// if this is the last filling block, copy our block setup to a
			// safe place since an out-of order first packet would mess up the frame
			// start of the next block
			if (blocks_filling.size()==1){
				memcpy((void*)&lastFilledBlockSetup,(void*)bs, sizeof(DrxBlockSetup));
				lastFilledBlockSetupGood=true;
			}

			// insert the block into startable list
			DrxBlockSetup** insptr = blocks_startable.nextIn();
			LOG_ASSERT(insptr!=NULL);
			*insptr=bs;
			bs->state = BS_STARTABLE;
			blocks_startable.doneIn(insptr);

			// remove the head of the list upto and including searchPos
			searchPos++;
			blocks_filling.erase(listStart,searchPos);

		}
		return true;
	} else {
		LOG_ASSERT(searchPos == listEnd);
		if (!canFillNewBlock){
			counters.framesStalledWaitingBlock++;
			return false;
		} else {
			// get a new block
			DrxBlockSetup** newPtr = blocks_free.nextOut();
			LOG_ASSERT(newPtr!=NULL);
			DrxBlockSetup* bs_new = *newPtr;
			LOG_ASSERT(bs_new->state == BS_UNUSED);


			// initialize the block
			if (lastCompatibleBlock != NULL){
				initBlockSetup(bs_new, f, lastCompatibleBlock);
			} else {
				if (lastFilledBlockSetupGood && blockMatch(f,&lastFilledBlockSetup)){
					// start stale from last started
					initBlockSetup(bs_new, f, &lastFilledBlockSetup);
				} else {
					// completely fresh block start or
					// incompatible with cached copy
					initBlockSetup(bs_new, f);
				}
			}
			LOG_ASSERT(bs_new->insertionCount == 0);
			LOG_ASSERT(*spc->getCellFills(spc->cellIndex(bs_new->bIdx, 0 ,0))== 0);
			LOG_ASSERT(*spc->getCellFills(spc->cellIndex(bs_new->bIdx, 0 ,1))== 0);
			LOG_ASSERT(*spc->getCellFills(spc->cellIndex(bs_new->bIdx, 1 ,0))== 0);
			LOG_ASSERT(*spc->getCellFills(spc->cellIndex(bs_new->bIdx, 1 ,1))== 0);
			// always invalidate since we have a newer block setup
			lastFilledBlockSetupGood = false;
			if (unpack(f,bs_new)){
				LOGC(L_ERROR, "Program error: first frame of block filled block", PROGRAM_ERROR_COLORS);
			}
			counters.framesInsertedNew++;
			blocks_filling.push_back(bs_new);
			blocks_free.doneOut(newPtr);
			return true;
		}
	}
}

bool DrxSpectrometer::unpack(DrxFrame* f, DrxBlockSetup* bs){
	//printSpecSetup();
	LOG_ASSERT(spc!=NULL);
	LOG_ASSERT(blocks!=NULL);
	LOG_ASSERT(bs!=NULL);
	LOG_ASSERT(f!=NULL);
	LOG_ASSERT(bs->state == BS_FILLING);
	bool    error         = false;
	size_t  satsThisRound = 0;
	size_t  framePos      = (f->header.timeTag - bs->timeTag0) / bs->timeTagStep;
	size_t  polIndex      = f->header.drx_polarization;
	size_t  tunIndex      = f->header.drx_tuning-1;
	size_t  cellIndex     = spc->cellIndex(bs->bIdx, tunIndex, polIndex);
	size_t* fills         = spc->getCellFills(cellIndex);
	size_t* sat_counts    = spc->getCellSatCounts(cellIndex);
	fftwf_complex* idata  = &spc->getCellIdata(cellIndex)[framePos*DRX_SAMPLES_PER_FRAME];

	LOG_ASSERT(framePos           < inputFramesPerCell);
	LOG_ASSERT(bs->insertionCount < inputFramesPerBlock);
	LOG_ASSERT((polIndex==0)||(polIndex==1));
	LOG_ASSERT((tunIndex==0)||(tunIndex==1));
	LOG_ASSERT(fills!=NULL);
	LOG_ASSERT(sat_counts!=NULL);
	LOG_ASSERT(idata!=NULL);
	LOG_ASSERT((*fills) <= intCount);


	// possibly update freq code
	if (unlikely(bs->freqCode[tunIndex]==FREQ_CODE_UNINITIALIZED)){
		bs->freqCode[tunIndex]=f->header.freqCode;
	}

	// update fillcount
	*fills += (DRX_SAMPLES_PER_FRAME / freqCount_or_samp_per_frame);

	// unpack, counting saturation counts along the way
	for (size_t i=0; i<DRX_SAMPLES_PER_FRAME; i+=2){
		signed char _i = f->samples[i].packed;
		signed char _q = f->samples[i+1].packed;
		satsThisRound += (size_t)(SatLUT[_i] | SatLUT[_q]);
		idata[i][0]    = (float)_i;
		idata[i][1]    = (float)_q;
	}
	// update sat counts
	*sat_counts += satsThisRound;

	// sanity checks
	if ((*fills) > intCount){
		LOGC(L_ERROR, "Program error: overfilled cell: " +LXS((*fills)) + ">" + LXS(intCount), PROGRAM_ERROR_COLORS);
		error = true;
	}
	if (framePos >= inputFramesPerCell){
		LOGC(L_ERROR, "Program error: bad framepos: " +LXS(framePos) + ">=" + LXS(inputFramesPerCell), PROGRAM_ERROR_COLORS);
		error = true;
	}
	if(error){
		printFrameSetup(f,bs);
		printBlockSetup(bs);
	}

	counters.framesInserted++;
	bs->insertionCount++;
	/*
	if (bs->insertionCount == inputFramesPerBlock){
		LOGC(L_ERROR, "BLOCK STARTABLE", FATAL2_COLORS);
		printSpecSetup();
		printBlockSetup(bs);
		printFrameSetup(f,bs);

	}
	*/
	return (bs->insertionCount == inputFramesPerBlock);
}

bool DrxSpectrometer::canRemove(ObjectBuffer<DrxBlockSetup*>* from){
	return from->nextOut() != NULL;
}

bool DrxSpectrometer::canInsert(ObjectBuffer<DrxBlockSetup*>* to){
	return to->nextIn() != NULL;
}
string DrxSpectrometer::getObjName(){
	return "DrxSpectrometer";
}
// checks if the head of 'from' is movable to the tail of 'to'
bool DrxSpectrometer::canMove(ObjectBuffer<DrxBlockSetup*>* from, ObjectBuffer<DrxBlockSetup*>* to){
	DrxBlockSetup** destptr=to->nextIn();
	if (!destptr)
		return false;
	DrxBlockSetup** sourceptr=from->nextOut();
	if (!sourceptr)
		return false;
	return true;
}

// moves the head of 'from' to the tail of 'to'
void DrxSpectrometer::doMove(ObjectBuffer<DrxBlockSetup*>* from,  ObjectBuffer<DrxBlockSetup*>* to){
	DrxBlockSetup** destptr=to->nextIn();
	DrxBlockSetup** sourceptr=from->nextOut();
	if (!destptr || !sourceptr){
		LOGC(L_ERROR, "["+getObjName()+"]: Tried to doMove(x,y) but one of them was not a viable pointer", ACTOR_ERROR_COLORS);
	}
	*destptr = *sourceptr;
	from->doneOut(sourceptr);
	to->doneIn(destptr);
}

/*
void DrxSpectrometer::generateTestPattern(float* specdata){
	return;
	static size_t curline = 0;
	size_t pcount = StokesSize(OutT);
	for (size_t t=0; t<2; t++){
		for (size_t p=0; p<pcount; p++){
			for (size_t f=0; f<freqCount_or_samp_per_frame; f++){
				specdata[(t*freqCount_or_samp_per_frame*pcount) + (f*pcount) + p] = getTestPattern(p,f,curline);
			}
		}
	}
	curline++;
}
*/

SpectrometerCounters* DrxSpectrometer::getCounters(){
	return &counters;
}

void DrxSpectrometer::printSpecSetup(){
	cout << "======================= Spec Setup ============================" << endl;
	cout << "Freq Count:          " <<        freqCount_or_samp_per_frame << endl;
	cout << "Int Count:           " <<        intCount << endl;
	cout << "outputHeaderSize:    " <<        outputHeaderSize << endl;
	cout << "outputDataSize:      " <<        outputDataSize << endl;
	cout << "outputBlockSize:     " <<        outputBlockSize << endl;
	cout << "inputFramesPerCell:  " <<        inputFramesPerCell << endl;
	cout << "inputFramesPerBlock: " <<        inputFramesPerBlock << endl;
	cout << "===============================================================" << endl;

}

void DrxSpectrometer::printBlockSetup(DrxBlockSetup* bs){
	cout << "======================= Block Setup ===========================" << endl;
	cout << "beam:              " << 		bs->beam << endl;
	cout << "decFactor:         " << 		bs->decFactor << endl;
	cout << "timeOffset:        " << 		bs->timeOffset << endl;
	cout << "timeTagStep:       " << 		bs->timeTagStep << endl;
	cout << "timeTag0:          " << 		bs->timeTag0 << " ("<<Time::humanReadable(bs->timeTag0)<<")"<< endl;
	cout << "timeTagN:          " << 		bs->timeTagN << " ("<<Time::humanReadable(bs->timeTagN)<<")"<< endl;
	cout << "N (derived)        " << 		(bs->timeTagN-bs->timeTag0)/bs->timeTagStep << endl;
	cout << "freqCode[0]:       " << 		bs->freqCode[0] << endl;
	cout << "freqCode[1]:       " << 		bs->freqCode[1] << endl;
	cout << "stepPhase:         " << 		bs->stepPhase << endl;
	//cout << "state:             " << 		bs->state << endl;
	cout << "spc_block_index:   " << 		(ssize_t) bs->bIdx << endl;
	//cout << "ticketFrameIndex:  " << 		(ssize_t) bs->ticketFrameIndex << endl;
	cout << "===============================================================" << endl;
}
void DrxSpectrometer::printFrameSetup(DrxFrame* f, DrxBlockSetup* bs){
	cout << "======================= Frame Setup ===========================" << endl;

	cout << "beam:                " << 		f->header.drx_beam << endl;
	cout << "decFactor:           " << 		f->header.decFactor << endl;
	cout << "timeOffset:          " << 		f->header.timeOffset << endl;
	size_t tts = f->header.decFactor*DRX_SAMPLES_PER_FRAME;
	cout << "timeTagStep: (der'd) " << 		tts << endl;
	cout << "timeTag:             " << 		f->header.timeTag << " ("<<Time::humanReadable(f->header.timeTag)<<")"<< endl;
	cout << "TimeTagN     (der'd) " << f->header.timeTag + (tts * freqCount_or_samp_per_frame * intCount / DRX_SAMPLES_PER_FRAME) << endl;

	if (bs != NULL){
		cout << "n:   (derived)       " << ((f->header.timeTag - bs->timeTag0) / bs->timeTagStep) << endl;
	}
	cout << "freqCode["<<((f->header.drx_tuning == DRX_TUN_0)?"0":"1")<<"]:         " << f->header.freqCode << endl;
	cout << "stepPhase:   (der'd) " << 		(((uint64_t)f->header.timeTag) % (((uint64_t) f->header.decFactor) * ((uint64_t) DRX_SAMPLES_PER_FRAME))) << endl;
	cout << "===============================================================" << endl;
}
string DrxSpectrometer::specReport(){
	double elapsed = rptTimer.getRuntime();
	if (elapsed==0) elapsed = 1.0/1000000;
	double cbw = ((double)counters.blocksCompleted) / (elapsed);
	stringstream ss("");


	size_t rx_rate =  getReceiveRate();
	size_t rx_pct  =  (rx_rate * 100ll) / (120ll * 1048576ll);
	size_t tx_rate =  getSendRate();
	size_t tx_pct  =  (tx_rate * 100ll) / (4ll * 1048576ll);


	ss << "==========================================================================================" << endl;
        ss << "== Spectrometer Report:                                                                 ==" << endl;
	ss << "==  Mode:  "<<setw(10)<<OutT.name()<<"         Frequency Ch. "<<setw(6)<<freqCount_or_samp_per_frame<<"   Integration count "<<setw(8)<<intCount<<"         ==" << endl;
	ss << "==  Frames                                                                              ==" << endl;
	ss << "==    "<<setw(9)<<counters.framesReceived<<"                                       (received)                        ==" << endl;
	ss << "==    "<<setw(9)<<counters.framesInserted<<" / "<<setw(9)<<counters.framesInsertedNew<<" / "<<setw(9)<<counters.framesInsertedExisting<<"               (inserted / init / join)          ==" << endl;
	ss << "==    "<<setw(9)<<counters.framesDroppedIncompatible<<" / "<<setw(9)<<counters.framesDroppedArrivedLate<<" / "<<setw(9)<<counters.framesDroppedArrivedLate_alt<<"              (incompatible / late1 / late2)     ==" << endl;
	ss << "==    "<<setw(9)<<counters.framesInsertedFresh<<" / "<<setw(9)<<counters.framesInsertedStale<<"                           (fresh / stale )                  ==" << endl;
	ss << "==  Blocks                                                                              ==" << endl;
	ss << "==    "<<setw(9)<<counters.blocksStarted<<" / "<<setw(9)<<counters.blocksCompleted<<" / "<<setw(9)<<counters.blocksDropped<<"               (started / completed / dropped)   ==" << endl;
//	ss << "==    "<<setw(9)<<counters.blocksStartedFull<<" / "<<setw(9)<<counters.blocksStartedHighWater<<"                           (full-start / highwater-start)    ==" << endl;
	ss << "==  Queues      (used / size)                                                           ==" << endl;
	ss << "==    "<<setw(9)<<blocks_free.used()     <<" / "<<setw(9)<<blocks_free.size()     <<"                           (free)                            ==" << endl;
	ss << "==    "<<setw(9)<<blocks_filling.size()  <<" / "<<setw(9)<<(INFINITY)             <<"                           (filling)                         ==" << endl;
	ss << "==    "<<setw(9)<<blocks_startable.used()<<" / "<<setw(9)<<blocks_startable.size()<<"                           (startable)                       ==" << endl;
	ss << "==    "<<setw(9)<<blocks_computing.used()<<" / "<<setw(9)<<blocks_computing.size()<<"                           (processing)                      ==" << endl;
	ss << "==    "<<setw(9)<<blocks_dropped.used()  <<" / "<<setw(9)<<blocks_dropped.size()  <<"                           (dropped)                         ==" << endl;
	ss << "==                                                                                      ==" << endl;
	ss << "==  Runtime: "<<setw(9)<<setprecision(5)<<elapsed<<" s          Compute B/W: "<<setw(9)<<setprecision(4)<<cbw<<" Blk/s                          ==" << endl;
	ss << "==  Receive rate:        ";
		progressbar(rx_pct,34,ss);
		ss << " ";
		ss << Storage::humanReadableBW(rx_rate);
		ss << "     ==" << endl;

	ss << "==  Input Buffer Usage:  ";
		progressbar(source->usagePct(),34,ss);
		ss << "                    ==" << endl;

	ss << "==  Send rate:           ";
		progressbar(tx_pct,34,ss);
		ss << " ";
		ss << Storage::humanReadableBW(tx_rate);
		ss << "     ==" << endl;

	ss << "==  Output Buffer Usage: ";
		progressbar(TicketBuffer::usagePct(),34,ss);
		ss << "                     ==" << endl;

	ss << "checkBlockStart_WaitOutbuf        " << counters.checkBlockStart_WaitOutbuf << endl;
	ss << "checkBlockDone_Done               " << counters.checkBlockDone_Done << endl;
	ss << "checkBlockDone_NotDone            " << counters.checkBlockDone_NotDone << endl;
	ss << "checkBlockDone_Error              " << counters.checkBlockDone_Error << endl;
	ss << "checkBlockDone_WaitFree           " << counters.checkBlockDone_WaitFree << endl;
	ss << "checkBlockDone_NotComputing       " << counters.checkBlockDone_NotComputing << endl;
	ss << "framesStalledWaitingBlock         " << counters.framesStalledWaitingBlock << endl;
	ss << "framesStalledBlockUnstartable     " << counters.framesStalledBlockUnstartable << endl;
	ss << "Illegal Frames                    " << counters.illegalFrames << endl;
	ss << "==========================================================================================" << endl;
	return ss.str();

}
