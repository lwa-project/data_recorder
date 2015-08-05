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
#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <pthread.h>
#include "Message.h"
#include "Response.h"
#include "TimeKeeper.h"
#include "Socket.h"
#include <list>
#include <cstring>
#include <cstdio>

using namespace std;



void usage(string errorMsg){

	cout << "[Error]  " << errorMsg << endl;
	cout <<
	"Usage: " << endl <<
	"\tMsender " << endl <<
	"\t\t-Source \"XXX\" " << endl <<
	"\t\t-Destination \"YYY\" " << endl <<
	"\t\t-Type \"ZZZ\" " << endl <<
	"\t\t-ReferenceNumber 123456789 " << endl <<
	"\t\t-DestinationIpAddress aaa.bbb.ccc.ddd " << endl <<
	"\t\t-DestinationPort 12345 " << endl <<
	"\t\t-ResponseListenPort 12345 " << endl <<
	"\t\t[-Data \"string with no embedded nulls\"]" << endl << endl;
	exit(EXIT_FAILURE);
}
void parseResponse(Response* resp, string responseFormat){

}
typedef int OptType;
#define NONE     0
#define STRING   1
#define CHARPTR  2
#define FLAG     3
#define SHORT    4
#define USHORT   5
#define LONG     6
#define ULONG    7

typedef struct __Opts{
	string	namefull;
	string  nameshort;
	void *  varptr;
	OptType type;
	int     minlen;
	int     maxlen;
	bool*   specifiedFlag;
} Opts;

bool matchEither(string x, string n1, string n2){
	string m1 = "-" + n1;
	string m2 = "-" + n2;
	return  (!m1.compare(x))||(!m2.compare(x));
}



