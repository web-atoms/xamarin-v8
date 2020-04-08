//
// Created by ackav on 08-04-2020.
//

#ifndef LIQUIDCORE_MASTER_COMMON_H
#define LIQUIDCORE_MASTER_COMMON_H
#include "v8.h"
#include "libplatform/libplatform.h"


using namespace v8;


#define V8_CONTEXT_SCOPE \
    v8::Isolate::Scope isolate_scope(_isolate);\
    HandleScope scope(_isolate);

typedef char* XString;

/**
   Everything is sent as a pointer to Persistent object, reason is, JavaScript engine should
   not destroy it till it is explicitly destroyed by host application.
*/
typedef Global<Value>* V8Handle;

extern "C" { ;

XString V8StringToXString(Local<Context> context, Local<v8::String> text);

typedef XString(*StringAllocator)(int length);

typedef void (*FreeMemory)(void *location);

typedef void (*LoggerCallback)(XString text);

}


#endif //LIQUIDCORE_MASTER_COMMON_H
