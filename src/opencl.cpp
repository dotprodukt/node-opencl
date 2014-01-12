#include <v8.h>
#include <node.h>

#include "common.hpp"
#include "platform.hpp"
#include "device.hpp"

using namespace v8;


#define SET_CL_ENUM( target, name ) target->Set(String::NewSymbol( #name ), Integer::New( CL_##name ), (PropertyAttribute)(ReadOnly|DontDelete) );

#define EXPORT_CL_ENUM( name ) SET_CL_ENUM( exports, name )

namespace nodecl {

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
		node::FatalException( trycatch );
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


V8_INVOCATION_CALLBACK( getPlatforms ){
	HandleScope scope;

	if( args.Length() ){
		if( !args[0]->IsFunction() ){
			ThrowException( Exception::TypeError(String::New("Expected callback function")));
			return scope.Close( Undefined() );
		}

		Local<Function> js_callback = Local<Function>::Cast( args[0] );

		start_getPlatforms_task( js_callback );

		return scope.Close( Undefined() );
	}

	return scope.Close( Platform::GetPlatforms() );
}

void Init( Handle<Object> exports ){
	INIT_EXPORT_V8_FUNCTION( exports, getPlatforms )

	Platform::Init( exports );
	Device::Init( exports );

	EXPORT_CL_ENUM( PLATFORM_PROFILE )
	EXPORT_CL_ENUM( PLATFORM_VERSION )
	EXPORT_CL_ENUM( PLATFORM_NAME )
	EXPORT_CL_ENUM( PLATFORM_VENDOR )
	EXPORT_CL_ENUM( PLATFORM_EXTENSIONS )

}


};

NODE_MODULE( opencl, nodecl::Init )