//
// Created by ackav on 08-04-2020.
//
#include "V8Response.h"
#include "V8Context.h"



V8Response V8Response_FromBoolean(bool value) {
    V8Response v = {};
    v.type = V8ResponseType::Boolean;
    v.result.booleanValue = value;
    return v;
}

V8Response V8Response_FromInteger(int value) {
    V8Response v = {};
    v.type = V8ResponseType::Integer;
    v.result.intValue = value;
    return v;
}
