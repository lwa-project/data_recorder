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


#include "Utility.h"
#include <cstring>
#include <boost/lexical_cast.hpp>
#include <boost/regex.h>
#include <boost/regex.hpp>
#include "../System/Config.h"
#include "../Primitives/TypeConversion.h"
#include "../System/Log.h"

INIT_ACCESS_MUTEX_ST(Utility);

Utility::Utility() {}
Utility::~Utility() {}
const string Utility::re_meta   = "(?:\\{([a-zA-Z0-9_]*)=([^\\}]*)\\})";


bool Utility::reMatch(const string& re, const string& str){
	SERIALIZE_ACCESS_ST();
	TRACE(TR_UTILITY);
	boost::regex e(re);
	boost::smatch what;
	return boost::regex_match(str, what, e, boost::match_extra);
}
bool Utility::reMatchSub(const string& re, const string& str){
	SERIALIZE_ACCESS_ST();
	TRACE(TR_UTILITY);
	return reMatch((".*" + re + ".*"),str);
}

string Utility::__opt(const string& s){
	SERIALIZE_ACCESS_ST();
	return "(?:"+s+")?";
}
string Utility::__or(const string& a, const string& b){
	SERIALIZE_ACCESS_ST();
	return "(?:(?:" + a + ")|(?:" + b + "))";
}
string Utility::__rem(const int min, const int max){
	SERIALIZE_ACCESS_ST();
	int _min=min;
	int _max=max;
	if ((_min==0) && (_max==1)) return "?";
	if ((_min==1) && (_max==0)) return "+";
	if ((_min==0) && (_max==0)) return "*";
	if (_min<0) _min=0;
	if (_max<0) _max=0;
	if (_max<_min) _max=_min;
	if (_max == _min)
		return string("{") + LXS(_max) + string("}");
	else
		return string("{") + LXS(_min) + string(",") + LXS(_max) + string("}");
}

StringList Utility::getMatches(const string& re, const string& str, const bool removeEmpty){
	SERIALIZE_ACCESS_ST();
	TRACE(TR_UTILITY);
	boost::regex e(re);
	boost::smatch res;
	//cout << endl<< endl<< re<< endl<< endl<<flush;
	if(boost::regex_match(str, res, e, boost::match_extra)){
		StringList rv;
		for(unsigned i = 0; i < res.size(); ++i){
			if ((res.length(i)) || !removeEmpty){
				rv.push_back(res[i]);
			}
		}
		return rv;
	} else {
		return StringList();
	}
}
StringList Utility::getMatchesSub(const string& re, const string& str, const bool removeEmpty){
	SERIALIZE_ACCESS_ST();
	TRACE(TR_UTILITY);
	return getMatches((".*" + re + ".*"),str, removeEmpty);
}


/*
string Utility::getPosString(size_t offset, size_t length, const char* buf, size_t buflen){
	SERIALIZE_ACCESS_ST();
	TRACE(TR_UTILITY);
	string rv="";

	for (size_t pos=0; (((offset+pos) < buflen) && (pos < length)); pos++){
		if (buf[offset+pos] == '\0'){
			return rv;
		} else if (isprint(buf[offset+pos])){
			rv.append(&buf[offset+pos],1);
		} else {
			rv.push_back('?');
		}
	}
	return rv;
}



void Utility::setPosString(size_t offset, size_t length, char* buf, size_t buflen, string str){
	SERIALIZE_ACCESS_ST();
	TRACE(TR_UTILITY);
	try{
		size_t o = offset;
		size_t i = 0;
		while ((i<str.size()) && (i<length) && (o<buflen)){
			buf[o++]=str[i++];
		}
		while ((i<length) && (o<buflen)){
			buf[o++]='\0'; i++;
		}
	} catch (exception& e) {
		LOGC(L_ERROR, e.what(), OBJECT_ERROR_COLORS);
	}
}
*/

/*
string Utility::safeEncode(const char* chars, size_t size){
	SERIALIZE_ACCESS_ST();
	TRACE(TR_UTILITY);
	fprintf(stdout, "Utility::safeEncode(const char* chars, size_t size)\n");fflush(stdout);
	string rv = "";
	//cout << "*************************" << endl << flush;
	for (size_t i=0; i<size; i++){
		//fprintf(stdout,"@%lu\n",i); fflush(stdout);
		if (isprint(chars[i])){
			if (chars[i] == '\\') rv.push_back(chars[i]);
			rv.push_back(chars[i]);
		} else {
			unsigned p = (unsigned) ((const unsigned char*)chars)[i];
			string n = LXS(p);
			if (n.size()>3){
				fprintf(stdout, ">>>>>>>>>string has too many digits<<<<<<<<<<<<");fflush(stdout);
				rv += "?";
			}
			if (n.size() == 3){
				rv += "\\" +n;
			} else {
				string pad(3 - n.size(),'0');
				rv += "\\" + pad + n;
			}
		}
	}
	return rv;
}

size_t Utility::safeDecode(const string& safe, char* buf, size_t max_size){
	SERIALIZE_ACCESS_ST();
	TRACE(TR_UTILITY);
	size_t ss = safe.size();
	const char* sc = safe.c_str();
	size_t i = 0;
	size_t j = 0;
	//cout << "\n------------------------------------------------------------------\n\n";
	while (i<max_size && j<ss){
		if (safe[j] != '\\'){
			//cout << safe[j];
			buf[i++] = safe[j++];
		} else {
			if (j+1 >= ss){
				//cout << "\n\n\n<1>\n\n\n";
				return i;
			}
			if (safe[j+1] == '\\'){
				buf[i++] = '\\'; j+=2;
				//cout << '\\';
			} else {
				if ((j+3) >= ss){
					//cout << "\n\n\n<2>\n\n\n";
					return i;
				}
				if (
						isdigit(safe[j+1]) &&
						isdigit(safe[j+2]) &&
						isdigit(safe[j+3])
				){
					string n3  = Utility::getPosString(j+1,3,sc,ss);
					unsigned u = (unsigned) strtoul(n3.c_str(),NULL,10);
					buf[i++] = (char) u;
					j+=4;
					//cout << "[\\" << u << "]";
				} else {

					//cout << "\n\n\n<3>\n\n\n";
					return i;
				}
			}
		}
	}
	//cout << "\n------------------------------------------------------------------\n\n";
	return i;
}
*/
// extracting metadata from strings of the form {PARAM=VALUE}, surrounded with optional whitespace
StringStringMap Utility::extractMeta(string& stringWillBeMutated){
	SERIALIZE_ACCESS_ST();
	StringStringMap rv;
	string          c_copy = stringWillBeMutated;
	boost::regex e(Utility::re_meta);
	boost::smatch what;
	while (boost::regex_search(c_copy, what, e, boost::match_extra)){
		if (what.size() >= 3){
			const string matched = what[0];
			const string param   = what[1];
			const string value   = what[2];
			c_copy.erase(c_copy.find(matched), matched.size());
			rv[param] = value;
		}
	}
	stringWillBeMutated = c_copy;
	return rv;
}

string Utility::strip(string& s, const string& pattern){
	SERIALIZE_ACCESS_ST();
	StringStringMap rv;
	string          c_copy = s;
	boost::regex e(pattern);
	boost::smatch what;
	while (boost::regex_search(c_copy, what, e, boost::match_extra)){
		if (what.size() >= 1){
			const string matched = what[0];
			c_copy.erase(c_copy.find(matched), matched.size());
		}
	}
	return c_copy;
}


