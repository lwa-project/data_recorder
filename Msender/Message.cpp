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
#include "Message.h"
#include "TimeKeeper.h"
#include <cstring>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <iostream>
using namespace std;
size_t	Message::convertPackedCharToNumber(const char * pc, int maxNumChars){ // positive decimal values only
	return convertPackedCharToNumber((char *) pc, maxNumChars);
}
size_t	Message::convertPackedCharToNumber(char * pc, int maxNumChars){ // positive decimal values only
	int i=0;
	size_t rv=0;
	if (! pc) return rv;
	while ((pc[i]==' ') && (i<maxNumChars)){ i++; }
	while ((i<maxNumChars) && (pc[i]>='0') && (pc[i]<='9') ){
		rv=(rv*10) + (size_t)(pc[i]-'0');
		i++;
	}
	return rv;
}
void	Message::convertNumberToPackedChar(size_t number, char * pc, int maxNumChars){ // positive decimal values only
	char bfr[MESSAGE_SIZE_LIMIT]; // overkill, but jic
	int l_max= maxNumChars;
	int l_given=sprintf (bfr, "%lu", number);
	int paddingSpaces = (l_given <= l_max) ? l_max-l_given : 0;
	memset(	(void*) pc,' ', paddingSpaces);
	memcpy(	(void*) &pc[paddingSpaces],(void*)bfr,l_max - paddingSpaces);
}

Message::Message(char * rawdata, size_t messageSize){
	memcpy((void *)this->_data.rawData,(void*)rawdata,messageSize);
	size_t dataLength=getDataLength();
	if ((dataLength+MESSAGE_MINIMUM_SIZE)!= messageSize){
		cout << "Warning: malformed message: message length was "<< messageSize <<" but content says length should be "<< (dataLength+MESSAGE_MINIMUM_SIZE) << endl;
		//setDataLength(messageSize-MESSAGE_MINIMUM_SIZE);
	}
}
Message::Message(string destination,  string sender,  string type,  size_t referenceNumber,  size_t dataLength,  size_t MJD,  size_t MPM,  char * data){
	memset((void *)this->_data.rawData,0,MESSAGE_SIZE_LIMIT);
	setDestination(destination);
	setSender(sender);
	setType(type);
	setReferenceNumber(referenceNumber);
	setMJD(MJD);
	setMPM(MPM);
	setData(data, dataLength);
}


//destructor
Message::~Message(){
}

// getters
std::string Message::getDestination() const{
	string rv("");
	rv.insert(0,(const char *) this->_data.fields._destination,MESSAGE_FIELDSIZE_DESTINATION);
	return rv;
}
std::string Message::getSender() const{
	string rv("");
	rv.insert(0,(const char *) this->_data.fields._sender,MESSAGE_FIELDSIZE_SENDER);
	return rv;
}
std::string Message::getType() const{
	string rv("");
	rv.insert(0,(const char *) this->_data.fields._type,MESSAGE_FIELDSIZE_TYPE);
	return rv;
}
size_t 		Message::getReferenceNumber() const{
	return convertPackedCharToNumber(this->_data.fields._reference, MESSAGE_FIELDSIZE_REFERENCE);
}
size_t 		Message::getDataLength() const{
	return convertPackedCharToNumber(this->_data.fields._dataLength, MESSAGE_FIELDSIZE_DATALENGTH);
}
size_t		Message::getMJD() const{
	return convertPackedCharToNumber(this->_data.fields._MJD, MESSAGE_FIELDSIZE_MJD);
}
size_t		Message::getMPM() const{
	return convertPackedCharToNumber(this->_data.fields._MPM, MESSAGE_FIELDSIZE_MPM);
}
char *		Message::getData(size_t * datalen) const{
	*datalen = convertPackedCharToNumber(this->_data.fields._dataLength, MESSAGE_FIELDSIZE_DATALENGTH) ;
	return (char *) this->_data.fields._data;
}
char *		Message::getMessageBlock(size_t * messageSize) const{
		*messageSize = convertPackedCharToNumber(this->_data.fields._dataLength, MESSAGE_FIELDSIZE_REFERENCE) + MESSAGE_MINIMUM_SIZE;
		return (char *) &this->_data.rawData[0];
}

