// ========================= DROSv2 License preamble===========================
// Software for DROSv2 is covered jointly under GPLv3 and Boost 1.0 licenses,
// to the extent required by included Boost sources and GPL sources, and to the
// more restrictive case pertaining thereunto, as defined herebelow. Beyond those
// requirements, any code not explicitly restricted by either of thowse two license
// models shall be deemed to be licensed under the GPLv3 license and subject to
// those restrictions.
//
// Copyright 2012, Virginia Tech, University of New Mexico, and Christopher Wolfe
//
// ========================= Boost License ====================================
// Boost Software License - Version 1.0 - August 17th, 2003
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
//
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
// =============================== GPL V3 ====================================
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.


#ifndef MESSAGE_H_
#define MESSAGE_H_
#include <boost/lexical_cast.hpp>
//#include <boost/interprocess/sync/scoped_lock.hpp>
#include "../Primitives/StdTypes.h"

#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>

#include <arpa/inet.h>
#include "../System/Time.h"
#include "../System/Log.h"
#include "../Common/Utility.h"
#include "../Common/CommonICD.h"
#include "../Threading/LockHelper.h"
#include "IpSpec.h"



class Message {
public:

	Message(){
		//LOGC(L_ERROR, "NEW", ACTOR_ERROR_COLORS);
		clear(true);
	}

	Message(
		const IpSpec& _sourceIpSpec, const IpSpec& _responseIpSpec,
		const string& _Destination,  const string& _Sender,         const string& _Type,
		const size_t& _Reference,    const size_t& _MJD,    		const size_t& _MPM,
		const string& _Data, const StringStringMap& _meta
	){
		//LOGC(L_ERROR, "BUILD", ACTOR_ERROR_COLORS);
		set( _sourceIpSpec, _responseIpSpec, _Destination, _Sender, _Type,
			 _Reference,    _MJD,    		 _MPM,         _Data,   _meta,
			 true);
	}

	Message( const Message& m){
		//LOGC(L_ERROR, "CLONE", ACTOR_ERROR_COLORS);
		cloneFrom(m);
	}

	virtual ~Message(){
		//LOGC(L_ERROR, "DESTROY", ACTOR_ERROR_COLORS);
		Utility::release<char>(Binary);
	}

	Message& operator=(const Message& m){
		//LOGC(L_ERROR, "ASSIGN", ACTOR_ERROR_COLORS);
		SERIALIZE_ACCESS();
		if (&m != this)
			cloneFrom(m);
		return *this;
	}

	void cloneFrom(const Message& m){
		set(
			m.sourceIpSpec,
			m.responseIpSpec,
			m.Destination,
			m.Sender,
			m.Type,
			m.Reference,
			m.MJD,
			m.MPM,
			m.Data,
			m.meta,
			true
		);
		responseSet   = m.responseSet;
		commentBinary = m.commentBinary;
		BinarySize    = m.BinarySize;
		Accept        = m.Accept;
		Status        = m.Status;
		Comment       = m.Comment;
		Binary        = m.Binary;
		if (m.responseSet && m.commentBinary && m.Binary && m.BinarySize){
			Binary = Utility::clone<char>(m.Binary,m.BinarySize);
		}
		meta = m.meta;
	}


	void clear(bool initial=false){
		SERIALIZE_ACCESS();
		if (!initial && (Binary!=NULL)){
			Utility::release<char>(Binary);
		}
		sourceIpSpec    = "127.0.0.1:0";
		responseIpSpec  = "127.0.0.1:0";
		Destination     = "XXX";
		Sender          = "XXX";
		Type            = "XXX";
		Reference       = 0;
		MJD             = Time::getMJD();
		MPM             = Time::getMPM();
		Data            = "";
		Accept          = false;
		Status          = "Unknown";
		Comment         = "";
		commentBinary   = false;
		Binary          = NULL;
		BinarySize      = 0;
		responseSet     = false;
		meta.clear();
	}

	void set(
		const IpSpec& _sourceIpSpec, const IpSpec& _responseIpSpec,
		const string& _Destination,  const string& _Sender,         const string& _Type,
		const size_t& _Reference,    const size_t& _MJD,    		const size_t& _MPM,
		const string& _Data, const StringStringMap& _meta,
		bool initial=false
	){
		SERIALIZE_ACCESS();
		clear(initial);
		sourceIpSpec    = _sourceIpSpec;
		responseIpSpec  = _responseIpSpec;
		Destination     = _Destination;
		Sender          = _Sender;
		Type            = _Type;
		Reference       = _Reference;
		MJD             = _MJD;
		MPM             = _MPM;
		Data            = _Data;
		meta            = _meta;
	}

	// const accessors
	IpSpec&      getSourceIpSpec()const{   SERIALIZE_ACCESS(); return sourceIpSpec;}
	IpSpec&      getResponseIpSpec()const{ SERIALIZE_ACCESS(); return responseIpSpec;}
	string       getDestination()const{    SERIALIZE_ACCESS(); return Destination;}
	string       getSender()const{         SERIALIZE_ACCESS(); return Sender;}
	string       getType()const{           SERIALIZE_ACCESS(); return Type;}
	size_t       getReference()const{      SERIALIZE_ACCESS(); return Reference;}
	size_t       getDataLength(bool _auto = false)const{
		SERIALIZE_ACCESS();
		if (!responseSet || !_auto)
			return Data.size();
		else {
			return (((commentBinary) ? BinarySize : Comment.size()) + (RFS_ACCEPT + RFS_STATUS));
		}
	}
	size_t 		 getMJD()const{            SERIALIZE_ACCESS(); return MJD;}
	size_t 		 getMPM()const{            SERIALIZE_ACCESS(); return MPM;}
	string       getData()const{           SERIALIZE_ACCESS(); return Data;}
	bool         getAccept()const{         SERIALIZE_ACCESS(); return Accept;}
	string       getStatus()const{         SERIALIZE_ACCESS(); return Status;}
	bool         isResponseSet()const{     SERIALIZE_ACCESS(); return responseSet;}
	bool         isCommentBinary()const{   SERIALIZE_ACCESS(); return commentBinary;}
	bool         getCommentString(string& ret_comment)const{
		SERIALIZE_ACCESS();
		if (responseSet){
			if (commentBinary){
				return false;
			} else {
				ret_comment = Comment;
				return true;
			}
		} else {
			return false;
		}
	}

