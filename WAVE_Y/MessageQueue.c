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
 * MessageQueue.c
 *
 *  Created on: Oct 18, 2009
 *      Author: chwolfe2
 */
#include "MessageQueue.h"
#include	<errno.h>
#include	<assert.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<sys/stat.h>

StatusCode MessageQueue_Create(MessageQueue* msgQ, char* name){
	assert(msgQ!=NULL);
  // set message queue attributes
	msgQ->handle                = -1;
	msgQ->attributes.mq_flags   = O_NONBLOCK;
	msgQ->attributes.mq_maxmsg  = DEFAULT_QUEUE_SIZE;
	//msgQ->attributes.mq_msgsize = MESSAGE_MAXIMUM_SIZE;
	msgQ->attributes.mq_msgsize = sizeof(BaseMessage);
	msgQ->attributes.mq_curmsgs = 0;
	msgQ->name                  = name;
  // create message queue
	msgQ->handle = mq_open(msgQ->name, O_RDWR | O_NONBLOCK | O_CREAT , DEFFILEMODE , &(msgQ->attributes) );
	if (msgQ->handle < 0)
		return FAILURE;
	else
		return SUCCESS;
}

StatusCode MessageQueue_Destroy(MessageQueue* msgQ){
	assert(msgQ!=NULL);
	mq_unlink(msgQ->name);
	mq_close(msgQ->handle);
	return SUCCESS;
}

StatusCode MessageQueue_Enqueue(MessageQueue* msgQ, BaseMessage* msg){
	mqd_t result = mq_send(msgQ->handle, (const char *)msg, sizeof(BaseMessage), 0);
	if (result != 0){
		if (errno!=EWOULDBLOCK){
			perror("mq_Send");
			return FAILURE;

		} else
			return NOT_READY;
	} else {
		return SUCCESS;
	}
}

StatusCode MessageQueue_Dequeue(MessageQueue* msgQ, BaseMessage* msg){
	mqd_t result = mq_receive(msgQ->handle, (char *)msg, sizeof(BaseMessage),NULL);
	if (result < 0){
		if (errno!=EWOULDBLOCK)
			return FAILURE;
		else
			return NOT_READY;
	} else {
		return SUCCESS;
	}
}