int main(int argc, char * argv[]){
	size_t sendsize=0;
	size_t actuallySent=0;
	char * ptr=NULL;

	string msgSource;
	string msgDestination;
	string msgType;
	size_t msgReferenceNumber=0;
	size_t msgDataLength=0;
	size_t msgMJD;
	size_t msgMPM;
	ssize_t timeout;
	char * msgData=NULL;

	unsigned short ResponseListenPort=0;
	unsigned short DestinationPort=0;
	string DestinationIpAddress;
	bool msgSourceSpecified=false;
	bool msgDestinationSpecified=false;
	bool msgTypeSpecified=false;
	bool msgReferenceNumberSpecified=false;
	bool DestinationIpAddressSpecified=false;
	bool DestinationPortSpecified=false;
	bool ResponseListenPortSpecified=false;
	bool msgDataSpecified=false; // the only one that does not have to be specified on the commandline
	bool timeoutSpecified=false;
	bool verbose=false;



	string expectedMPM;
	string expectedMJD;
	string expectedSender;
	string expectedDestination;
	string expectedType;
	string expectedComment;
	string expectedStatus;
	string expectedAccepted;
	string expectedReference;

	bool expectedMPMSpecified=false;
	bool expectedMJDSpecified=false;
	bool expectedSenderSpecified=false;
	bool expectedDestinationSpecified=false;
	bool expectedTypeSpecified=false;
	bool expectedCommentSpecified=false;
	bool expectedStatusSpecified=false;
	bool expectedAcceptedSpecified=false;
	bool expectedReferenceSpecified=false;

	bool noListen=false;



	int i=1;
	int p=argc-1;
/*
	string a="Data";
	string b="Destination";
	string c="DestinationIpAddress";
	string d="D";
	cout << !a.compare(a) << " " << !a.compare(b) << " " << !a.compare(c) << " " << !a.compare(d) << endl;
	cout << !b.compare(a) << " " << !b.compare(b) << " " << !b.compare(c) << " " << !b.compare(d) << endl;
	cout << !c.compare(a) << " " << !c.compare(b) << " " << !c.compare(c) << " " << !c.compare(d) << endl;
	cout << !d.compare(a) << " " << !d.compare(b) << " " << !d.compare(c) << " " << !d.compare(d) << endl;
*/



	Opts opts[]={
			{"Source",                "s",  &msgSource,                STRING,   3,     3, &msgSourceSpecified},
			{"Destination",           "d",  &msgDestination,           STRING,   3,     3, &msgDestinationSpecified},
			{"Type",                  "t",  &msgType,                  STRING,   3,     3, &msgTypeSpecified},
			{"ReferenceNumber",       "r",  &msgReferenceNumber,       ULONG,    1,     9, &msgReferenceNumberSpecified},
			{"DestinationIpAddress",  "I",  &DestinationIpAddress,     STRING,  -1,    15, &DestinationIpAddressSpecified},
			{"DestinationPort",       "po", &DestinationPort,          USHORT,   1,     9, &DestinationPortSpecified},
			{"ResponseListenPort",    "pi", &ResponseListenPort,       USHORT,   1,     9, &ResponseListenPortSpecified},
			{"Data",                  "D",  &msgData,                  CHARPTR, -1,  8154, &msgDataSpecified},
			{"Verbose",               "v",  &verbose,                  FLAG,    -1,    -1, (bool*) NULL},
			{"NoListen",              "nl", &noListen,                 FLAG,    -1,    -1, (bool*) NULL},
			{"Timeout",               "to", &timeout,                  LONG,     1,     9, &timeoutSpecified},

			{"expectedMPM",		      "eM", &expectedMPM,              STRING,   1,     9, &expectedMPMSpecified},
			{"expectedMJD",		      "eD", &expectedMJD,              STRING,   5,     6, &expectedMJDSpecified},
			{"expectedSender",		  "es", &expectedSender,           STRING,   3,     3, &expectedSenderSpecified},
			{"expectedDestination",   "ed", &expectedDestination,      STRING,   3,     3, &expectedDestinationSpecified},
			{"expectedType",		  "et", &expectedType,             STRING,   3,     3, &expectedTypeSpecified},
			{"expectedComment",	  	  "eC", &expectedComment,          STRING,  -1,     3, &expectedCommentSpecified},
			{"expectedStatus",		  "eS", &expectedStatus,           STRING,   1,     7, &expectedStatusSpecified},
			{"expectedAccepted",	  "eA", &expectedAccepted,         STRING,   1,     1, &expectedAcceptedSpecified},
			{"expectedReference",	  "er", &expectedReference,        STRING,   1,     9, &expectedReferenceSpecified},
			{"", "", NULL, NONE, -1, -1, (bool*) NULL}
	};

	while (i<argc){
		int j=0;
		bool matched=false;
		while ((opts[j].type != NONE) && !matched){
			//cout << "Matching '" << argv[i] << "' against '-" << opts[j].namefull << "' or '-" << opts[j].nameshort << "'" << endl;
			if (matchEither(argv[i], opts[j].namefull.c_str(), opts[j].nameshort.c_str())){
				// check parameter value is available
				if (opts[j].type != FLAG){
					if (i >= (argc-1))	usage("Missing operand: " + opts[j].namefull);
				}
				// check length
				if (opts[j].type != FLAG){
					int slen=strlen(argv[i+1]);
					if ((opts[j].minlen != -1) && (slen < opts[j].minlen))
							usage("Illegal value supplied for parameter '" + opts[j].namefull + "' : too short.");
					if ((opts[j].maxlen != -1) && (slen > opts[j].maxlen))
						usage("Illegal value supplied for parameter '" + opts[j].namefull + "' : too long.");
				}
				// update 'specified' flag for parameter
				if (opts[j].specifiedFlag != ((bool*)NULL) ){
					*opts[j].specifiedFlag = true;
				}
				// assign parameter value
				switch (opts[j].type){
					case STRING:	*((string *)        opts[j].varptr)=argv[i+1];                          break;
					case CHARPTR:	*((char**)          opts[j].varptr)=argv[i+1];                          break;
					case FLAG:		*((bool *)          opts[j].varptr)=true;                               break;
					case SHORT:     *((short*)          opts[j].varptr) = (short)          atoi(argv[i+1]); break;
					case USHORT:    *((unsigned short*) opts[j].varptr) = (unsigned short) atoi(argv[i+1]); break;
					case LONG:		*((ssize_t*)        opts[j].varptr) = strtol(argv[i+1],NULL,10);        break;
					case ULONG:		*((size_t*)         opts[j].varptr) = strtoul(argv[i+1],NULL,10);       break;
					default:	// fall through
					case NONE:
						cout << "Error: reached unreachable code at File :'"<<__FILE__<<"' Line: " << __LINE__ << endl;
						exit(-1);
						break;
				}
				matched=true;
				i = i + ((opts[j].type == FLAG) ? 1 : 2);
			}
			j++;
		}
		if (!matched){
			usage(string("Unknown commandline option:")+argv[i]);
		}
	}




	if (
		(! msgSourceSpecified) 				||
		(! msgDestinationSpecified) 		||
		(! msgTypeSpecified) 				||
		(! msgReferenceNumberSpecified)		||
		(! DestinationIpAddressSpecified) 	||
		(! DestinationPortSpecified) 		||
		(! ResponseListenPortSpecified) ){
		usage("You must specify all options except data, timeout, and the verbosity flag");
	}


	Message* msg;
	msgMJD=TimeKeeper::getMJD();
	msgMPM=TimeKeeper::getMPM();
	if (!msgDataSpecified)
		msg = new Message(msgDestination, msgSource, msgType, msgReferenceNumber, 0, msgMJD, msgMPM, (char *) "");
	else{
		string mdata=msgData;
		size_t pos;
		size_t pos2;
		char MPM[8192];
		char MJD[8192];
		//cout << "STRING='"<<  mdata << "'" << endl;
		if ((pos = mdata.find("MPM"))!=string::npos){
			pos2 = mdata.find(" ",pos+1);
			string p = mdata.substr(pos+3,pos2-pos-3);
			int mpmDelta=atoi(p.c_str());
			if (mpmDelta >= 0){
				sprintf(MPM,"%09lu",msgMPM+(size_t)mpmDelta);
			} else {
				sprintf(MPM,"%09lu",msgMPM-(size_t)(mpmDelta*-1));
			}
			//cout << "Replacing 'MPM" << p << "' with '" << MPM << "'" << endl;
			mdata=mdata.replace(pos,pos2-pos,MPM);
		}
		//cout << "STRING='"<<  mdata << "'" << endl;
		if ((pos = mdata.find("MJD"))!=string::npos){
			pos2 = mdata.find(" ",pos+1);
			string p = mdata.substr(pos+3,pos2-pos-3);

			int mjdDelta=atoi(p.c_str());
			if (mjdDelta >= 0){
				sprintf(MJD,"%06lu",msgMJD+(size_t)mjdDelta);
			} else {
				sprintf(MJD,"%06lu",msgMJD-(size_t)(mjdDelta*-1));
			}
			//cout << "Replacing 'MJD" << p << "' with '" << MJD << "'" << endl;
			mdata=mdata.replace(pos,pos2-pos,MJD);
		}
		//cout << "STRING='"<<  mdata << "'" << endl;
		msgDataLength = mdata.length();
		msg = new Message(msgDestination, msgSource, msgType, msgReferenceNumber, msgDataLength, msgMJD, msgMPM, (char *)mdata.c_str());
	}
	if (msg==NULL) {
		cout << "allocation fault." << endl;
		exit (EXIT_FAILURE);
	}
	if (verbose) cout << *msg;
	Socket mySocket;
	mySocket.openInbound(ResponseListenPort);
	if (mySocket.inboundConnectionIsOpen() ){
		//cout << "Listen socket opened Successfully." << endl;
	} else {
		cout << "Could not open listen socket on port "<< ResponseListenPort << endl;
		exit (EXIT_FAILURE);
	}
	mySocket.openOutbound(DestinationIpAddress.c_str(), DestinationPort);
	if (mySocket.outboundConnectionIsOpen() ){
		//cout << "Send socket opened Successfully." << endl;
	} else {
		cout << "Could not open send socket to "<< DestinationIpAddress << " on port " << DestinationPort << endl;
		exit (EXIT_FAILURE);
	}
	ptr=msg->getMessageBlock(&sendsize);
	actuallySent=mySocket.send(ptr,sendsize);
	//cout  << endl << endl<< "Sent " <<  actuallySent << " bytes to " << DestinationIpAddress << " on port " << DestinationPort << endl << endl;
	// now receive response
	if(noListen) return 0;
	Response* response=NULL;
	char 		messageBuffer[MESSAGE_SIZE_LIMIT];
	size_t 		bytesReceived=0;
	bool error=false;
	TimeKeeper::resetRuntime();
	while ( (mySocket.inboundConnectionIsOpen() && mySocket.outboundConnectionIsOpen()) && bytesReceived == 0 ){
		if (timeoutSpecified && ( ((ssize_t)TimeKeeper::getRuntime()) >= timeout)){
			cout << "No Response: Timeout Exceeded.\n";
			exit (EXIT_FAILURE);
		}


		bytesReceived = mySocket.receive(messageBuffer, MESSAGE_SIZE_LIMIT);
		switch (bytesReceived){
			case 0:
				;//cout << "Zero-byte response, still waiting." << endl;
			break;
			default:
				if (bytesReceived>=MESSAGE_MINIMUM_SIZE){
					response=new Response(messageBuffer,bytesReceived);
					//cout << "received " <<  bytesReceived << " bytes" << endl;
					if (verbose)
						cout << *response;
					else
						cout << (*response).getComment() << endl;
					if ((expectedMPMSpecified) && (response->getMPM() != strtoul(expectedMPM.c_str(),NULL,10))) 						{error=true; printf ("[Error]  mismatched MPM              %lu != %lu\n",   response->getMPM() , strtoul(expectedMPM.c_str(),NULL,10));}
					if ((expectedMJDSpecified) && (response->getMJD() != strtoul(expectedMJD.c_str(),NULL,10))) 						{error=true; printf ("[Error]  mismatched MJD              %lu != %lu\n",   response->getMJD(), strtoul(expectedMJD.c_str(),NULL,10));}
					if ((expectedReferenceSpecified) && (response->getReferenceNumber() != strtoul(expectedReference.c_str(),NULL,10))) {error=true; printf ("[Error]  mismatched reference number %lu != %lu\n",   response->getReferenceNumber() , strtoul(expectedReference.c_str(),NULL,10));}
					if ((expectedSenderSpecified) && (response->getSender() != expectedSender)) 										{error=true; printf ("[Error]  mismatched Sender           '%s' != '%s'\n", response->getSender().c_str(), expectedSender.c_str());}
					if ((expectedDestinationSpecified) && (response->getDestination() != expectedDestination)) 							{error=true; printf ("[Error]  mismatched Destination      '%s' != '%s'\n", response->getDestination().c_str() , expectedDestination.c_str());}
					if ((expectedTypeSpecified) && (response->getType() != expectedType)) 												{error=true; printf ("[Error]  mismatched Type             '%s' != '%s'\n", response->getType().c_str(), expectedType.c_str());}
					if ((expectedStatusSpecified) && (response->getGeneralStatus() != expectedStatus)) 									{error=true; printf ("[Error]  mismatched Status           '%s' != '%s'\n", response->getGeneralStatus().c_str(), expectedStatus.c_str());}
					if ((expectedAcceptedSpecified) && (response->getAcceptReject() != expectedAccepted)) 								{error=true; printf ("[Error]  mismatched Accepted         '%s' != '%s'\n", response->getAcceptReject().c_str(), expectedAccepted.c_str());}
					if (expectedCommentSpecified){
						string com=response->getComment();
						char bufff[8192];
						strncpy(bufff, expectedComment.c_str(),8192);
						char* token = strtok(bufff," ");
						while (token){
							if (com.find(token)==string::npos){
								error=true;
								printf("[Error]  mismatched response data: couldn't find '%s' in '%s'\n",token,com.c_str());
							}
							token =strtok(NULL," ");
						}
					}
					if (expectedMPMSpecified || expectedMJDSpecified || expectedReferenceSpecified || expectedSenderSpecified || expectedDestinationSpecified || expectedTypeSpecified || expectedStatusSpecified || expectedAcceptedSpecified || expectedCommentSpecified){
						if (!error) printf("[PASS]\n"); else printf("[FAIL]\n");
					}
					delete (response);
				} else {
					cout << "[Error]  response was "<< bytesReceived << " bytes in length but the minimum is 38" << endl;
					exit (EXIT_FAILURE);
				}
			break;
		}
	}
}
