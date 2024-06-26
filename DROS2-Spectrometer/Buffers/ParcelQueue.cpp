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

#include "ParcelQueue.h"
#include "../Common/branchPredict.h"
#include "../Common/misc.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <boost/thread.hpp>
#include <boost/random.hpp>

using namespace std;
using namespace boost;

ParcelQueue::ParcelQueue(const size_t &numParcels, const size_t &numAllocations, const bool &clearUnused):
	abuf(numAllocations,3),
	pbuf(numParcels,1),
	palloc_peek(0),	palloc_offset(0),
	pfree_peek(0),	pfree_offset(0),
	haveFragment(false),
	clearUnused(clearUnused),
	valid(abuf.isValid() && pbuf.isValid()),
	consumerDone(false),
	remnant()
{
	remnant.iov_base = NULL;
	remnant.iov_len  = 0;
	cout << "\n\n ParcelQueue constructor entered ("<<numParcels << "," << numAllocations << "," << clearUnused <<").\n\n";

}
void ParcelQueue::reset(){
	if (unlikely(!valid)) return;
	abuf.reset();
	pbuf.reset();
	palloc_peek      = 0;
	palloc_offset    = 0;
	pfree_peek       = 0;
	pfree_offset     = 0;
	haveFragment     = false;
	valid            = abuf.isValid() && pbuf.isValid();
	consumerDone     = false;
	remnant.iov_base = NULL;
	remnant.iov_len  = 0;
}

ParcelQueue::~ParcelQueue() {
	// nothing special to do
}

// =====================================================
// the allocator side interface
Allocation* ParcelQueue::nextAllocIn() const{
	if (unlikely(!valid)) return NULL;
	return abuf.nextIn();
}
void        ParcelQueue::doneAllocIn(Allocation* ptr){
	if (unlikely(!valid)) return;
	abuf.doneIn(ptr);
}
Allocation* ParcelQueue::nextAllocOut() const{
	if (unlikely(!valid)) return NULL;
	return abuf.nextOut();
}
void        ParcelQueue::doneAllocOut(Allocation* ptr){
	if (unlikely(!valid)) return;
	abuf.doneOut(ptr);
}

size_t		ParcelQueue::numAllocUsed()const{
	if (unlikely(!valid)) return 0;
	return abuf.used();
}

// =====================================================
// single-parcel user-side interface
Parcel*     ParcelQueue::nextParcel(size_t size){
	static int cnt = 0;
	Parcel* ptmp = pbuf.nextOut();
	if (unlikely(ptmp!=NULL)){
		if (unlikely((ptmp->iov[0].iov_len+ptmp->iov[1].iov_len) != size)){
			cout << "edata:["<<cnt<<" / "<<ptmp->iov[0].iov_len<<" / "<<ptmp->iov[1].iov_len<<" / "<<size<<"]";
			FATAL(USAGE_ERROR,"leftover parcel size is incompatible with new msg_size.");
		}
		return ptmp;
	} else {
		if (unlikely(!__allocParcel(size))){
			__allocParcel(size);
		}
		cnt++;
		return pbuf.nextOut();
	}
}

void        ParcelQueue::doneParcel(Parcel* ptr){
	if (unlikely(ptr != pbuf.nextOut())){
		FATAL(USAGE_ERROR,"leftover parcel returned is not the tail in the parcel buffer.");
	}
	__freeParcel();
}

// =====================================================
// multi-parcel user-side interface (mutually exclusive)