// setters
void	Message::setDestination(string newValue){
	char * where = this->_data.fields._destination;
	int l_max= MESSAGE_FIELDSIZE_DESTINATION;
	int l_given= newValue.length();
	int paddingSpaces = (l_given <= l_max) ? l_max-l_given : 0;
	memset(	(void*) where,' ', paddingSpaces);
	memcpy(	(void*) &where[paddingSpaces],(void*)newValue.c_str(),l_max - paddingSpaces);
}
void	Message::setSender(string newValue){
	char * 	where 			= this->_data.fields._sender;
	int 	l_max			= MESSAGE_FIELDSIZE_SENDER;
	int 	l_given			= newValue.length();
	int 	paddingSpaces 	= (l_given <= l_max) ? l_max-l_given : 0;
	memset(	(void*) where,' ', paddingSpaces);
	memcpy(	(void*) &where[paddingSpaces],(void*)newValue.c_str(),l_max - paddingSpaces);
}
void	Message::setType(string newValue){
	char * 	where 		= this->_data.fields._type;
	int 	l_max			= MESSAGE_FIELDSIZE_TYPE;
	int 	l_given			= newValue.length();
	int 	paddingSpaces 	= (l_given <= l_max) ? l_max-l_given : 0;
	memset(	(void*) where,' ', paddingSpaces);
	memcpy(	(void*) &where[paddingSpaces],(void*)newValue.c_str(),l_max - paddingSpaces);
}
void	Message::setReferenceNumber(size_t newValue){
	convertNumberToPackedChar( newValue, this->_data.fields._reference, MESSAGE_FIELDSIZE_REFERENCE);
}
void	Message::setMJD(size_t newValue){
	convertNumberToPackedChar( newValue, this->_data.fields._MJD, MESSAGE_FIELDSIZE_MJD);
}
void	Message::setMPM(size_t newValue){
	convertNumberToPackedChar( newValue, this->_data.fields._MPM, MESSAGE_FIELDSIZE_MPM);
}
void	Message::setDataLength(size_t newValue){
	convertNumberToPackedChar( newValue, this->_data.fields._dataLength, MESSAGE_FIELDSIZE_DATALENGTH);
}
string Message::getDataStr() const{
	string rv = "No Data Specified";
	if (getDataLength()){
		rv="";
		for (size_t i=0;i<getDataLength();i++)
			if (isprint(_data.fields._data[i]))
				rv.push_back (_data.fields._data[i]);
			else
				rv.push_back ('@');
	}
	return rv;
}

void	Message::setData(char * data, size_t datalen){
	if (datalen <=MESSAGE_FIELDSIZE_DATA){
		memcpy(	(void*) this->_data.fields._data,(void*)data,datalen);
		setDataLength(datalen);
	}
}
Message Message::buildResponse(string mySourceDesignator, bool R_RESPONSE, string R_SUMMARY, string  R_COMMENT){
	Message rv(*this);
	rv.setDestination(getSender());
	rv.setSender(mySourceDesignator);
	rv.setDataLength(1+7+R_COMMENT.size());

	int comsize=R_COMMENT.size();
	int sumsize=R_SUMMARY.size();

	// set mjd
	rv.setMJD(TimeKeeper::getMJD());

	// set mpm
	rv.setMPM(TimeKeeper::getMPM());

	// set response code
	if (R_RESPONSE) // true indicates aceptance
		rv._data.fields._data[0]='A';
	else
		rv._data.fields._data[0]='R';

	// set response summary
	char * where = &rv._data.fields._data[1];
	if (sumsize<7){
		strncpy(where, R_SUMMARY.c_str(), sumsize);
		for (int i=sumsize;i<7;i++)
			where[i]=' ';
	} else {
		strncpy(where, R_SUMMARY.c_str(), 7);
	}

	// set response comment // note this pads with nulls
	where = &rv._data.fields._data[8];
	strncpy(where, R_SUMMARY.c_str(), MESSAGE_FIELDSIZE_DATA-8);

	// pad with spaces
	where = &rv._data.fields._data[8+comsize];
	memset ((void * ) where, (int)' ', MESSAGE_FIELDSIZE_DATA-(8+comsize));

	return rv;
}
string Message::toString(){
	stringstream ss;
	ss << "from \""<< getSender()<< "\" to \"" << getDestination() << "\" type=" << getType() << " ref#="<<  getReferenceNumber() <<".";
	return ss.str();
}

ostream& operator<< (ostream & os,  const Message &p){
	os 	<< "[send]Message :" << endl
		<< "[send]\tSender:                     " << p.getSender() << endl
		<< "[send]\tDestination:                " << p.getDestination() << endl
		<< "[send]\tType:                       " << p.getType() << endl
		<< "[send]\tReference:                  " << p.getReferenceNumber() << endl
		<< "[send]\tModified Julian Day:        " << p.getMJD() << endl
		<< "[send]\tMilliseconds Past Midnight: " << p.getMPM() << endl
		<< "[send]\tData Length:                " << p.getDataLength() << endl
		<< "[send]\tData:                       " << p.getDataStr() << endl;
	size_t dlen;
	char*  data=p.getData(&dlen);
	size_t iter=0;
	os << hex;
	while ((dlen-iter) >= 16){
		os << "[send]\t\t<" << setw(4)<< iter << ">:   ";
		for ( size_t i=0; i<16; i++){
			os << setw(2) << (((unsigned int) data[iter+i])&0xff) << " ";
		}
		iter+=16;
		os << endl;
	}
	if (dlen-iter) os <<  "[send]\t\t<" << setw(4)<< iter << ">:   ";
	while (iter<dlen){
		os << setw(2) << (((unsigned int) data[iter++])&0xff) << " ";
	}
	os << endl << dec;
	return os;
}
