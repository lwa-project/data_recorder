/*
  MCS-DROS: Monitor and Control System - Data Recorder Operating Software
  Copyright (C) 2009-2010  Virginia Tech, Christopher Wolfe <chwolfe2@vt.edu>

  This file is part of MCS-DROS.

  MCS-DROS is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  MCS-DROS is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with MCS-DROS.  If not, see <http://www.gnu.org/licenses/>.
*/
/*
 * RingQueue.c
 *
 *  Created on: Oct 18, 2009
 *      Author: chwolfe2
 */

// size is always assumed to be 1.5GiB --> no longer the case (jupiter search uses additional queues of smaller sizes)
#include "RingQueue.h"
#include <sys/mman.h>
#include <assert.h>
#include <string.h>

//#define RING_QUEUE_STATIC_SIZE 1610612736
#define RING_QUEUE_REMOVAL_SIZE 262144
#define RING_QUEUE_OVERLAP_GRACE RING_QUEUE_REMOVAL_SIZE

StatusCode RingQueue_Create(RingQueue* rq, size_t sizeInMB){
	assert(rq!=NULL);
	assert(rq->data==NULL);
	rq->data = mmap(NULL, (sizeInMB * 1048576l) + RING_QUEUE_OVERLAP_GRACE, PROT_READ | PROT_WRITE, MAP_PRIVATE	| MAP_ANONYMOUS | MAP_POPULATE | MAP_LOCKED, -1, 0);
	if (rq->data == MAP_FAILED) {
		return FAILURE;
	}
	rq->in = 0;
	rq->out = 0;
	rq->totalSize = (sizeInMB * 1048576l);
	rq->bytesUsed = 0;
	rq->allocatedSize = (sizeInMB * 1048576l) + RING_QUEUE_OVERLAP_GRACE;
	//rq->insertionSize = insertionSize;
	rq->allowUndersizeRemoval=false;
	return SUCCESS;
}
StatusCode RingQueue_Destroy(RingQueue* rq){
	assert(rq!=NULL);
	assert(rq->data != NULL);
	if ((munmap(rq->data, rq->allocatedSize)) == -1)
		return FAILURE;
	rq->in = 0;
	rq->out = 0;
	rq->totalSize = 0;
	rq->bytesUsed = 0;
	rq->allocatedSize = 0;
	rq->data=NULL;
	//rq->insertionSize=0;
	return SUCCESS;
}
StatusCode RingQueue_Reset(RingQueue* rq){
	assert(rq!=NULL);
	assert(rq->data != NULL);
	rq->in = 0;
	rq->out = 0;
	rq->bytesUsed = 0;
	rq->allowUndersizeRemoval=false;
	return SUCCESS;
}
StatusCode RingQueue_NextInsertionPoint(RingQueue* rq, char** insertionPoint, size_t bytesToInsert, size_t * bytesInsertable){
//StatusCode RingQueue_NextInsertionPoint(RingQueue* rq, char** insertionPoint){
	if (bytesInsertable) *bytesInsertable = rq->totalSize - rq->bytesUsed;
	if ((rq->totalSize - rq->bytesUsed) < bytesToInsert){
		*insertionPoint = NULL;
		return FAILURE;
	} else {
		*insertionPoint =  &(((char*) rq->data)[rq->in]);
		return SUCCESS;
	}
}

StatusCode RingQueue_NotifyInsertionComplete(RingQueue* rq, size_t bytesInserted){
	rq->bytesUsed += bytesInserted;
	rq->in += bytesInserted;
	if (rq->in >= rq->totalSize) {
		if (rq->in > rq->totalSize ) {
			//wrote past the end, so we need to copy some bytes back to the beginning of the queue
			memcpy(rq->data, (void*) &(((char*) rq->data)[rq->totalSize]),(rq->in - rq->totalSize));
			rq->in=(rq->in - rq->totalSize);
		} else {
			rq->in = 0;
		}
	}
	return SUCCESS;
}


StatusCode RingQueue_NextRemovalPoint(RingQueue* rq, char** removalPoint, size_t bytesToRemove, size_t * bytesRemovable){
//StatusCode RingQueue_NextRemovalPoint(RingQueue* rq, char** removalPoint, size_t * bytesRemovable){
	if ((rq->bytesUsed) < bytesToRemove/*RING_QUEUE_REMOVAL_SIZE*/){
		if(rq->allowUndersizeRemoval){
			*bytesRemovable = rq->bytesUsed;
			*removalPoint = &(((char*) rq->data)[rq->out]);
			return SUCCESS;
		} else {
			*bytesRemovable = 0;
			*removalPoint = NULL;
			return NOT_READY;
		}
	} else {
		*bytesRemovable = bytesToRemove/*RING_QUEUE_REMOVAL_SIZE*/;
		*removalPoint = &(((char*) rq->data)[rq->out]);
		return SUCCESS;
	}
}

StatusCode RingQueue_NotifyRemovalComplete(RingQueue* rq, size_t bytesRemoved){
	if (bytesRemoved>rq->bytesUsed){
		RingQueue_Reset(rq);
	} else {
		rq->bytesUsed -= bytesRemoved;
		rq->out += bytesRemoved;
		if (rq->out >= rq->totalSize) {
			rq->out = 0;
		}
	}
	return SUCCESS;
}