// socket recvmmsg preparation/finalization
// note: only msgvec[X].msg_hdr.msg_iov and msgvec[X].msg_hdr.msg_iovlen are modified / interacted with
size_t  ParcelQueue::nextMsgs(const size_t msg_size, size_t msg_count, struct mmsghdr *msgvec){
	//cout << "\n\n%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n\n\n";
	size_t prepared = 0;
	Parcel*      ptmp = pbuf.nextStage(PBUF_UNUSED);
	// first fill from any leftovers in the parcel queue
	while ((prepared<msg_count) && (ptmp!=NULL)){
		msgvec[prepared].msg_hdr.msg_iov    = &ptmp->iov[0];
		msgvec[prepared].msg_hdr.msg_iovlen = (ptmp->iov[1].iov_len != 0) ? 2 : 1;
		if (unlikely((ptmp->iov[0].iov_len+ptmp->iov[1].iov_len) != msg_size)){
			FATAL(USAGE_ERROR,"leftover parcel size is incompatible with new msg_size.");
		}
		prepared++;
		ptmp = pbuf.peekStage(PBUF_UNUSED,prepared);
		//cout << "N";cout.flush();
	}
	// attempt to allocate <msg_count - prepared> more parcels
	__allocNParcel(msg_size, msg_count - prepared);
	ptmp = pbuf.peekStage(PBUF_UNUSED,prepared);
	while ((prepared<msg_count) && (ptmp!=NULL)){
		msgvec[prepared].msg_hdr.msg_iov    = &ptmp->iov[0];
		msgvec[prepared].msg_hdr.msg_iovlen = (ptmp->iov[1].iov_len != 0) ? 2 : 1;
		if (unlikely((ptmp->iov[0].iov_len+ptmp->iov[1].iov_len) != msg_size)){
			FATAL(USAGE_ERROR,"leftover parcel size is incompatible with new msg_size.");
		}
		prepared++;
		ptmp = pbuf.peekStage(PBUF_UNUSED,prepared);
		//cout << "n";cout.flush();
	}
	return prepared;
}

void ParcelQueue::doneMsgs(size_t messagesUsed, struct mmsghdr *msgvec){
	size_t released = 0;
	Parcel*      ptmp = pbuf.nextOut();
	while((released<messagesUsed) && ptmp){
		//cout << "#";cout.flush();
		if (unlikely(msgvec[released].msg_hdr.msg_iov != (&ptmp->iov[0]))){
			FATAL(PROGRAM_ERROR,"Msgs structure out of sync with parcel buffer.");
		}

		if (clearUnused){
			size_t l1 = msgvec[released].msg_len;
			size_t la = ptmp->iov[0].iov_len;
			size_t lb = ptmp->iov[1].iov_len;
			size_t l0 = la + lb;
			if (unlikely(l1 != l0)){
				if(unlikely(lb!=0)){//split
					if (l1 < la) bzero(  PTR_TO_BYTE(ptmp->iov[0].iov_base, l1      ),      la - l1); // clear first fragment unused
					             bzero(  PTR_TO_BYTE(ptmp->iov[1].iov_base,(l1 - la)), lb - (l1-la)); // clear second fragment unused
				} else {
					             bzero(  PTR_TO_BYTE(ptmp->iov[0].iov_base, l1      ),      la - l1); // clear unused
				}
			}
		}
		__freeParcel();
		released++;
		ptmp = pbuf.nextOut();
		//cout << "D";cout.flush();
	}
	if (unlikely(released<messagesUsed)){
		FATAL(PROGRAM_ERROR,"Failed to release all messages.");
	}
}
void ParcelQueue::signalParcelConsumerDone(){
	if (unlikely(!valid)) return;
	consumerDone=true;
}

bool ParcelQueue::isConsumerDone()const{
	return valid && consumerDone;
}

Allocation* ParcelQueue::getFractionalCompletion(){
	if (unlikely(!valid)) return NULL;
	if (unlikely(!consumerDone))
		FATAL(USAGE_ERROR,"called getFractionalCompletion but consumer wasn't finished")
	Allocation* atmp = __freeLocn();
	if (unlikely(!atmp)) return NULL;
	remnant.iov_base = atmp->iov_base;
	remnant.iov_len  = pfree_offset;
	return &remnant;
}

// move a free allocation into the used queue, return true if successful
bool ParcelQueue::__grabFreeAlloc(){
	return abuf.advStage(ABUF_UNUSED);
}

// move a free allocation into the used queue, return true if successful
bool ParcelQueue::__ungrabInUseAlloc(){
	if (abuf.advStage(ABUF_USING)){
		palloc_peek--;
		pfree_peek--;
		return true;
	} else {
		return false;
	}
}

void ParcelQueue::__advanceAlloc(){
	//cout << "\n[^]\n"; cout.flush();
	palloc_offset = 0;
	palloc_peek++;
}

