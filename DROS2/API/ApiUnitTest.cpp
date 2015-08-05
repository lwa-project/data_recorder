
#include "Buffer.h"
#include "ObjectBuffer.h"
#include "ObjectArrayBuffer.h"

typedef struct __s1{
	int a;
	int b;
	int c;
	int d;
	int e;
} s1;
typedef struct __s2{
	int a;
	int b;
	int c;
	int d;
	int e;
	int f;
	int g;
	int h;
	int i;
	int j;
} s2;

void DRAPI_UT(){

	DRAPI::Buffer* b1 = new DRAPI::Buffer(16384,64,16,true);
	assert(b1!=NULL);

	DRAPI::Buffer* b2 = new DRAPI::Buffer(32768,*b1,true);
	assert(b2!=NULL);

	delete(b1);

	assert((*b2)[31] != NULL);
	delete(b2);


	DRAPI::ObjectBuffer<s1>* bs1 = new DRAPI::ObjectBuffer<s1>(256,32);
	assert(bs1!=NULL);
	s1* el0   = &((*bs1)[0]);
	s1* el128 = &((*bs1)[128]);

	DRAPI::ObjectBuffer<s2>* bs2 = new DRAPI::ObjectBuffer<s2>(*bs1);
	assert(bs2!=NULL);
	s2* _el0   = &((*bs2)[0]);
	s2* _el64  = &((*bs2)[64]);

	assert((size_t)el0            == (size_t)_el0);
	assert((size_t)el128          == (size_t)_el64);
	assert(bs1->getCount()        == 0);
	assert(bs1->getAllocSize()    == 0);
	assert(bs1->getAlignment()    == 0);
	assert(bs1->getElementCount() == 0);

	assert(bs2->getCount()        == 0);
	assert(bs2->getAllocSize()    == 0);
	assert(bs2->getAlignment()    == 0);
	assert(bs2->getElementCount() == 0);






}

