#include "common.hpp"
#include "device.hpp"
#include <iostream>

using namespace nodecl;

Persistent<FunctionTemplate> Device::constructorTemplate;
std::unordered_map<cl_device_id, Persistent<Object>*> Device::deviceMap;

bool guardNewDevice = TRUE;

Device::Device( cl_device_id handle ){
	clHandle = handle;
}

Device::~Device(){
	std::unordered_map<cl_device_id, Persistent<Object>*>::const_iterator pair
		= deviceMap.find( clHandle );

	if( pair == deviceMap.end() ){
		std::cout << "Redundant released of CLDevice wrapper.\n";
	}

	deviceMap.erase( clHandle );

	std::cout << "Released CLDevice wrapper.\n";
}

#define SET_DEVICE_TYPE_ENUM_HLP( target, name ) target->Set(String::NewSymbol( #name ), Integer::New( CL_DEVICE_TYPE_##name ), (PropertyAttribute)(ReadOnly|DontDelete|DontEnum) );

#define SET_DEVICE_TYPE_ENUM( name )\
	SET_DEVICE_TYPE_ENUM_HLP( constructorTemplate, name )\
	SET_DEVICE_TYPE_ENUM_HLP( prototype, name )

void Device::Init( Handle<Object> exports ){
	constructorTemplate = Persistent<FunctionTemplate>::New(FunctionTemplate::New( V8_INVOCATION_CALLBACK_NAME(constructor) ));
	constructorTemplate->SetClassName( String::NewSymbol( "CLDevice" ));
	constructorTemplate->InstanceTemplate()->SetInternalFieldCount(1);
	Handle<ObjectTemplate> prototype = constructorTemplate->PrototypeTemplate();

	SET_DEVICE_TYPE_ENUM( CPU )
	SET_DEVICE_TYPE_ENUM( GPU )
	SET_DEVICE_TYPE_ENUM( ACCELERATOR )
	SET_DEVICE_TYPE_ENUM( CUSTOM )
	SET_DEVICE_TYPE_ENUM( DEFAULT )
	SET_DEVICE_TYPE_ENUM( ALL )

	exports->Set( String::NewSymbol("Device"), constructorTemplate->GetFunction());

	//guardNewDevice = TRUE;
}

Local<Object> Device::New( cl_device_id handle ){
	Device* device = new Device( handle );

	guardNewDevice = FALSE;
	Local<Object> deviceObject = constructorTemplate->GetFunction()->NewInstance();
	guardNewDevice = TRUE;
	device->Wrap( deviceObject );

	deviceMap.insert( std::pair<cl_device_id, Persistent<Object>*>( handle, &device->handle_) );

	return deviceObject;
}

bool Device::IsDevice( Handle<Value> value ){
	return constructorTemplate->HasInstance( value );
}

Handle<String> getErrorMessage_getDevices( cl_int error ){
	switch( error ){
		case CL_INVALID_PLATFORM:
			return String::New("Invalid platform");
			break;
		case CL_INVALID_DEVICE_TYPE:
			return String::New("Invalid device type");
			break;
		case CL_OUT_OF_RESOURCES:
			return String::New("Out of device resources");
			break;
		case CL_OUT_OF_HOST_MEMORY:
			return String::New("Out of host memory");
			break;
	}
}

Handle<Value> Device::GetDevices( cl_platform_id platform, cl_device_type type ){
	cl_uint numDevices;
	cl_int err = clGetDeviceIDs( platform, type, 0, NULL, &numDevices );

	if( err != CL_SUCCESS ){
		if( err == CL_DEVICE_NOT_FOUND ){
			return Array::New();
		}

		ThrowException( Exception::Error(getErrorMessage_getDevices(err)));
		return Undefined();
	}

	cl_device_id* ids = new cl_device_id[ numDevices ];
	err = clGetDeviceIDs( platform, type, numDevices, ids, NULL );

	if( err != CL_SUCCESS ){
		ThrowException( Exception::Error(getErrorMessage_getDevices(err)));
		return Undefined();
	}

	Local<Array> devices = Array::New( numDevices );

	for( unsigned int i=0; i<numDevices; i++ ){
		devices->Set( i, GetDeviceByID( ids[i] ) );
	}
	delete[] ids;

	return devices;
}

Handle<Value> Device::GetDevices( Platform* platform, cl_device_type type ){
	return GetDevices( platform->GetHandle(), type );
}

Handle<Object> Device::GetDeviceByID( cl_device_id handle ){
	std::unordered_map<cl_device_id, Persistent<Object>*>::const_iterator pair
		= deviceMap.find( handle );

	if( pair == deviceMap.end() ){
		return Device::New( handle );
	}

	return *pair->second;
}

V8_INVOCATION_CALLBACK( Device::constructor ){
	HandleScope scope;

	if( guardNewDevice ){
		ThrowException( Exception::TypeError(String::New("Illegal constructor")) );
		return scope.Close( Undefined() );
	}

	return scope.Close( args.This() );
}