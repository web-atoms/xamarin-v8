//
// Created by ackav on 08-04-2020.
//
#include "V8Response.h"
#include "V8Context.h"

V8Response V8Response_From(Local<Context> context, Local<Value> handle)
{
    V8Response v = {};
    v.type = V8ResponseType::Handle;

    Isolate* isolate = context->GetIsolate();

    if (handle.IsEmpty()) {
        return V8Response_FromError(context, v8::String::NewFromUtf8(isolate, "Unexpected empty value"));
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
        v.result.handle.value.boolValue = (uint8_t)handle->BooleanValue(context).ToChecked();
    } else if (handle->IsInt32()) {
        v.result.handle.handleType = V8HandleType::Integer;
        v.result.handle.value.intValue = handle->Int32Value(context).ToChecked();
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
    else if (handle->IsSymbol()) {
        v.result.handle.handleType = V8HandleType::TypeSymbol;
    }
    else if (handle->IsExternal()) {
        v.result.handle.handleType = V8HandleType::Wrapped;
        Local<v8::External> e = handle.As<v8::External>();
        v.result.handle.value.refValue = e->Value();
    }
    else if (handle->IsObject()) {
        v.result.handle.handleType = V8HandleType::Object;
    }

    V8Handle h = new Global<Value>();
    v.result.handle.handle = h;
    h->Reset(isolate, handle);
    return v;
}

V8Response V8Response_FromError(Local<Context> context, const char* text) {
    MaybeLocal<v8::String> t = v8::String::NewFromUtf8(context->GetIsolate(), text, String::NewStringType::kNormalString);
    return V8Response_FromError(context, t.ToLocalChecked());
}

V8Response V8Response_FromWrappedFunction(Local<Context> context, Local<v8::Function> handle) {
    V8Response v = {};
    v.type = V8ResponseType::Handle;
    V8Handle h = new Global<Value>();
    h->Reset(context->GetIsolate(), handle);
    v.result.handle.handleType = V8HandleType::WrappedFunction;
    v.result.handle.handle = h;
    return v;
}

V8Response V8Response_FromWrappedObject(Local<Context> context, Local<External> handle) {
    V8Response v = {};
    v.type = V8ResponseType::Handle;
    V8Handle h = new Global<Value>();
    h->Reset(context->GetIsolate(), handle);
    v.result.handle.handleType = V8HandleType::Wrapped;
    v.result.handle.handle = h;
    v.result.handle.value.refValue = handle->Value();
    return v;
}



V8Response V8Response_FromError(Local<Context> context, Local<Value> error) {
    V8Response v = V8Response();
    v.type = V8ResponseType::Error;
    Isolate* isolate = context->GetIsolate();
    MaybeLocal<v8::Object> obj = error->ToObject(context);
    Local<v8::Object> local = obj.ToLocalChecked();
    Local<v8::Name> name = v8::String::NewFromUtf8(isolate, "stack", NewStringType::kNormal)
            .ToLocalChecked();
    if (local->HasOwnProperty(context, name).ToChecked()) {
        Local<v8::Value> stack = local->Get(context, name).ToLocalChecked();
        Local<v8::String> stackString = stack->ToString(context).ToLocalChecked();
        v.result.error.stack = V8StringToXString(context, stackString);
    }
    else {
        v.result.error.stack = nullptr;
    }
    Local<v8::String> msg = local->ToString(context).ToLocalChecked();
    v.result.error.message = V8StringToXString(context, msg);
    return v;
}

V8Response V8Response_ToString(Local<Context> context, Local<Value> value) {
    V8Response v = V8Response();
    v.type = V8ResponseType::StringValue;
    Local<v8::String> s = value->ToString(context).ToLocalChecked();
    v.result.stringValue = V8StringToXString(context, s);
    return v;
}

V8Response V8Response_FromBoolean(Local<Context> context, bool value) {
    V8Response v = {};
    v.type = V8ResponseType::BooleanValue;
    v.result.booleanValue = value;
    return v;
}

V8Response V8Response_FromInteger(Local<Context> context, int value) {
    V8Response v = {};
    v.type = V8ResponseType::IntegerValue;
    v.result.intValue = value;
    return v;
}
