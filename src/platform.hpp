#ifndef NWCL_PLATFORM_HPP
#define NWCL_PLATFORM_HPP

#include "common.hpp"

using namespace v8;

namespace nwcl {

class Platform : public node::ObjectWrap {
private:
	cl_platform_id clHandle;

	Platform( cl_platform_id handle );
	~Platform();

public:
	static void Init( Handle<Object> exports );

	static Handle<Object> New( cl_platform_id handle );
	
	cl_platform_id GetID();

	static bool IsPlatform( Handle<Value> value );

	static void GetPlatforms( Handle<Function> callback );
	static Local<Array> GetPlatforms();
};
	
};

#endif