void ParcelQueue::__advanceFree(){
	//cout << "\n[v]\n"; cout.flush();
	pfree_offset= 0;
	pfree_peek++;
	if (unlikely(!__ungrabInUseAlloc())){
		FATAL(PROGRAM_ERROR,"__advFree() called, but __ungrabInUseAlloc() failed.");
	}
}

// find the next usable allocation; if none, try to grab from the free pool
Allocation* ParcelQueue::__allocLocn(){
	Allocation* atmp =  abuf.peekStage(ABUF_USING,palloc_peek);
	if (likely(atmp!=NULL)) {
		return atmp;
	} else {
		if (__grabFreeAlloc()){
			atmp =  abuf.peekStage(ABUF_USING,palloc_peek);
			if (unlikely(!atmp)){
				FATAL(PROGRAM_ERROR,"__grabFreeAlloc() succeeded but subsequent peek failed to return a usable allocator.");
			}
			return atmp;
		}
		return NULL;
	}
}

Allocation* ParcelQueue::__freeLocn(){
	Allocation* atmp =  abuf.peekStage(ABUF_USING,pfree_peek);
	if (unlikely(!atmp)){
		FATAL(PROGRAM_ERROR,"no in-use allocators when freeLocn() was called.");
	}
	return atmp;
}
// allocate N parcels and insert them into the parcel queue, return the number allocated
size_t ParcelQueue::__allocNParcel(const size_t size, const size_t count){
	size_t nalloc = 0;
	// repeat the loop twice since a boundary corner case will return false on the first
	// pass, but succeed the second time around if there's another allocation available
	while((nalloc<count)  && __allocParcel(size)){nalloc++;}
	while((nalloc<count)  && __allocParcel(size)){nalloc++;}
	return nalloc;
}
// allocate a parcel and insert it into the parcel queue, return true on success
bool ParcelQueue::__allocParcel(const size_t size){
	Allocation* atmp        = __allocLocn();
	if (unlikely(!atmp))	 return false;

	Parcel* ptmp            = pbuf.nextIn();
	if (unlikely(!ptmp))      return false;
	if (unlikely(atmp->iov_len <= palloc_offset)){
		FATAL(PROGRAM_ERROR,"ended up with allocation offset larger than current allocation.");
	}
	size_t bytesAvail =  atmp->iov_len - palloc_offset;
	if (unlikely(haveFragment)){
		size_t reqSize = (size - ptmp->iov[0].iov_len);
		if (unlikely(bytesAvail < reqSize)){
			// already working with fragment
			FATAL(USAGE_ERROR,"allocation geometry would have caused a 3-fragment parcel, which is not possible.");
		}
		ptmp->iov[1].iov_base  = PTR_TO_BYTE(atmp->iov_base,palloc_offset);
		ptmp->iov[1].iov_len   = reqSize;
		palloc_offset    += reqSize;
		if (unlikely(palloc_offset == atmp->iov_len)){
			__advanceAlloc();
		}
		//cout << "<";cout.flush();
		pbuf.doneIn(ptmp);
		haveFragment=false;
		return true;
	} else {
		struct iovec* toFill = (&ptmp->iov[0]);
		toFill->iov_base  = PTR_TO_BYTE(atmp->iov_base,palloc_offset);
		if (unlikely(bytesAvail < size)){
			// set up fragment
			toFill->iov_len   = bytesAvail;
			__advanceAlloc();
			haveFragment=true;
			//cout << "{";cout.flush();
			return false;
		} else {
			toFill->iov_len       = size;
			ptmp->iov[1].iov_base = NULL;
			ptmp->iov[1].iov_len  = 0;
			palloc_offset     += size;
			if  (unlikely(palloc_offset == atmp->iov_len)){
				__advanceAlloc();
			}
			//cout << "<";cout.flush();
			pbuf.doneIn(ptmp);
			return true;
		}
	}
}

