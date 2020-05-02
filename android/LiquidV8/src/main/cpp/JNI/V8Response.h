//
// Created by ackav on 08-04-2020.
//

#ifndef LIQUIDCORE_MASTER_V8RESPONSE_H
#define LIQUIDCORE_MASTER_V8RESPONSE_H

#include "common.h"

//enum V8ResponseType : uint8_t {
//    Error = 0,
//    ConstError = 1,
//    Handle = 2,
//    StringValue = 3,
//    ConstStringValue = 4,
//    BooleanValue = 5,
//    IntegerValue = 6,
//    ArrayValue = 7
//};

enum V8ResponseType : uint8_t {
    None = 0,
    Undefined = 1,
    Null = 2,
    Boolean = 3,
    Number = 8,
    NotANumber = 9,
    Integer = 10,
    BigInt = 11,
    String = 0xFF,
    Object = 0xF0,
    Function = 0xF1,
    Array = 0xF2,
    Date = 0xF3,
    Wrapped = 0xF4,
    WrappedFunction = 0xF5,
    TypeSymbol = 0xF6,

    // these are array pointers...
    // intValue contains length

    CharArray = 0x12,
    ConstCharArray = 0x13,
    Error= 0x14,
    ConstError = 0x15,

    ResponseArray = 0x16
};

typedef union {
    uint8_t boolValue;
    int32_t intValue;
    int64_t longValue;
    double doubleValue;
    void* refValue;
} V8Value;

extern "C" {

/*
When a call is made from outside, response will indicate success/failure
and it will contain the value. In case of string, the response must be
disposed by the caller by calling V8Context_Release method.
*/
    struct V8Response {
        int type;
        int length;
        ClrPointer address;
        union {
            double doubleValue;
            int64_t longValue;
            int32_t intValue;
            uint8_t booleanValue;
            ClrPointer refValue;
        } result;
    };

}
V8Response V8Response_From(Local<Context> &context, Local<Value> &handle);

// V8Response V8Response_FromWrappedObject(Local<Context> context, Local<v8::External> handle);

V8Response V8Response_FromError(const uint16_t * text);

V8Response V8Response_FromErrorWithStack(const char* text, const char* stack);

// V8Response V8Response_FromError(Local<Context> context, Local<Value> error);

// V8Response V8Response_ToString(Isolate* isolate, Local<v8::String> &text);

V8Response V8Response_FromBoolean(bool value);

V8Response V8Response_FromInteger(int value);

#endif //LIQUIDCORE_MASTER_V8RESPONSE_H
