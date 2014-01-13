#ifndef NODECL_PLATFORM_HPP
#define NODECL_PLATFORM_HPP

#include "common.hpp"
#include <unordered_map>

using namespace v8;

namespace nodecl {

class Platform : public node::ObjectWrap {
private:
	static Persistent<FunctionTemplate> constructorTemplate;
	static std::unordered_map<cl_platform_id, Persistent<Object>*> platformMap;

	cl_platform_id clHandle;

	Platform( cl_platform_id handle );
	~Platform();

public:
	cl_platform_id GetHandle();

	static void Init( Handle<Object> exports );

	static Local<Object> New( cl_platform_id handle );
	
	static bool IsPlatform( Handle<Value> value );

	static void GetPlatforms( Handle<Function> callback );
	static Local<Array> GetPlatforms();
	
	static Handle<Object> GetPlatformByID( cl_platform_id handle );

};
	
};

#endif