//
// Created by ackav on 08-04-2020.
//
#include "V8Response.h"
#include "V8Context.h"

V8Response V8Response::From(Local<Context> context, Local<Value> handle)
{
    V8Response v = V8Response();
    v.type = V8ResponseType::Handle;

    Isolate* isolate = context->GetIsolate();

    if (handle.IsEmpty()) {
        return FromError(context, v8::String::NewFromUtf8(isolate, "Unexpected empty value"));
    }


    // for handle, we need to set the type..
    if (handle->IsUndefined()) {
        v.result.handle.handleType = V8HandleType::Undefined;
    }
    else if (handle->IsNull()) {
        v.result.handle.handleType = V8HandleType::Null;
    }
    else if (handle->IsString() || handle->IsStringObject()) {
        v.result.handle.handleType = V8HandleType::String;
    }
    else if (handle->IsBoolean() || handle->IsBooleanObject()) {
        v.result.handle.handleType = V8HandleType::Boolean;
        v.result.handle.value.boolValue = handle->BooleanValue(context).ToChecked();
    }
    else if (handle->IsNumber() || handle->IsNumberObject()) {
        double d;
        if (handle->NumberValue(context).To(&d)) {
            v.result.handle.handleType = V8HandleType::Number;
            v.result.handle.value.doubleValue = d;
        }
        else {
            v.result.handle.handleType = V8HandleType::NotANumber;
        }
    }
    else if (handle->IsDate()) {
        v.result.handle.handleType = V8HandleType::Date;
        v.result.handle.value.doubleValue = handle->ToObject(context).ToLocalChecked().As<v8::Date>()->ValueOf();
    }
    else if (handle->IsArray()
             || handle->IsArgumentsObject()
             || handle->IsBigInt64Array()) {
        v.result.handle.handleType = V8HandleType::Array;
    }
    else if (handle->IsObject()) {
        v.result.handle.handleType = V8HandleType::Object;
    }

    v.result.handle.handle = new Global<Value>();
    v.result.handle.handle->Reset(isolate, handle);
    return v;
}

V8Response V8Response::FromError(Local<Context> context, Local<Value> error) {
    V8Response v = V8Response();
    v.type = V8ResponseType::Error;
    Isolate* isolate = context->GetIsolate();
    MaybeLocal<v8::Object> obj = error->ToObject(context);
    Local<v8::Object> local = obj.ToLocalChecked();
    Local<v8::Name> name = v8::String::NewFromUtf8(isolate, "stack");
    if (local->HasOwnProperty(context, name).ToChecked()) {
        Local<v8::Value> stack = local->Get(context, name).ToLocalChecked();
        v.result.error.stack = V8StringToXString(context, stack.As<v8::String>());
    }
    else {
        v.result.error.stack = nullptr;
    }
    Local<v8::String> msg = local->ToString(context).ToLocalChecked();
    v.result.error.message = V8StringToXString(context, msg);
    return v;
}

V8Response V8Response::ToString(Local<Context> context, Local<Value> value) {
    V8Response v = V8Response();
    v.type = V8ResponseType::StringValue;
    Local<v8::String> s = value->ToString(context).ToLocalChecked();
    v.result.stringValue = V8StringToXString(context, s);
    return v;
}

