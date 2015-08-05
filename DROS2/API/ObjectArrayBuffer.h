#ifndef OBJECTARRAYBUFFER_H_
#define OBJECTARRAYBUFFER_H_

#include "ObjectBuffer.h"

namespace DRAPI {
template <class T>
class ObjectArrayBuffer: private DRAPI::ObjectBuffer<T> {
public:
	ObjectArrayBuffer(size_t _width, size_t _depth, size_t alignment=16):
		DRAPI::ObjectBuffer<T>(_width*_depth, alignment),
		width(_width),
		depth(_depth){

	}

	template <class T_other>
	ObjectArrayBuffer(ObjectArrayBuffer<T_other>& toVampire):
		ObjectBuffer<T>(toVampire){
		size_t oldWidth = toVampire.width;
		size_t oldDepth = toVampire.depth;
		toVampire.width = 0;
		toVampire.depth = 0;
		size_t newCount = ObjectBuffer<T>::getCount();
		size_t _width   = (oldWidth * sizeof(T_other)) / sizeof(T);
		size_t _depth   = newCount / _width;
		while ((_depth < _width) && (_width>1)){
			_width >>= 1;
			_depth <<= 1;
		}
		width = _width;
		depth =  newCount / _width;;
	}

	inline size_t index(size_t x, size_t y)const {
		return (x + (y*width));
	}
	inline T& operator[](size_t idx){
		return ObjectBuffer<T>::operator[](idx);
	}
	virtual ~ObjectArrayBuffer(){

	}
private:
	size_t width;
	size_t depth;
};

} /* namespace DRAPI */
#endif /* OBJECTARRAYBUFFER_H_ */
