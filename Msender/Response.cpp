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
#include "Response.h"
#include <cstring>
#include <iomanip>
#include <sstream>
#include <iostream>
using namespace std;

Response::Response(char * rawdata, size_t messageSize):
Message(rawdata, messageSize){
}
Response::~Response()
{
}
string Response::getAcceptReject() const{
	string rv = "Error in response length";
	if (getDataLength()>=1){
		if 		(_data.fields._data[0]=='A') rv="Accepted";
		else if (_data.fields._data[0]=='R') rv="Rejected";
		else rv="Unknown response code";
	}
	return rv;
}
string Response::getGeneralStatus() const{
	string rv = "Error in response length";
	char buf[10];
	if (getDataLength()>=8){
		for (int i=0;i<7;i++)
			buf[i]=_data.fields._data[i+1];
		buf[7]='\0';
		rv=string(buf);
	}
	return rv;
}
string Response::getComment() const{
	string rv = "No Comment Specified";
	if (getDataLength()>8){
		rv="";
		for (size_t i=0;i<getDataLength()-8;i++)
			rv.push_back (_data.fields._data[i+8]);
	}
	return rv;
}

ostream& operator<< (ostream & os,  const Response &p){
	os 	<< "[recv]Response :" << endl
		<< "[recv]\tSender:                     " << p.getSender() << endl
		<< "[recv]\tDestination:                " << p.getDestination() << endl
		<< "[recv]\tType:                       " << p.getType() << endl
		<< "[recv]\tReference:                  " << p.getReferenceNumber() << endl
		<< "[recv]\tModified Julian Day:        " << p.getMJD() << endl
		<< "[recv]\tMilliseconds Past Midnight: " << p.getMPM() << endl
		<< "[recv]\tAccept/Reject:              " << p.getAcceptReject() << endl
		<< "[recv]\tGeneral Status:             " << p.getGeneralStatus() << endl
		<< "[recv]\tComment:                    " << p.getComment() << endl
		<< "[recv]\tData Length:                " << p.getDataLength() << endl
		<< "[recv]\tData: " << endl;
	size_t dlen;
	char*  data=p.getData(&dlen);
	size_t iter=0;
	os << hex;
	while ((dlen-iter) >= 16){
		os << "[recv]\t\t<" << setw(4)<< iter << ">:   ";
		for ( size_t i=0; i<16; i++){
			os << setw(2) << (((unsigned int) data[iter+i])&0xff) << " ";
		}
		iter+=16;
		os << endl;
	}
	if (dlen-iter) os <<  "[recv]\t\t<" << setw(4)<< iter << ">:   ";
	while (iter<dlen){
		os << setw(2) << (((unsigned int) data[iter++])&0xff) << " ";
	}
	os << endl << dec;
	return os;
}