// free the the used parcel-queue's tail
void ParcelQueue::__freeParcel(){
	Parcel* ptmp            = pbuf.nextOut();
	if (unlikely(!ptmp))      return;

	Allocation* atmp        = __freeLocn();
	size_t bytesFreeable = atmp->iov_len - pfree_offset;

	if (unlikely(bytesFreeable < ptmp->iov[0].iov_len))
		FATAL(PROGRAM_ERROR,"incorrect parcel (or split-head) size.");

	if (unlikely(ptmp->iov[0].iov_base != PTR_TO_BYTE(atmp->iov_base,pfree_offset))){
		cout 	<< "\nedata:["
				<< ptmp->iov[0].iov_base << ", "
				<< PTR_TO_BYTE(atmp->iov_base,pfree_offset) << ", "
				<< pfree_offset<< " "
				<<"]\n";
		FATAL(PROGRAM_ERROR,"incorrect parcel (or split-head) address.");
	}
	if (unlikely(ptmp->iov[1].iov_len != 0)){
		// split case
		__advanceFree();
		atmp        = __freeLocn();
		if (unlikely(!atmp))
			FATAL(PROGRAM_ERROR,"split-parcel with no tail allocation.");

		bytesFreeable = atmp->iov_len;
		if (unlikely(bytesFreeable < ptmp->iov[1].iov_len))
			FATAL(PROGRAM_ERROR,"incorrect split-parcel tail size.");

		if (unlikely(ptmp->iov[1].iov_base != atmp->iov_base))
			FATAL(PROGRAM_ERROR,"incorrect split-parcel head address.");

		pfree_offset += ptmp->iov[1].iov_len;
	} else {
		// single case
		pfree_offset += ptmp->iov[0].iov_len;
	}
	if((unlikely(pfree_offset >= atmp->iov_len))){
		if((unlikely(pfree_offset > atmp->iov_len))){
			FATAL(PROGRAM_ERROR,"parcel over-consumed allocation.");
		}
		__advanceFree();
	}
	//cout << ">";cout.flush();
	pbuf.doneOut(ptmp);
}
void ParcelQueue::print()const{
	size_t pused     = pbuf.used();
	size_t a_allocd  = abuf.usedInStage(0);
	size_t a_using   = abuf.usedInStage(1);
	size_t a_used    = abuf.usedInStage(2);
	size_t a_total_u = abuf.used();
	cout << "\tPQ-P   [ " << setw(10) << 0 << " / " << setw(10) << 0 << " / " << setw(15) << pused << " ]";
	progressbar((pused*100ll)/pbuf.size(),25);
	cout << "\n";
	cout << "\tPQ-A   [ " << setw(10) << a_allocd << " / " << setw(10) << a_using << " / " << setw(15) << a_used << " ]";
	progressbar((a_total_u*100ll)/abuf.size(),25);
	cout << "\n";
}

#ifdef CONFIG_UNIT_TESTS

#define UT_BURN (1048576ll * 2024ll)
#define UT_A_UNIT  262144
#define UT_A_COUNT 32
#define UT_BIG_MODULOUS (UT_A_UNIT * UT_A_COUNT)
#define UT_P_UNIT  1024
#define UT_P_COUNT 64

