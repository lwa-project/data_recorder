#ifndef BUFFER_H_
#define BUFFER_H_
#include <cstdlib>
#include <cstring>
#include <cassert>


namespace DRAPI {



class Buffer {
public:
	Buffer(const size_t _elementSize, const size_t _elementCount, const size_t _baseAlignment = 16, const bool clear = true) :
			allocSize(0),
			elementSize(_elementSize),
			elementCount(_elementCount),
			baseAlignment(_baseAlignment),
			baseAsAllocated(NULL),
			baseAsAligned(NULL) {


		size_t s = (elementSize * elementCount) + baseAlignment;
		baseAsAllocated = malloc(s);
		if (!baseAsAllocated)
			return;
		if (clear) {
			bzero(baseAsAllocated, s);
		}
		allocSize  = elementSize * elementCount;
		size_t ptr = (size_t) baseAsAllocated;
		size_t mask = baseAlignment - 1;
		size_t ofs = ptr & mask;
		if (ofs != 0) {
			baseAsAligned = (void*) (ptr + baseAlignment - ofs);
		} else {
			baseAsAligned = baseAsAllocated;
		}
	}
	Buffer(const size_t _elementSize, Buffer& toVampire, const bool clear = true):
		allocSize(0),
		elementSize(0),elementCount(0),
		baseAlignment(0),
		baseAsAllocated(NULL),
		baseAsAligned(NULL){
		toVampire.swapStorage(*this);
		size_t __attribute__((unused)) ignored = reshape(_elementSize);
		if (clear) bzero(baseAsAllocated, allocSize);
	}

	inline void* operator[](size_t idx) {
		assert(idx < elementCount);
		if (!baseAsAligned)
			return NULL;
		else
			return (void*) &(((char*) baseAsAligned)[idx * elementSize]);
	}

	virtual void swapStorage(Buffer& b_getsTheStorage){
		if (baseAsAllocated){
			void*  t1 = b_getsTheStorage.baseAsAllocated;
			void*  t2 = b_getsTheStorage.baseAsAligned;
			size_t a  = b_getsTheStorage.baseAlignment;
			size_t b  = b_getsTheStorage.elementSize;
			size_t c  = b_getsTheStorage.elementCount;
			size_t d  = b_getsTheStorage.allocSize;

			b_getsTheStorage.baseAsAllocated = this->baseAsAllocated;
			b_getsTheStorage.baseAsAligned   = this->baseAsAligned;
			b_getsTheStorage.baseAlignment   = this->baseAlignment;
			b_getsTheStorage.elementSize     = this->elementSize;
			b_getsTheStorage.elementCount    = this->elementCount;
			b_getsTheStorage.allocSize       = this->allocSize;

			this->baseAsAllocated = t1;
			this->baseAsAligned   = t2;
			this->baseAlignment   = a;
			this->elementSize     = b;
			this->elementCount    = c;
			this->allocSize       = d;
		}
	}
	virtual size_t reshape(size_t newElementSize, size_t newElementCount=0){
		assert (newElementSize != 0);
		size_t __newElementSize  = newElementSize;
		size_t __newElementCount = newElementCount ? newElementCount : (allocSize/newElementSize);
		if (__newElementCount < 1) __newElementCount = 1;
		size_t __newSize = __newElementCount * __newElementSize + baseAlignment;

		if ((__newSize-baseAlignment) > allocSize){
			void* tmp = realloc(baseAsAllocated, __newSize);
			if (tmp == NULL){
				return 0;
			} else {
				baseAsAllocated = tmp;
				size_t ptr  = (size_t) baseAsAllocated;
				size_t mask = baseAlignment - 1;
				size_t ofs  = ptr & mask;
				if (ofs != 0) {
					baseAsAligned = (void*) (ptr + baseAlignment - ofs);
				} else {
					baseAsAligned = baseAsAllocated;
				}
				allocSize = __newElementCount * __newElementSize;
			}
		}
		elementSize     = __newElementSize;
		elementCount    = __newElementCount;
		return elementCount;
	}

	size_t getElementSize()  const {return elementSize;  }
	size_t getElementCount() const {return elementCount; }
	size_t getAllocSize()    const {return allocSize;    }
	size_t getAlignment()    const {return baseAlignment;}


	virtual ~Buffer() {
		if (baseAsAllocated)
			free(baseAsAllocated);
	}

private:
	Buffer():allocSize(0),elementSize(0),elementCount(0),baseAlignment(0),baseAsAllocated(NULL),baseAsAligned(NULL){}
	Buffer(const Buffer& toCopy):allocSize(0),elementSize(0),elementCount(0),baseAlignment(0),baseAsAllocated(NULL),baseAsAligned(NULL){}
	Buffer& operator=(const Buffer& toCopy){ return *this; }

	size_t allocSize;
	size_t elementSize;
	size_t elementCount;
	size_t baseAlignment;
	void* baseAsAllocated;
	void* baseAsAligned;
};

} /* namespace DRAPI */
#endif /* BUFFER_H_ */

