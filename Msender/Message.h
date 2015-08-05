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
#ifndef MESSAGE_H_
#define MESSAGE_H_
#include <string>
using namespace std;

#pragma pack(push,1)  // save current value
#pragma pack(1)     // 8-bit packing

#define MESSAGE_SIZE_LIMIT 8192
#define MESSAGE_FIELDSIZE_DESTINATION	3
#define MESSAGE_FIELDSIZE_SENDER		3
#define MESSAGE_FIELDSIZE_TYPE			3
#define MESSAGE_FIELDSIZE_REFERENCE		9
#define MESSAGE_FIELDSIZE_DATALENGTH	4
#define MESSAGE_FIELDSIZE_MJD			6
#define MESSAGE_FIELDSIZE_MPM			9
#define MESSAGE_FIELDSIZE_IGNORED		1
#define MESSAGE_MINIMUM_SIZE			(MESSAGE_FIELDSIZE_DESTINATION + MESSAGE_FIELDSIZE_SENDER + MESSAGE_FIELDSIZE_TYPE + MESSAGE_FIELDSIZE_REFERENCE + MESSAGE_FIELDSIZE_DATALENGTH + MESSAGE_FIELDSIZE_MJD + MESSAGE_FIELDSIZE_MPM + MESSAGE_FIELDSIZE_IGNORED)
#define MESSAGE_FIELDSIZE_DATA			(MESSAGE_SIZE_LIMIT - MESSAGE_MINIMUM_SIZE)

typedef struct __MESSAGE_S {
		char _destination	[MESSAGE_FIELDSIZE_DESTINATION];
		char _sender		[MESSAGE_FIELDSIZE_SENDER];
		char _type			[MESSAGE_FIELDSIZE_TYPE];
		char _reference		[MESSAGE_FIELDSIZE_REFERENCE];
		char _dataLength	[MESSAGE_FIELDSIZE_DATALENGTH];
		char _MJD			[MESSAGE_FIELDSIZE_MJD];
		char _MPM			[MESSAGE_FIELDSIZE_MPM];
		char _ignored		[MESSAGE_FIELDSIZE_IGNORED];
		char _data			[MESSAGE_FIELDSIZE_DATA];
} MESSAGE_S;
typedef union __MESSAGE_U {
	MESSAGE_S 	fields;
	char 		rawData[MESSAGE_SIZE_LIMIT];
} MESSAGE_U;

class Message{
	protected:
		MESSAGE_U _data;
		void		setDataLength(size_t newValue);
	public:
		// static methods
		static size_t	convertPackedCharToNumber(char * pc, int maxNumChars);
		static void		convertNumberToPackedChar(size_t number, char * pc, int maxNumChars);
		static size_t	convertPackedCharToNumber(const char * pc, int maxNumChars);
		Message buildResponse(string mySourceDesignator, bool R_RESPONSE, string R_SUMMARY, string  R_COMMENT);
		//constructors (note: that the default copy constructor will work)
		Message(char * rawdata, size_t messageSize);
		Message(string destination,
				string sender,
				string type,
				size_t referenceNumber,
				size_t dataLength,
				size_t MJD,
				size_t MPM,
				char * data);


		//destructor
		virtual 	~Message();

		// getters
		std::string getDestination() const;
		std::string getSender() const;
		std::string getType() const;
		size_t 		getReferenceNumber() const;
		size_t 		getDataLength() const;
		size_t		getMJD() const;
		size_t		getMPM() const;
		char *		getData(size_t * datalen) const;
		char *		getMessageBlock(size_t * messageSize) const;
		string		toString();
		string 		getDataStr()const;
		// setters
		void		setDestination(string newValue);
		void		setSender(string newValue);
		void		setType(string newValue);
		void		setReferenceNumber(size_t newValue);
		void		setMJD(size_t newValue);
		void		setMPM(size_t newValue);
		void		setData(char* data, size_t datalen);

};
ostream& operator<< (ostream & os,  const Message &p);
#pragma pack(pop)     // restore previous value

#endif /*MESSAGE_H_*/
