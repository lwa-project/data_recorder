#ifndef OBJECTBUFFER_H_
#define OBJECTBUFFER_H_

#include "Buffer.h"

namespace DRAPI {

template <class T>
class ObjectBuffer: public DRAPI::Buffer {
public:
	ObjectBuffer(size_t count, size_t alignment=16):Buffer(sizeof(T),count,alignment){
	}
	template <class T_other>
	ObjectBuffer(ObjectBuffer<T_other>& toVampire):
		Buffer(sizeof(T),toVampire,true){}

	inline T& operator[](size_t idx){
		return *((T*) Buffer::operator [](idx));
	}
	size_t getCount() const {return Buffer::getElementCount();}
	virtual ~ObjectBuffer(){

	}


};

} /* namespace DRAPI */
#endif /* OBJECTBUFFER_H_ */
