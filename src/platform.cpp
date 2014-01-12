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


Local<Array> arrayFromPlatformIDs( cl_uint numPlatforms, cl_platform_id* platforms ){
	Local<Array> arr = Array::New( numPlatforms );
	for( int i=0; i<numPlatforms; i++ ){
		arr->Set( i, Platform::GetPlatformByID( platforms[i] ) );
	}
	return arr;
}

cl_int getPlatformIDs( cl_uint* numPlatforms, cl_platform_id** platforms ){
	cl_int err = clGetPlatformIDs( 0, NULL, numPlatforms );
	
	if( err != CL_SUCCESS || *numPlatforms == 0 ){
		*numPlatforms = 0;
		*platforms = NULL;
		return err;
	}

	*platforms = new cl_platform_id[ *numPlatforms ];
	
	err = clGetPlatformIDs( *numPlatforms, *platforms, NULL );
	
	if( err != CL_SUCCESS ){
		delete[] *platforms;
		*platforms = NULL;
	}

	return err;
}

struct PlatformsBaton : CLBaton<uv_work_t> {
	cl_uint numPlatforms;
	cl_platform_id* platforms;

	PlatformsBaton(): CLBaton(),
		numPlatforms(0),
		platforms(NULL){}
	PlatformsBaton( Handle<Function> callback ): CLBaton( callback ),
		numPlatforms(0),
		platforms(NULL){}

	~PlatformsBaton(){
		if( platforms != NULL ) delete[] platforms;
	}
};

// called in thread from uv thread pool
void getPlatforms_task( uv_work_t* task ){
	PlatformsBaton* baton = static_cast<PlatformsBaton*>(task->data);

	baton->error = getPlatformIDs( &baton->numPlatforms, &baton->platforms );
}

// called in main thread
void after_getPlatforms_task( uv_work_t* task, int status ){
	HandleScope scope;

	PlatformsBaton* baton = static_cast<PlatformsBaton*>(task->data);

	Handle<Value>* argv = new Handle<Value>[ 2 ];

	argv[0] = Integer::New( baton->error );

	if( baton->error == CL_SUCCESS ){
		argv[1] = arrayFromPlatformIDs( baton->numPlatforms, baton->platforms );
	} else {
		argv[1] = Undefined();
	}

	TryCatch trycatch;

	baton->callback->Call( Context::GetCurrent()->Global(), 2, argv );

	if( trycatch.HasCaught() ){
		ThrowException( trycatch.Exception() );
	}

	delete baton;
}

void start_getPlatforms_task( Handle<Function> callback ){
	PlatformsBaton* baton = new PlatformsBaton( callback );

	uv_queue_work( uv_default_loop(), &baton->task, getPlatforms_task, after_getPlatforms_task );
}

void Platform::GetPlatforms( Handle<Function> callback ){
	start_getPlatforms_task( callback );
}

Local<Array> Platform::GetPlatforms(){
	cl_uint numPlatforms;
	cl_platform_id* ids;

	if( getPlatformIDs( &numPlatforms, &ids ) != CL_SUCCESS ){
		// throw
	}

	return arrayFromPlatformIDs( numPlatforms, ids );
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