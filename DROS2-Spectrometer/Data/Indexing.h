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


#ifndef INDEXING_H_
#define INDEXING_H_

typedef long unsigned   _u;
typedef const _u        _uc;

template <class T* t, _uc D0,_uc D1,_uc D2,_uc D3,_uc D4,_uc D5>
class Indexer_6d{
	Indexer_6d(){};
	~Indexer_6d(){};
	T* operator()(_uc d0,_uc d1,_uc d2,_uc d3,_uc d4,_uc d5){
		return &t[(((((((((d0*D1)+d1)*D2)+d2)*D3)+d3)*D4)+d4)*D5)+d5];
	}
	_uc numel()const{return D0*D1*D2*D3*D4*D5;}
};
template <class T* t, _uc D0,_uc D1,_uc D2,_uc D3,_uc D4>
class Indexer_5d{
	Indexer_5d(){};
	~Indexer_5d(){};
	T* operator()(_uc d0,_uc d1,_uc d2,_uc d3,_uc d4){
		return &t[(((((((d0*D1)+d1)*D2)+d2)*D3)+d3)*D4)+d4];
	}
	_uc numel()const{return D0*D1*D2*D3*D4;}
};
template <class T* t, _uc D0,_uc D1,_uc D2,_uc D3>
class Indexer_4d{
	Indexer_4d(){};
	~Indexer_4d(){};
	T* operator()(_uc d0,_uc d1,_uc d2,_uc d3){
		return &t[(((((d0*D1)+d1)*D2)+d2)*D3)+d3];
	}
	_uc numel()const{return D0*D1*D2*D3;}
};
template <class T* t, _uc D0,_uc D1,_uc D2>
class Indexer_3d{
	Indexer_3d(){};
	~Indexer_3d(){};
	T* operator()(_uc d0,_uc d1,_uc d2){
		return &t[(((d0*D1)+d1)*D2)+d2];
	}
	_uc numel()const{return D0*D1*D2;}
};
template <class T* t, _uc D0,_uc D1>
class Indexer_2d{
	Indexer_2d(){};
	~Indexer_2d(){};
	T* operator()(_uc d0,_uc d1){
		return &t[(d0*D1)+d1];
	}
	_uc numel()const{return D0*D1;}
};
template <class T* t, _uc D0>
class Indexer_1d{
	Indexer_1d(){};
	~Indexer_1d(){};
	T* operator()(_uc d0){
		return &t[d0];
	}
	_uc index(_uc& d0)const{return d0;}
	_uc numel()const{return D0;}
};

#endif /* INDEXING_H_ */