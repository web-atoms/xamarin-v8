#pragma once
#include "v8.h"
using namespace v8;

typedef char* XString;

/**
   Everything is sent as a pointer to Persistent object, reason is, JavaScript engine should
   not destroy it till it is explicitly destroyed by host application.
*/
typedef Persistent<Value, CopyablePersistentTraits<Value>>* V8Handle;

enum V8ResponseType: int16_t {
	Error = 0,
	Handle = 1,
	String = 2
};

enum V8HandleType: int16_t {
	None = 0,
	Undefined = 1,
	Null = 2,
	Number = 3,
	NotANumber = 4,
	BigInt = 5,
	Boolean = 6,
	String = 0xFF,
	Object = 0xF0,
	Function = 0xF1,
	Array = 0xF2,
	Remote = 0xF3,
	Date = 0xF4
};

typedef union {
	bool boolValue;
	int32_t intValue;
	int64_t longValue;
	double doubleValue;
	XString stringValue;
} V8Value;

XString V8StringToXString(Local<Context> context, Local<v8::String> text);

/*
When a call is made from outside, response will indicate success/failure
and it will contain the value. In case of string, the response must be
disposed by the caller by calling V8Context_Release method.
*/
struct V8Response
{
public:
	V8ResponseType type;
	union {
		struct {
			V8Handle handle;
			V8HandleType handleType;
			V8Value value;
		} handle;
		struct {
			XString message;
			XString stack;
		} error;
		XString stringValue;
		long longValue;
	} result;

	static V8Response From(Local<Context> context, Local<Value> handle);

	static V8Response FromError(Local<Context> context, Local<Value> error);

	static V8Response ToString(Local<Context> context, Local<Value> error);
};

