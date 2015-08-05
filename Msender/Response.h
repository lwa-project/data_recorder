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
#ifndef RESPONSE_H_
#define RESPONSE_H_

#include "Message.h"

class Response : public virtual Message
{
public:
	string getAcceptReject() const;
	string getGeneralStatus() const;
	string getComment() const;
	Response(char * rawdata, size_t messageSize);
	virtual ~Response();
};
ostream& operator<< (ostream & os,  const Response &p);
#endif /*RESPONSE_H_*/
