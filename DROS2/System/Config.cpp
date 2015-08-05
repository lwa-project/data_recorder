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

#include "Config.h"

SINGLETON_CLASS_SOURCE(Config);
SINGLETON_CLASS_CONSTRUCTOR(Config){
	readConfig();
}
SINGLETON_CLASS_DESTRUCTOR(Config){
	writeConfig();
}

#define DO_LOAD(var, type, mandatory) if (!vm.count(#var)) {if(mandatory){cout << "Error: missing configuration option '"<< #var <<"'\n"; exit(-1);}else {cout << "Warning: optional configuration option '"<< #var <<"' not specified\n";}} else {var=vm[#var].as<type>();}
#define DO_SAVE(s, var, comment) s << "#" comment << endl << #var " = " << var << endl << endl
#define ADD_SPEC_DEF(v,va, type, def, comment) (#v "," va, value<type>()->default_value(def), comment)


void  Config::readConfig(){
	SERIALIZE_ACCESS();
	options_description o_general("General Options");
	o_general.add_options()
		ADD_SPEC_DEF(MyReferenceDesignator, "r", string,         "DR1",          "System reference designator")
		ADD_SPEC_DEF(SelfIp,                "I", string,         "10.1.1.21",    "System IP address")
		ADD_SPEC_DEF(TimeAuthority,         "T", string,         "10.1.1.51",    "NTP server")
		ADD_SPEC_DEF(SerialNumber,          "S", string,         "Not Assigned", "System Serial Number")
		ADD_SPEC_DEF(ArraySelect,           "A", string,         "0",            "Startup DRSU Selection (Barcode or 0-based index)")
		ADD_SPEC_DEF(MessageInPort,         "i", unsigned short, 5000,           "UDP Port to listen on for MCS messages")
		ADD_SPEC_DEF(MessageOutPort,        "o", unsigned short, 5001,           "UDP Port to send responses to")
		ADD_SPEC_DEF(DataInPort,            "d", unsigned short, 3000,           "UDP Port to receive data on")
	;

	variables_map vm;
	ifstream ifs(DEFAULT_CONFIG_FILE);
	if (!ifs){
		cout << "Error: Can not open config file: " << DEFAULT_CONFIG_FILE << "\n";
		exit(-1);
	} else {
		store(parse_config_file(ifs, o_general,true), vm);
		notify(vm);
		ifs.close();
	}
	DO_LOAD(MyReferenceDesignator,string,true);
	DO_LOAD(SelfIp,string,true);
	DO_LOAD(TimeAuthority,string,true);
	DO_LOAD(SerialNumber,string,true);
	DO_LOAD(ArraySelect,string,true);
	DO_LOAD(MessageInPort,unsigned short,true);
	DO_LOAD(MessageOutPort,unsigned short,true);
	DO_LOAD(DataInPort,unsigned short,true);
	LOGC(L_INFO,"Configuration data loaded...", CONFIG_COLORS);
	LOG_START_SESSION(L_DEBUG);


	SHOW_CONFIG_VAL(MyReferenceDesignator);
	SHOW_CONFIG_VAL(SelfIp);
	SHOW_CONFIG_VAL(MessageInPort);
	SHOW_CONFIG_VAL(MessageOutPort);
	SHOW_CONFIG_VAL(TimeAuthority);
	SHOW_CONFIG_VAL(SerialNumber);
	SHOW_CONFIG_VAL(ArraySelect);
	SHOW_CONFIG_VAL(DataInPort);

	LOG_END_SESSION();



}

void  Config::writeConfig(){
	SERIALIZE_ACCESS();
	stringstream sout;
	ofstream fout(DEFAULT_CONFIG_FILE,ios_base::trunc | ios_base::out);
	if (!fout){
		cout << "Error: Can not open config file for writing: " << DEFAULT_CONFIG_FILE << "\n";
		exit(-1);
	}
	DO_SAVE(fout, MyReferenceDesignator, "System reference designator");
	DO_SAVE(fout, SelfIp,                "System IP address");
	DO_SAVE(fout, TimeAuthority,         "NTP server");
	DO_SAVE(fout, SerialNumber,          "System Serial Number");
	DO_SAVE(fout, ArraySelect,           "Startup DRSU Selection (Barcode or 0-based index)");
	DO_SAVE(fout, MessageInPort,         "UDP Port to listen on for MCS messages");
	DO_SAVE(fout, MessageOutPort,        "UDP Port to send responses to");
	DO_SAVE(fout, DataInPort,            "UDP Port to receive data on");
	fout.flush();
	fout.close();
	LOGC(L_INFO,"Configuration data saved...",CONFIG_COLORS);
}

