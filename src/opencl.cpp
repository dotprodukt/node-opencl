#include <v8.h>
#include <node.h>

#include "common.hpp"
#include "platform.hpp"

using namespace v8;


#define SET_CL_ENUM( target, name ) target->Set(String::NewSymbol( #name ), Integer::New( CL_##name ), (PropertyAttribute)(ReadOnly|DontDelete) );

#define EXPORT_CL_ENUM( name ) SET_CL_ENUM( exports, name )

namespace nodecl {


V8_INVOCATION_CALLBACK( getPlatforms ){
	HandleScope scope;
	return scope.Close( Platform::GetPlatforms() );
}

void Init( Handle<Object> exports ){
	INIT_EXPORT_V8_FUNCTION( exports, getPlatforms )

	Platform::Init( exports );

	EXPORT_CL_ENUM( PLATFORM_PROFILE )
	EXPORT_CL_ENUM( PLATFORM_VERSION )
	EXPORT_CL_ENUM( PLATFORM_NAME )
	EXPORT_CL_ENUM( PLATFORM_VENDOR )
	EXPORT_CL_ENUM( PLATFORM_EXTENSIONS )

}


};

NODE_MODULE( opencl, nodecl::Init )