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
 * Socket.h
 *
 *  Created on: Oct 18, 2009
 *      Author: chwolfe2
 */

#ifndef SOCKET_H_
#define SOCKET_H_
#include "Defines.h"
#include	<sys/socket.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<sys/select.h>
#include	<netinet/in.h>
#include 	<arpa/inet.h>
#include 	<netdb.h>

typedef struct __Socket{
  int              Handle;
  struct sockaddr_in    Address;
  struct sockaddr_in    SourceAddress;
  socklen_t        		SourceAddressSize;
  struct hostent * 		host;				// only used for outbound sockets
  size_t 				bytes;     			// sent or received total
  size_t 				packets;   			// sent or received total
} Socket;
#define SocketSize sizeof(Socket)


StatusCode Socket_Create(Socket* s);
StatusCode Socket_Destroy(Socket* s);
StatusCode Socket_OpenRecieve(Socket* s, unsigned short port);
StatusCode Socket_OpenSend(Socket* s, char* destinationURL, unsigned short port);
StatusCode Socket_Close(Socket* s);
StatusCode Socket_Recieve(Socket* s, char* buffer, size_t* size);
StatusCode Socket_Send(Socket* s, char* buffer, size_t size);




#endif /* SOCKET_H_ */
