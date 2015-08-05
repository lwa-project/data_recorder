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
 * Socket.c
 *
 *  Created on: Oct 18, 2009
 *      Author: chwolfe2
 */
#include	<sys/stat.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<fcntl.h>
#include	<errno.h>
#include 	<netdb.h>
#include 	<string.h>
#include 	<stdlib.h>
#include 	<stdio.h>
#include    <assert.h>
#include	"Socket.h"
#include	"Message.h"
#include "Log.h"

StatusCode Socket_Create(Socket* s){
	assert(s!=NULL);
	memset((void *)s, 0, SocketSize);
	s->Handle=-1;
	return SUCCESS;

}
StatusCode Socket_Destroy(Socket* s){
	assert(s!=NULL);
	if (s->Handle!=-1) close(s->Handle);
	memset((void *)s, 0, SocketSize);
	s->Handle=-1;
	return SUCCESS;
}
StatusCode Socket_OpenRecieve(Socket* s, unsigned short port){
	Log_Add("[SOCKET] Info: open receive on port %hd\n",port);
	assert(s!=NULL);
	assert(s->Handle == -1);
	s->Handle  					= -1;
	s->Address.sin_family 		= AF_INET;
	s->Address.sin_port			= htons(port);
	s->Address.sin_addr.s_addr 	= INADDR_ANY;
	memset(s->Address.sin_zero, '\0', sizeof s->Address.sin_zero);
	// create the socket
	s->Handle = socket(AF_INET, SOCK_DGRAM, 0);
	if (s->Handle < 0)	{
		Log_Add("[SOCKET] Error: socket() error\n");
		return FAILURE;
	}

	// bind the socket to localhost::port specified above
	int result = bind(s->Handle, (struct sockaddr *) &s->Address,	sizeof(struct sockaddr));
	if (result != 0) {
		close(s->Handle);
		Log_Add("[SOCKET] Error: bind() error\n");
		return FAILURE;
	}
	//* set socket to be non-blocking
	result = fcntl(s->Handle, F_GETFL);
	if (result < 0){
		Log_Add("[SOCKET] Error: Fcntl F_GETFL\n");
		close(s->Handle);
		return FAILURE;
	}
	result = result | O_NONBLOCK;
	result = fcntl(s->Handle, F_SETFL, result);
	if (result < 0){
		Log_Add("[SOCKET] Error: Fcntl F_SETFL\n");
		close(s->Handle);
		return FAILURE;
	}
	return SUCCESS;
}
StatusCode Socket_OpenSend(Socket* s, char* destinationURL, unsigned short port){
	Log_Add("[SOCKET] Info: open send %s on port %hd\n",destinationURL,port);
	assert(s!=NULL);
	assert(s->Handle == -1);

	// initialize structure for destination ip information (typically MCS)
	s->Handle 				= -1;
	s->Address.sin_family 	= AF_INET;
	s->Address.sin_port 	= htons(port);
	s->host 				= gethostbyname(destinationURL);
	memcpy((char*)& s->Address.sin_addr,(char*)s->host->h_addr, s->host->h_length);

	// create the socket
	s->Handle = socket(AF_INET, SOCK_DGRAM, 0);
	if (s->Handle < 0) {
		return FAILURE;
	}

	//* set socket to be non-blocking
	int result = fcntl(s->Handle, F_GETFL);
	if (result < 0){
		close(s->Handle);
		return FAILURE;
	}
	result = result | O_NONBLOCK;
	result = fcntl(s->Handle, F_SETFL, result);
	if (result < 0){
		close(s->Handle);
		return FAILURE;
	}
	return SUCCESS;
}
StatusCode Socket_Close(Socket* s){
	assert(s!=NULL);
	if(s->Handle != -1)	close(s->Handle);
	s->Handle=-1;
	s->bytes = 0;
	s->packets= 0;
	return SUCCESS;
}
StatusCode Socket_Recieve(Socket* s, char* buffer, size_t* size){
	ssize_t result =  recv(s->Handle, (void *)buffer, sizeof(BaseMessage), O_NONBLOCK);
	if (result < 0){
		if (errno != EWOULDBLOCK){
			perror("[SOCKET] Error: recv()");
			Log_Add("[SOCKET] Error Info: Hdl:%d  buf=%lx  siz=%lx\n",s->Handle,(size_t)buffer, (size_t)sizeof(BaseMessage));
			return FAILURE;
		} else {
			return NOT_READY;
		}
	} else {
		*size = result;
		s->bytes += result;
		s->packets++;
		return SUCCESS;
	}
}

StatusCode Socket_Send(Socket* s, char* buffer, size_t size){
	ssize_t result =  sendto(s->Handle, (void *)buffer, size, 0, (struct sockaddr*)(&(s->Address)), sizeof(struct sockaddr));
	if (result < 0){
		if (errno != EWOULDBLOCK)
			return FAILURE;
		else
			return NOT_READY;
	} else {
		s->bytes += size;
		s->packets++;
		return SUCCESS;
	}
}

