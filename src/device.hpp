#ifndef NODECL_DEVICE_HPP
#define NODECL_DEVICE_HPP

#include "common.hpp"
#include "platform.hpp"

using namespace v8;

namespace nodecl {

class Device : public node::ObjectWrap {
private:
	static Persistent<FunctionTemplate> constructorTemplate;
	static std::unordered_map<cl_device_id, Persistent<Object>*> deviceMap;

	cl_device_id clHandle;

	Device( cl_device_id handle );
	~Device();

	static V8_INVOCATION_CALLBACK( constructor );

public:

	static void Init( Handle<Object> exports );

	static Local<Object> New( cl_device_id handle );

	static bool IsDevice( Handle<Value> value );

	static Handle<Value> GetDevices( cl_platform_id platform, cl_device_type type = CL_DEVICE_TYPE_ALL );
	static Handle<Value> GetDevices( Platform* platform, cl_device_type type = CL_DEVICE_TYPE_ALL );

	static Handle<Object> GetDeviceByID( cl_device_id handle );
};

};

#endif