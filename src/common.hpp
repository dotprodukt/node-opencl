#ifndef NWCL_COMMON_HPP
#define NWCL_COMMON_HPP

#include <v8.h>
#include <node.h>
#include "nwcl.hpp"

#define SET_JS_ENUM( target, name ) target->Set(String::NewSymbol( #name ), Integer::New( name ), (PropertyAttribute)(ReadOnly|DontDelete|DontEnum) );

#define V8_FUNCTION_PREFIX v8_function_
#define V8_SYMBOL_PREFIX v8_symbol_

#define V8_INVOCATION_CALLBACK_PREFIX v8_invocation_callback_
#define V8_INVOCATION_CALLBACK_SUFFIX _invocation_callback

#define V8_INVOCATION_CALLBACK_NAME( name ) name##V8_INVOCATION_CALLBACK_SUFFIX

#define V8_INVOCATION_CALLBACK( name ) v8::Handle<v8::Value> V8_INVOCATION_CALLBACK_NAME( name )( const v8::Arguments& args )

#define V8_SET( target, key, value ) target->Set( key, value );

#define INIT_JS_METHOD( target, name ){\
Local<Function> function = FunctionTemplate::New( V8_INVOCATION_CALLBACK_NAME(name) )->GetFunction();\
Local<String> symbol = String::NewSymbol( #name );\
function->SetName( symbol );\
target->Set( symbol, function );\
}

#define INIT_V8_FUNCTION( name ) Local<Function> V8_FUNCTION_PREFIX##name = FunctionTemplate::New( V8_INVOCATION_CALLBACK_NAME(name) )->GetFunction();\
Local<String> V8_SYMBOL_PREFIX##name = String::NewSymbol( #name );\
V8_FUNCTION_PREFIX##name->SetName( V8_SYMBOL_PREFIX##name );

#define EXPORT_V8_FUNCTION( target, name, value ) V8_SET( target, String::NewSymbol( #name ), value )

#define INIT_EXPORT_V8_FUNCTION( target, name ){\
INIT_V8_FUNCTION( name )\
V8_SET( target, V8_SYMBOL_PREFIX##name, V8_FUNCTION_PREFIX##name )\
}


template<typename T>
struct Baton {
	T task;

	Baton(){
		task.data = (void*)this;
	}
};

template<typename T>
struct NodeBaton: Baton<T> {
	v8::Persistent<v8::Function> callback;

	NodeBaton(): Baton<T>(){}
	NodeBaton( v8::Handle<v8::Function> callback ): Baton<T>(),
		callback( v8::Persistent<v8::Function>::New( callback )){}

	~NodeBaton(){
		callback.Dispose();
	}
};

template<typename T>
struct CLBaton: NodeBaton<T> {
	int error;

	CLBaton(): NodeBaton<T>(),
		error(CL_SUCCESS){}
	CLBaton( v8::Handle<v8::Function> callback ): NodeBaton<T>( callback ),
		error(CL_SUCCESS){}
};

struct CLWorkBaton: CLBaton<uv_work_t>{
	CLWorkBaton(): CLBaton<uv_work_t>(){}
	CLWorkBaton( v8::Handle<v8::Function> callback ):
		CLBaton<uv_work_t>( callback ){}
};

struct CLAsyncBaton: CLBaton<uv_async_t>{
	CLAsyncBaton(): CLBaton<uv_async_t>(){}
	CLAsyncBaton( v8::Handle<v8::Function> callback ):
		CLBaton<uv_async_t>( callback ){}
};

#endif