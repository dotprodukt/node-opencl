#ifndef NWCL_HPP
#define NWCL_HPP

#include <v8.h>
#include <CL/cl.h>

namespace nwcl {

using namespace v8;

namespace internal {

template<typename T>
struct CLHandles {
	cl_uint length;
	T* elements;

	CLHandles(): length(0), elements(NULL){}
	~CLHandles(){
		if( elements != NULL ){
			delete[] elements;
			length = 0;
		}
	}

	template<typename S>
	Local<Array> ToArray(){
		Local<Array> arr = Array::New( length );
		for( cl_uint i=0; i<length; i++ ){
			arr->Set( i, S::New( elements[i] ));
		}
		return arr;
	}

	T Get( cl_uint index );
};

}}

#endif