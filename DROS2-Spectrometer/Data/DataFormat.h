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

#ifndef DATAFORMAT_H_
#define DATAFORMAT_H_

#include "../Common/Common.h"

#include "../Primitives/StdTypes.h"
#include "../Threading/LockHelper.h"

#include <boost/foreach.hpp>
#include <boost/assign.hpp>

using namespace boost::assign;

#define foreach BOOST_FOREACH

enum PacketType{PT_DRX, PT_TBN, PT_TBW, PT_TBF, PT_FSC, PT_SPC, PT_COR};

class DataFormat {
public:
	typedef map<string, DataFormat> FormatList;

	static const bool isFormatNameKnown(string name){
		SERIALIZE_ACCESS_ST();
		return (knownFormats.find(name) != knownFormats.end());
	}

	static const DataFormat getFormatByName(string name){
		SERIALIZE_ACCESS_ST();
		return knownFormats[name];
	}
	static const DataFormat getFormatByNumber(size_t index){
		SERIALIZE_ACCESS_ST();
		if (index>knownFormats.size()) return DataFormat();
		size_t i=0;
		foreach(FormatList::value_type res, knownFormats){
			if (i==index){
				return res.second;
			}
			i++;
		}
		return DataFormat();
	}
	static const string defaultFormatName;

	static size_t getKnownCount(){
		SERIALIZE_ACCESS_ST();
		return knownFormats.size();
	}
private:
	DECLARE_ACCESS_MUTEX_ST();
	static FormatList knownFormats;

public:
	DataFormat(string name, size_t frameSize, size_t dataRate, PacketType type, int decFactor):
		name(name),
		frameSize(frameSize),
		dataRate(dataRate),
		frameRate((double)dataRate/(double)frameSize),
		type(type),
		decFactor(decFactor){
	}
	DataFormat():
		name("Unknown Data Format"),
		frameSize(0),
		dataRate(0),
		frameRate(0),
		type(PT_DRX),
		decFactor(0){
	}

	DataFormat(const DataFormat& tc):
		name(tc.name),
		frameSize(tc.frameSize),
		dataRate(tc.dataRate),
		frameRate(tc.frameRate),
		type(tc.type),
		decFactor(tc.decFactor){
	}

	DataFormat& operator=(const DataFormat& tc){
		if (&tc==this) return *this;
		name      = tc.name;
		frameSize = tc.frameSize;
		dataRate  = tc.dataRate;
		frameRate = tc.frameRate;
		type      = tc.type;
		decFactor = tc.decFactor;
		 return *this;
	}


	virtual ~DataFormat(){
	}

	const      string& getName() const {return name;}
	size_t     getFrameSize()    const {return frameSize;}
	size_t     getDataRate()     const {return dataRate;}
	double     getFrameRate()    const {return frameRate;}
	PacketType getPacketType()   const {return type;}
	int        getDecFactor()    const {return decFactor;}
	int        getBitDepth()     const {return 4;}

	size_t estimateDataVolume(millisecond runtime, double percentOverallocate = 5.0){
		return (size_t)( (((double)runtime)*((double)dataRate)/1000.0)*(1.0+(percentOverallocate/100.0)));
	}

private:
	string name;
	size_t frameSize;
	size_t dataRate;
	double frameRate;
	PacketType type;
	int decFactor;

};

#endif /* DATAFORMAT_H_ */