void __PQ_Consumer(){
	cout << "Consumer started ...\n";
	struct mmsghdr msgs[UT_P_COUNT];
	size_t burn      = 0;
	size_t toget     = 0;
	size_t gotten    = 0;
	size_t toConfirm = 0;
	size_t span      = (size_t) -1ll;
	static size_t np = 0;
	bool shouldSpan = false;
	ParcelQueue* utpq = ParcelQueue::utpq;
	boost::random::mt19937 rng;
	boost::random::uniform_int_distribution<> countGenerator(10,UT_P_COUNT-1);
	boost::random::uniform_int_distribution<> psizeGenerator(0,UT_P_UNIT);
	while(burn < UT_BURN){
		toget  = countGenerator(rng);
		gotten = utpq->nextMsgs((size_t) UT_P_UNIT, toget, &msgs[0]);
		for (size_t i=0; i<gotten; i++){
			size_t correct_address = (((np+i)*UT_P_UNIT)) % ((size_t)UT_BIG_MODULOUS);
			size_t packet_address  = (size_t) msgs[i].msg_hdr.msg_iov[0].iov_base;
			if (packet_address != correct_address){
				FATAL(UNIT_TEST_FAILURE,"Parcel Alignment");
			}
			size_t correct_end     = (burn + ((i+1)*UT_P_UNIT)-1) % ((size_t)UT_BIG_MODULOUS);
			size_t ca_block        = correct_address / UT_A_UNIT;
			size_t ce_block        = correct_end / UT_A_UNIT;
			shouldSpan = (ca_block != ce_block);

			//shouldSpan = (correct_address & ~(UT_A_UNIT-1)) != ((correct_address+UT_P_UNIT-1) & ~(UT_A_UNIT-1));
			if ((shouldSpan && (msgs[i].msg_hdr.msg_iovlen != 2)) || (!shouldSpan && (msgs[i].msg_hdr.msg_iovlen != 1))){
				cout << "\nedata:["
						<< burn << ", "
						<< i << ", "
						<< correct_address << ", "
						<< packet_address << ", "
						<< (correct_address & ~(UT_A_UNIT-1)) << ", "
						<< ((correct_address+UT_P_UNIT-1) & ~(UT_A_UNIT-1)) << ", "
						<< msgs[i].msg_hdr.msg_iovlen << ", "
						<< msgs[i].msg_hdr.msg_iov[0].iov_base << ", "
						<< msgs[i].msg_hdr.msg_iov[1].iov_base << ", "
						<< msgs[i].msg_hdr.msg_iov[0].iov_len << ", "
						<< msgs[i].msg_hdr.msg_iov[1].iov_len << " "
						<<"]\n";
				FATAL(UNIT_TEST_FAILURE,"Spanning +/- wrong");
			}
			if (shouldSpan){
				span=i;
			}
		}
		if(gotten){
			do {
				toConfirm = countGenerator(rng);
			} while (toConfirm > gotten);
			toConfirm=gotten;
			for (size_t i=0; i<toConfirm; i++){
				msgs[i].msg_len = psizeGenerator(rng);
				burn += (size_t) UT_P_UNIT;
			}
			for (size_t i=toConfirm; i<gotten; i++){
				msgs[i].msg_len = 0;
				//cout << "-";cout.flush();
			}
/*
			cout << hex << "( ["<<np<<"] "<< dec<< toget <<", "<< gotten <<", "<< toConfirm <<")";
			if  ((span <= toConfirm) && (span!=(size_t) -1ll)){
				cout << " span @ " << span;
			}
			cout << endl;
//*/
			np+=toConfirm;
		}
		utpq->doneMsgs(toConfirm,&msgs[0]);
	}
	cout << "Consumer finished ...\n";
}
void __PQ_Producer(){
	usleep(1000000);
	cout << "Producer started ...\n";
	size_t burnIn  = 0;
	size_t burnOut = 0;
	ParcelQueue* utpq = ParcelQueue::utpq;
	Allocation* atmp;
	do{
		if ((atmp = utpq->nextAllocOut()) != NULL){
			if (atmp->iov_base != PTR_TO_BYTE(NULL,(burnOut % (size_t)UT_BIG_MODULOUS))){
				FATAL(UNIT_TEST_FAILURE,"Recovered allocation addressing wrong");
			}
			utpq->doneAllocOut(atmp);
			burnOut += UT_A_UNIT;
			//cout << "-";cout.flush();
		}
		if ((atmp = utpq->nextAllocIn()) != NULL){
			atmp->iov_base = PTR_TO_BYTE(NULL,(burnIn % (size_t)UT_BIG_MODULOUS));
			atmp->iov_len  = UT_A_UNIT;
			utpq->doneAllocIn(atmp);
			burnIn += UT_A_UNIT;
			//cout << "+";
			if (burnIn > (UT_BURN + UT_BIG_MODULOUS + UT_A_UNIT)){
				FATAL(UNIT_TEST_FAILURE,"Too much insertion");
			}

		}
	} while (burnOut < UT_BURN);
	cout << "Producer finished ...\n";

}

ParcelQueue* ParcelQueue::utpq=NULL;
void ParcelQueue::unitTest(){
	if (!utpq)
		utpq = new ParcelQueue(256,32,false);
	if (!utpq){
		FATAL(UNIT_TEST_FAILURE,"Can't create parcel queue");
	}
	cout << "Beginning Unit Test\n";
	cout << "Launch Producer\n";
	boost::thread utt1(__PQ_Producer);
	cout << "Launch Consumer\n";
	boost::thread utt2(__PQ_Consumer);
	cout << "Sleep on join Consumer\n";
	utt1.join();
	cout << "Sleep on join thread 2\n";
	utt2.join();
	cout << "Finished\n";
}
#endif

