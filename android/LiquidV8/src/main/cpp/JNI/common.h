//
// Created by ackav on 08-04-2020.
//

#ifndef LIQUIDCORE_MASTER_COMMON_H
#define LIQUIDCORE_MASTER_COMMON_H
#include "v8.h"
#include <android/log.h>
#include "libplatform/libplatform.h"


using namespace v8;

#define WRAPPED_CLASS 0xA0A

#define TO_CHECKED(n)   Checked(__FILE__, __LINE__, n)

template<typename T>
    static Local<T> Checked(const char * fileName, const int line, MaybeLocal<T> m) {
        if (m.IsEmpty()) {
            __android_log_print(ANDROID_LOG_ERROR, "V8", "MayBeLocal is Empty at %s %d", fileName, line);
        }
        return m.ToLocalChecked();
    }

template<typename T>
static T Checked(const char * fileName, const int line, Maybe<T> m) {
    T value;
    if (!m.To(&value)) {
        __android_log_print(ANDROID_LOG_ERROR, "V8", "MayBeLocal is Empty at %s %d", fileName, line);
    }
    return value;
}

//#define V8_HANDLE_SCOPE \
//    v8::Isolate::Scope isolate_scope(_isolate);\
//    HandleScope scope(_isolate); \
//    Local<Context> context = GetContext(); \
//    Context::Scope context_scope(context); \

#define V8_HANDLE_SCOPE \
    HandleScope scope(_isolate); \
    Local<Context> context = GetContext();

//#define V8_CONTEXT_SCOPE \
//    v8::Isolate::Scope isolate_scope(_isolate);\
//    HandleScope scope(_isolate); \
//    Local<Context> context = GetContext(); \
//    Context::Scope context_scope(context); \
//    TryCatch tryCatch(_isolate); \


#define V8_CONTEXT_SCOPE \
    HandleScope scope(_isolate); \
    Local<Context> context = GetContext(); \
    TryCatch tryCatch(_isolate); \

#define V8_STRING(s) \
    TO_CHECKED(v8::String::NewFromUtf8(_isolate, s, v8::NewStringType::kNormal))

#define V8_STRING16(s, l) \
    TO_CHECKED(v8::String::NewFromTwoByte(_isolate, s, NewStringType::kNormal, l))


#define V8_UTF16STRING(s) \
    TO_CHECKED(v8::String::NewFromTwoByte(_isolate, s->Value, NewStringType::kNormal, s->Length))


typedef char* XString;

typedef const uint16_t* X16String;

typedef const uint8_t* X8String;

/**
   Everything is sent as a pointer to Persistent object, reason is, JavaScript engine should
   not destroy it till it is explicitly destroyed by host application.
*/
typedef Global<Value>* V8Handle;

typedef void* ClrPointer;

#define TO_HANDLE(n) static_cast<V8Handle>(n)

char* CopyString(const char* msg) ;

extern "C" { ;

struct __Utf16Value {
    const uint16_t* Value;
    int Length;
};

typedef __Utf16Value* Utf16Value;

typedef void (*FreeMemory)(void *location);

typedef void (*LoggerCallback)(XString text);

typedef void* (*AllocateMemory) (int length);

typedef __Utf16Value (*ReadDebugMessage)();

typedef void (*SendDebugMessage)(int len, X8String text, X16String text16);

typedef void (*QueueTask)(void* task, double delay);


enum NullableBool: int8_t {
    NotSet = 0,
    False = 1,
    True = 2
};
}

#endif //LIQUIDCORE_MASTER_COMMON_H
