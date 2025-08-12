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
#ifndef SOCKET_H_
#define SOCKET_H_
#include	<sys/socket.h>
#include	<netinet/in.h>

class Socket{
private:
	int 				inboundSocketHandle;
	sockaddr_in 		inboundAddress;
	sockaddr_in 		inboundSourceAddress;
	socklen_t			inboundSourceAddressSize;
	unsigned short		inboundport;
	bool				inboundIsOpen;
	int 				outboundSocketHandle;
	sockaddr_in 		outboundAddress;
	unsigned short		outboundport;
	bool				outboundIsOpen;
	size_t  			_LifetimeBytesSent;
	size_t 				_LifetimeBytesReceived;

public:
	Socket();
	virtual ~Socket();
	bool		inboundConnectionIsOpen();
	bool		outboundConnectionIsOpen();
	void		closeInbound();
	void		closeOutbound();
	void	openInbound(unsigned short port);
	void	openOutbound(const char * hostIP, unsigned short port);
	size_t	send(char* data, size_t length);
	size_t	receive(char* data, size_t maxLength);
	size_t  getBytesSent();
	size_t  getBytesReceived();
}; 

#endif /*SOCKET_H_*/
