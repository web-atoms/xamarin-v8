//
// Created by ackav on 08-04-2020.
//

#ifndef LIQUIDCORE_MASTER_V8RESPONSE_H
#define LIQUIDCORE_MASTER_V8RESPONSE_H

#include "common.h"

enum V8ResponseType : int16_t {
    Error = 0,
    Handle = 1,
    StringValue = 2
};

enum V8HandleType : int16_t {
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
} V8Value;

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
            V8HandleType handleType;
            V8Handle handle;
            V8Value value;
        } handle;
        struct {
            XString message;
            XString stack;
        } error;
        XString stringValue;
        long longValue;
    } result;
};

static V8Response V8Response_From(Local<Context> context, Local<Value> handle);

static V8Response V8Response_FromError(Local<Context> context, Local<Value> error);

static V8Response V8Response_ToString(Local<Context> context, Local<Value> error);

#endif //LIQUIDCORE_MASTER_V8RESPONSE_H
