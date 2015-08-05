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
 * RingQueue.h
 *
 *  Created on: Oct 18, 2009
 *      Author: chwolfe2
 */

#ifndef RINGQUEUE_H_
#define RINGQUEUE_H_
#include "Defines.h"
#include <stdlib.h>

// size to allocate for ring queue. is 1.5 GiB
#define RING_QUEUE_STATIC_SIZE 1610612736

// size of removal requests. 256kB corresponds to size of packet written to hard drive.
#define RING_QUEUE_REMOVAL_SIZE 262144


typedef struct __ringqueue{
  void * data;
  size_t in;
  size_t out;
  size_t totalSize;
  size_t bytesUsed;
  size_t allocatedSize;
  //size_t insertionSize;
  boolean allowUndersizeRemoval;
} RingQueue;

StatusCode RingQueue_Create(RingQueue* rq, size_t insertionSize);
StatusCode RingQueue_Destroy(RingQueue* rq);
StatusCode RingQueue_Reset(RingQueue* rq);
StatusCode RingQueue_NextInsertionPoint(RingQueue* rq, char** insertionPoint, size_t bytesToInsert, size_t * bytesInsertable);
StatusCode RingQueue_NotifyInsertionComplete(RingQueue* rq, size_t bytesInserted);
StatusCode RingQueue_NextRemovalPoint(RingQueue* rq, char** removalPoint, size_t bytesToRemove, size_t * bytesRemovable);
StatusCode RingQueue_NotifyRemovalComplete(RingQueue* rq, size_t bytesRemoved);

#endif /* RINGQUEUE_H_ */
