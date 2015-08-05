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
#include "Socket.h"
#include	<sys/socket.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<sys/select.h>
#include	<netinet/in.h>
#include 	<arpa/inet.h>
#include	<fcntl.h>
#include	<errno.h>
#include 	<netdb.h>
#include	<string.h>
#include	<time.h>
#include <iostream>

using namespace std;
Socket::Socket():inboundIsOpen(false),outboundIsOpen(false){
	//cout << "Socket initialized.\n";
}

Socket::~Socket(){
	//cout << "Socket deleted.\n";
}


void Socket::openInbound(unsigned short port){

	if(inboundIsOpen) close( inboundSocketHandle);
	inboundAddress.sin_family=AF_INET;
	inboundAddress.sin_port=htons(port);
	inboundAddress.sin_addr.s_addr = INADDR_ANY;
	memset(inboundAddress.sin_zero, '\0', sizeof inboundAddress.sin_zero);
	_LifetimeBytesReceived=0;
	// create the socket
	inboundSocketHandle=socket(AF_INET, SOCK_DGRAM, 0);
	inboundIsOpen=false;
	if (inboundSocketHandle < 0){
		cout << "Inbound socket could not be opened.\n";
		return;
	}else{
		// bind the socket to localhost,udp port specified above
		int result=bind ( inboundSocketHandle , (struct sockaddr *) &inboundAddress , sizeof(sockaddr));
		if (result!=0){
			cout << "Inbound socket could not be opened.\n";
			close( inboundSocketHandle );
			return;
		}
		// set socket to be non-blocking
		result = fcntl(inboundSocketHandle, F_GETFL);
		if (result < 0){
			cout << "Inbound socket could not get flags\n";
			close(inboundSocketHandle);
			return;
		}
		result = result | O_NONBLOCK;
		result = fcntl(inboundSocketHandle, F_SETFL, result);
		if (result < 0){
			cout << "Inbound socket could not set NON-BLOCKING flag\n";
			close(inboundSocketHandle);
			return;
		}
		inboundIsOpen=true;
	}
}
void Socket::openOutbound(const char * hostIP, unsigned short port){
	if(outboundIsOpen) close( outboundSocketHandle);

	// create the server address structure used in sending packets
	outboundAddress.sin_family=AF_INET;
	outboundAddress.sin_port=htons(port);
	struct hostent *hp = gethostbyname(hostIP);
	//struct hostent *hp = gethostbyname("192.168.1.10");
	memcpy((char*)&outboundAddress.sin_addr,(char*)hp->h_addr,hp->h_length);
	_LifetimeBytesSent=0;
	// create the socket
	outboundSocketHandle=socket(AF_INET, SOCK_DGRAM, 0);
	if (outboundSocketHandle < 0){
		outboundIsOpen=false;
		cout << "Outbound socket could not be opened.\n";
	} else {
		outboundIsOpen=true;
		//cout << "Outbound socket opened by request.\n";
	}
	return;

}

bool Socket::inboundConnectionIsOpen(){

	return inboundIsOpen;
}

bool Socket::outboundConnectionIsOpen(){

	return outboundIsOpen;
}

size_t	Socket::send(char* data, size_t length){
	size_t bytesSent=0;
	ssize_t		status			= 0;
	size_t		attemptCount	= 0;
	int			rError			= 0;
	while ((bytesSent<length) && (attemptCount < length)){
		status 	=  sendto(	outboundSocketHandle, (void*) &data[bytesSent], length-bytesSent, 0, (sockaddr*)&outboundAddress, sizeof(outboundAddress));
		switch (status){
			case -1 :
				rError=errno;
				switch (rError){
					case EWOULDBLOCK:	break; // do nothing, send until we get an error or message is sent
					case EBADF		:	cout << "not valid file descriptor.\n"; break;
					case ECONNRESET	:	cout << "connection closed by peer.\n"; break;
					case EINTR		:	cout << "receive interrupted.\n"; break;
					case EINVAL		:	cout << "no out-of-band data available.\n"; break;
					case ENOTCONN	:	cout << "not connected.\n"; break;
					case ENOTSOCK	:	cout << "not a socket.\n"; break;
					case EOPNOTSUPP	:	cout << "invalid option.\n"; break;
					case ENOBUFS	:	cout << "insufficient system resources.\n"; break;
					case ENOMEM		:	cout << "insufficient memory.\n"; break;
					case EAFNOSUPPORT:	cout << "incompatible address.\n"; break;
					case EMSGSIZE	:	cout << "message size too large.\n"; break;
					case EPIPE		:	cout << "connection shut down.\n"; break;
					case EACCES		:	cout << "insufficient permission for this operation.\n"; break;
					case EDESTADDRREQ:	cout << "missing destination address.\n"; break;
					case EHOSTUNREACH:	cout << "destination unreachable.\n"; break;
					case EISCONN	:	cout << "socket in use.\n"; break;
					case ENETDOWN	:	cout << "network interface unavailable.\n"; break;
					case ENETUNREACH:	cout << "network unreachable.\n"; break;
					default 		:	cout << "unknown error code.\n"; break;
				};
			case  0 :
				attemptCount++;				break;
			default :
				bytesSent+=status;		break;
		}
	}
	_LifetimeBytesSent+=bytesSent;
	return bytesSent;
}

size_t	Socket::receive(char* data, size_t maxLength){
	ssize_t		status			= 0;
	int			rError			= 0;
	size_t bytesReceived;
	status = recvfrom(inboundSocketHandle, data ,maxLength , 0, (sockaddr *)&inboundSourceAddress, &inboundSourceAddressSize);
	switch (status){
		case -1:
			rError=errno;
			switch (rError) {
				case EWOULDBLOCK:	bytesReceived=0; break;
				case EBADF		:	cout << "not valid file descriptor.\n"; break;
				case ECONNRESET	:	cout << "connection closed by peer.\n"; break;
				case EFAULT		:	cout << "invalid pointer.\n"; break;
				case EINTR		:	cout << "receive interrupted.\n"; break;
				case EINVAL		:	cout << "no out-of-band data available.\n"; break;
				case ENOTCONN	:	cout << "not connected.\n"; break;
				case ENOTSOCK	:	cout << "not a socket.\n"; break;
				case EOPNOTSUPP	:	cout << "invalid option.\n"; break;
				case ETIMEDOUT	:	cout << "connection timeout.\n"; break;
				case EIO		:	cout << "filesystem input/output error.\n"; break;
				case ENOBUFS	:	cout << "insufficient system resources.\n"; break;
				case ENOMEM		:	cout << "insufficient memory.\n"; break;
				case ENOSR		:	cout << "insufficient stream resources.\n"; break;
				default 		:	cout << "unknown error code.\n"; break;
			};
			bytesReceived=0; break;
		case  0: // socket shut down on other end. since udp, shouldn't get here
			cout << "Socket remotely disconnected (shouldn't happen).\n";
			bytesReceived=0; break;
		default:
			bytesReceived=status;
	};
	_LifetimeBytesReceived+=bytesReceived;
	return bytesReceived;
}
void	Socket::closeInbound(){
	//cout << "Inbound socket closed by request.\n";
	close(inboundSocketHandle);
	inboundSocketHandle=-1;
}
void	Socket::closeOutbound(){
	//cout << "Outbound socket closed by request.\n";
	close(outboundSocketHandle);
	outboundSocketHandle=-1;

}



size_t Socket::getBytesSent(){
	return _LifetimeBytesSent;
}
size_t Socket::getBytesReceived(){
	return _LifetimeBytesReceived;
}
