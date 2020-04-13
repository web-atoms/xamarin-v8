//
// Created by ackav on 08-04-2020.
//

#ifndef LIQUIDCORE_MASTER_COMMON_H
#define LIQUIDCORE_MASTER_COMMON_H
#include "v8.h"
#include "libplatform/libplatform.h"


using namespace v8;

#define WRAPPED_CLASS 0xA0A


#define V8_HANDLE_SCOPE \
    v8::Isolate::Scope isolate_scope(_isolate);\
    HandleScope scope(_isolate);


#define V8_CONTEXT_SCOPE \
    v8::Isolate::Scope isolate_scope(_isolate);\
    HandleScope scope(_isolate); \
    Local<Context> context = GetContext(); \
    Context::Scope context_scope(context); \
    TryCatch tryCatch(_isolate); \

#define V8_STRING(s) \
    v8::String::NewFromUtf8(_isolate, s, v8::NewStringType::kNormal).ToLocalChecked()


typedef char* XString;

/**
   Everything is sent as a pointer to Persistent object, reason is, JavaScript engine should
   not destroy it till it is explicitly destroyed by host application.
*/
// typedef Global<Value> V8Handle;
class V8Handle {
private:
    Global<Value> value;
public:
    Local<Value> Get(Isolate* isolate) {
        return value.Get(isolate);
    }

    void Reset() {
        value.Reset();
    }

    void Reset(Isolate* isolate, Local<Value> t) {
        value.Reset(isolate, t);
    }

    bool IsEmpty() {
        return value.IsEmpty();
    }

    bool IsNearDeath() {
        return value.IsNearDeath();
    }
};

char* CopyString(const char* msg) ;



#define V8_FREE_HANDLE(h)                           \
    delete h                                        \


extern "C" { ;

XString V8StringToXString(Local<Context> context, Local<v8::String> text);

typedef XString(*StringAllocator)(int length);

typedef void (*FreeMemory)(void *location);

typedef void (*LoggerCallback)(XString text);

enum NullableBool: int8_t {
    NotSet = 0,
    False = 1,
    True = 2
};

}


#endif //LIQUIDCORE_MASTER_COMMON_H
