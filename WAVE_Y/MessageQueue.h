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
 * MessageQueue.h
 *
 *  Created on: Oct 18, 2009
 *      Author: chwolfe2
 */

#ifndef MESSAGEQUEUE_H_
#define MESSAGEQUEUE_H_
#include    <mqueue.h>
#include "Defines.h"
#include "Message.h"

#define DEFAULT_QUEUE_SIZE 10

typedef struct __MessageQueue{
  mqd_t          handle;
  struct mq_attr attributes;
  char *         name;
} MessageQueue;


StatusCode MessageQueue_Create(MessageQueue* msgQ, char* name);
StatusCode MessageQueue_Destroy(MessageQueue* msgQ);
StatusCode MessageQueue_Enqueue(MessageQueue* msgQ, BaseMessage* msg);
StatusCode MessageQueue_Dequeue(MessageQueue* msgQ, BaseMessage* msg);

#endif /* MESSAGEQUEUE_H_ */
