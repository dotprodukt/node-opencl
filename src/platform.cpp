#include "common.hpp"
#include "platform.hpp"
#include "device.hpp"
#include <iostream>

using namespace nodecl;

Persistent<FunctionTemplate> Platform::constructorTemplate;
std::unordered_map<cl_platform_id, Persistent<Object>*> Platform::platformMap;

bool guardNew = TRUE;

#define SET_PLATFORM_ENUM_PAIR( name )\
	SET_JS_ENUM( constructorTemplate, CL_PLATFORM_##name )\
	SET_JS_ENUM( prototype, CL_PLATFORM_##name )

#define PLATFORM_CLASS_METHOD( name ) INIT_EXPORT_V8_FUNCTION( constructorTemplate, name )

#define PLATFORM_PROTOTYPE_METHOD( name ) INIT_EXPORT_V8_FUNCTION( prototype, name )

Platform::Platform( cl_platform_id handle ){
	clHandle = handle;
}

Platform::~Platform(){
	std::unordered_map<cl_platform_id, Persistent<Object>*>::const_iterator pair
		= platformMap.find( clHandle );

	if( pair == platformMap.end() ){
		std::cout << "Redundant released of CLPlatform wrapper.\n";
	}

	platformMap.erase( clHandle );

	std::cout << "Released CLPlatform wrapper.\n";
}

cl_platform_id Platform::GetHandle(){
	return clHandle;
}

void Platform::Init( Handle<Object> exports ){
	constructorTemplate = Persistent<FunctionTemplate>::New(FunctionTemplate::New( V8_INVOCATION_CALLBACK_NAME(constructor) ));
	constructorTemplate->SetClassName( String::NewSymbol( "CLPlatform" ));
	constructorTemplate->InstanceTemplate()->SetInternalFieldCount(1);
	Handle<ObjectTemplate> prototype = constructorTemplate->PrototypeTemplate();

	SET_PLATFORM_ENUM_PAIR( PROFILE )
	SET_PLATFORM_ENUM_PAIR( VERSION )
	SET_PLATFORM_ENUM_PAIR( NAME )
	SET_PLATFORM_ENUM_PAIR( VENDOR )
	SET_PLATFORM_ENUM_PAIR( EXTENSIONS )

	PLATFORM_CLASS_METHOD( isPlatform )

	PLATFORM_PROTOTYPE_METHOD( getInfo )
	PLATFORM_PROTOTYPE_METHOD( getDevices )

	exports->Set( String::NewSymbol("Platform"), constructorTemplate->GetFunction());

	//guardNew = TRUE;
}

Local<Object> Platform::New( cl_platform_id handle ){
	Platform* platform = new Platform( handle );

	guardNew = FALSE;
	Local<Object> platformObject = constructorTemplate->GetFunction()->NewInstance();
	guardNew = TRUE;
	platform->Wrap( platformObject );

	platformMap.insert( std::pair<cl_platform_id, Persistent<Object>*>( handle, &platform->handle_) );

	return platformObject;
}

bool Platform::IsPlatform( Handle<Value> value ){
	return constructorTemplate->HasInstance( value );
}


struct GetPlatformsBaton : WorkBaton {
	cl_uint numPlatforms;
	cl_platform_id* platforms;
};


void getPlatforms_task( uv_work_t* task ){
	GetPlatformsBaton* baton = static_cast<GetPlatformsBaton*>(task->data);

	cl_uint numPlatforms;
	cl_int err = clGetPlatformIDs( 0, NULL, &numPlatforms );

	if( err != CL_SUCCESS ){
		baton->error = err;
		return;
	}

	cl_platform_id* ids = new cl_platform_id[ numPlatforms ];
	err = clGetPlatformIDs( numPlatforms, ids, NULL );

	if( err != CL_SUCCESS ){
		delete[] ids;
		baton->error = err;
		return;
	}

	baton->numPlatforms = numPlatforms;
	baton->platforms = ids;
}

void after_getPlatforms_task( uv_work_t* task, int status ){
	HandleScope scope;

	GetPlatformsBaton* baton = static_cast<GetPlatformsBaton*>(task->data);

	int numArgs = 1;
	if( baton->error == 0 && baton->platforms != NULL ){
		numArgs += 1;
	}

	Handle<Value>* argv = new Handle<Value>[ numArgs ];
	if( numArgs == 2 ){
		argv[0] = Undefined();
		Local<Array> args = Array::New( baton->numPlatforms );

		for( int i=0; i<baton->numPlatforms; i++ ){
			args->Set( i, Platform::GetPlatformByID( baton->platforms[i] ));
		}

		argv[1] = args;
	} else {
		argv[0] = Integer::New( baton->error );
	}

	TryCatch trycatch;

	baton->callback->Call( Context::GetCurrent()->Global(), numArgs, argv );

	if( trycatch.HasCaught() ){
		ThrowException( trycatch.Exception() );
	}

	baton->callback.Dispose();
	delete[] baton->platforms;
	delete baton;
}

void start_getPlatforms_task( Handle<Function> callback ){
	HandleScope scope;

	GetPlatformsBaton* baton = new GetPlatformsBaton();

	baton->task.data = (void*)baton;
	baton->callback = Persistent<Function>::New( callback );
	baton->numPlatforms = 0;
	baton->error = 0;
	baton->platforms = NULL;

	uv_queue_work( uv_default_loop(), &baton->task, getPlatforms_task, after_getPlatforms_task );
}

void Platform::GetPlatforms( Handle<Function> callback ){
	start_getPlatforms_task( callback );
}

Local<Array> Platform::GetPlatforms(){
	cl_uint numPlatforms;
	cl_int err = clGetPlatformIDs( 0, NULL, &numPlatforms );

	if( err != CL_SUCCESS ){
		// throw
	}

	cl_platform_id* ids = new cl_platform_id[ numPlatforms ];
	err = clGetPlatformIDs( numPlatforms, ids, NULL );

	if( err != CL_SUCCESS ){
		// throw
	}

	Local<Array> platforms = Array::New( numPlatforms );

	for( unsigned int i=0; i<numPlatforms; i++ ){
		platforms->Set( i, GetPlatformByID( ids[i] ));
	}
	delete[] ids;

	return platforms;
}

Handle<Object> Platform::GetPlatformByID( cl_platform_id handle ){
	std::unordered_map<cl_platform_id, Persistent<Object>*>::const_iterator pair
		= platformMap.find( handle );

	if( pair == platformMap.end() ){
		return Platform::New( handle );
	}

	return *pair->second;
}

Handle<String> getErrorMessage_getInfo( cl_int error ){
	switch( error ){
		case CL_INVALID_PLATFORM:
			return String::New("Invalid platform");
			break;
		case CL_INVALID_VALUE:
			return String::New("Invalid parameter enum");
			break;
		case CL_OUT_OF_HOST_MEMORY:
			return String::New("Out of host memory");
			break;
	}

	return String::NewSymbol("Unknown error");
}

V8_INVOCATION_CALLBACK( Platform::isPlatform ){
	HandleScope scope;
	return scope.Close( Boolean::New( IsPlatform(args[0]) ));
}

V8_INVOCATION_CALLBACK( Platform::constructor ){
	HandleScope scope;

	if( guardNew ){
		ThrowException( Exception::TypeError(String::New("Illegal constructor")) );
		return scope.Close( Undefined() );
	}

	return scope.Close( args.This() );
}

V8_INVOCATION_CALLBACK( Platform::getInfo ){
	HandleScope scope;

	if( !args.Length() ){
		ThrowException( Exception::Error(String::New("Expects parameter enum")));
		return scope.Close( Undefined() );
	}

	if( !(IsPlatform(args.This())) ){
		//ThrowException( Exception::TypeError(String::New("Illegal invocation")));
		return scope.Close( Undefined() );
	}

	if( !args[0]->IsInt32() ){
		ThrowException( Exception::TypeError(String::New("Parameter enum must be integer")));
		return scope.Close( Undefined() );
	}

	Platform* platform = node::ObjectWrap::Unwrap<Platform>( args.This() );

	cl_platform_info pi = (args[0]->ToInt32()->Value());

	size_t param_size;

	cl_int err = clGetPlatformInfo( platform->clHandle, pi, 0, NULL, &param_size );

	if( err != CL_SUCCESS ){
		ThrowException( Exception::Error(getErrorMessage_getInfo(err)));
		return scope.Close(Undefined());
	}


	char* info = new char[ param_size ];
	err = clGetPlatformInfo( platform->clHandle, pi, param_size, info, NULL );

	if( err != CL_SUCCESS ){
		delete[] info;
		ThrowException( Exception::Error(getErrorMessage_getInfo(err)));
		return scope.Close(Undefined());
	}	

	Local<String> infoString = String::New( info );
	delete[] info;

	return scope.Close( infoString );
}

V8_INVOCATION_CALLBACK( Platform::getDevices ){
	HandleScope scope;

	if( !(IsPlatform(args.This())) ){
		//ThrowException( Exception::TypeError(String::New("Illegal invocation")));
		return scope.Close( Undefined() );
	}

	Platform* platform = node::ObjectWrap::Unwrap<Platform>( args.This() );

	if( args.Length() ){
		if( !args[0]->IsInt32() ){
			ThrowException( Exception::TypeError(String::New("Expects device type")));
			return scope.Close( Undefined() );
		}

		return scope.Close( Device::GetDevices( platform, args[0]->ToInt32()->Value() ) );
	}



	return scope.Close( Device::GetDevices( platform ) );
}