	bool         getCommentBinary(char*& ret_b_comment, size_t& ret_size)const{
		SERIALIZE_ACCESS();
		if (responseSet){
			if (!commentBinary){
				return false;
			} else {
				ret_b_comment = Binary;
				ret_size      = BinarySize;
				return true;
			}
		} else {
			return false;
		}
	}
	bool         isSendable()const{ //sanity check
		SERIALIZE_ACCESS();
		if (!responseSet)                                           return false;
		if (commentBinary && (Binary == NULL) && (BinarySize != 0)) return false;
		//if (!Utility::isIP(responseIpSpec))                         return false;
		//if (!Utility::hasPort(responseIpSpec))                      return false;
		return true;
	}

	StringStringMap getMeta()const{
		return meta;
	}
	string getMetaValue(const string& key){
		foreach(StringStringMap::value_type pair, meta){
			if (!pair.first.compare(key))
				return pair.second;
		}
		return "";
	}

	// mutators
	void         setResponse(const bool accept, const string status, const string comment){
		SERIALIZE_ACCESS();
		if (responseSet){
			if (commentBinary){
				Utility::release<char>(this->Binary);
				BinarySize = 0;
			} else {
				Comment = "";
			}
		}
		MJD             = Time::getMJD();
		MPM             = Time::getMPM();
		Accept          = accept;
		Status          = status;
		Comment         = comment;
		responseSet   = true;
		commentBinary = false;
	}

	void         setResponse(const bool accept, const string status, const char* binary, const size_t  binarySize){
		SERIALIZE_ACCESS();
		if (responseSet){
			if (commentBinary){
				Utility::release<char>(this->Binary);
				BinarySize      = 0;
			} else {
				Comment = "";
			}
		}
		MJD             = Time::getMJD();
		MPM             = Time::getMPM();
		Accept          = accept;
		Status          = status;
		Binary          = Utility::clone(binary,binarySize);
		BinarySize      = binarySize;
		responseSet   = true;
		commentBinary = true;
	}


	virtual string toString(bool showIpInfo=true, bool showData=true) const{
		SERIALIZE_ACCESS();
		stringstream ss;

		if (!responseSet){
			// Treat as Message
			ss << "Message("        <<
					MJD             << ":"  <<
					MPM             << ", " <<
					Sender          << ", " <<
					Destination     << ", " <<
					Reference       << ", " <<
					Type            << ", " <<
					getDataLength() << ", ";
			if (showData){
				ss << "'" << Data << "'";
			} else {
				ss << "__DATA_OMITTED__";
			}
			ss << " ) ";
			if (showIpInfo)
				ss << sourceIpSpec;
		} else {
			// Treat as Response
			ss << "Response("           <<
					MJD                 << ":"  <<
					MPM                 << ", " <<
					Destination         << ", " << // note the swap in display, but not inner content
					Sender              << ", " << // note the swap in display, but not inner content
					Reference           << ", " <<
					Type                << ", " <<
					getDataLength(true) << ", " <<
					((Accept)?"Accept":"Reject") << ", " <<
					Status << ", ";
			if (showData){
				if (commentBinary)
					ss << "'" << Data << "', " << "__BINARY_DATA__";
				else
					ss << "'" << Data << "', " << "'" << Comment << "'";
			} else {
				ss << "__DATA_OMITTED__, __DATA_OMITTED__";
			}
			ss << " ) ";
			if (showIpInfo)
				ss << responseIpSpec;
		}
		return ss.str();
	}

	void pack(char * buf){
		SERIALIZE_ACCESS();
		PackedMessage* pm = (PackedMessage*) buf;
		if (commentBinary)	CommonICD::_pack2(pm, Sender, Destination, Type, Reference, MJD, MPM, Accept, Status, Binary, BinarySize);
		else  			    CommonICD::_pack( pm, Sender, Destination, Type, Reference, MJD, MPM, Accept, Status, Comment);
	}

private:
	// ICD fields for messages
	string  Destination;
	string  Sender;
	string  Type;
	size_t  Reference;
	size_t 	MJD;
	size_t 	MPM;
	string  Data;
	// ICD fields for responses (in addition to message fields)
	bool    Accept;
	string  Status;
	string  Comment;

	// what type of response are we sending?
	bool    commentBinary;
	mutable char* Binary;
	size_t  BinarySize;

	bool    responseSet;


	// where did it come from, where is the response going
	mutable IpSpec  sourceIpSpec;   // the source IP/port as received
	mutable IpSpec  responseIpSpec; // the IP/port to respond to

	// additional metadata
	StringStringMap meta;

	DECLARE_ACCESS_MUTEX();
};

ostream& operator << (ostream& out, const Message& m);

#endif /* MESSAGE_H_ */
