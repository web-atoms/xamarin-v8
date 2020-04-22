//
// Created by ackav on 08-04-2020.
//
#include "V8Response.h"
#include "V8Context.h"
#include "V8External.h"

V8Response V8Response_From(Local<Context> &context, Local<Value> &handle)
{
    V8Response v = {};
    v.type = V8ResponseType::Handle;

    Isolate* isolate = context->GetIsolate();
    HandleScope hs(isolate);

    if (handle.IsEmpty()) {
        return V8Response_FromError("Unexpected empty value");
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
        v.result.handle.value.boolValue = (uint8_t)handle->BooleanValue(isolate);
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
        V8External* e = V8External::CheckInExternal(context, handle);
        if (e != nullptr) {
            v.result.handle.value.refValue = e->Data();
        }
    }
    else if (handle->IsObject()) {
        v.result.handle.handleType = V8HandleType::Wrapped;
        V8External* e = V8External::CheckInExternal(context, handle);
        if (e != nullptr) {
            v.result.handle.value.refValue = e->Data();
        }
        v.result.handle.handleType = V8HandleType::Object;
    }

    V8Handle h = new Global<Value>();
    h->SetWrapperClassId(WRAPPED_CLASS);
    v.result.handle.handle = (void*)h;
    h->Reset(isolate, handle);
    return v;
}

V8Response V8Response_FromError(const char* text) {
    V8Response r = {};
    r.type = V8ResponseType ::Error;
    r.result.error.message = CopyString(text);
    return r;
}

//V8Response V8Response_FromWrappedObject(Local<Context> context, Local<External> handle) {
//    V8Response v = {};
//    v.type = V8ResponseType::Handle;
//    V8Handle h = new Global<Value>();
//    h->Reset(context->GetIsolate(), handle);
//    v.result.handle.handleType = V8HandleType::Wrapped;
//    v.result.handle.handle = h;
//    v.result.handle.value.refValue = handle->Value();
//    return v;
//}



//V8Response V8Response_FromError(Local<Context> context, Local<Value> error) {
//    V8Response v = V8Response();
//    v.type = V8ResponseType::Error;
//    Isolate* isolate = context->GetIsolate();
//    MaybeLocal<v8::Object> obj = error->ToObject(context);
//    Local<v8::Object> local = obj.ToLocalChecked();
//    Local<v8::Name> name = v8::String::NewFromUtf8(isolate, "stack", NewStringType::kNormal)
//            .ToLocalChecked();
//    if (local->HasOwnProperty(context, name).ToChecked()) {
//        Local<v8::Value> stack = local->Get(context, name).ToLocalChecked();
//        Local<v8::String> stackString = stack->ToString(context).ToLocalChecked();
//        v.result.error.stack = V8StringToXString(isolate, stackString);
//    }
//    else {
//        v.result.error.stack = nullptr;
//    }
//    Local<v8::String> msg = local->ToString(context).ToLocalChecked();
//    v.result.error.message = V8StringToXString(isolate, msg);
//    return v;
//}

V8Response V8Response_ToString(XString text) {
    V8Response v = V8Response();
    v.type = V8ResponseType::StringValue;
    // Local<v8::String> s = value->ToString(context).ToLocalChecked();
    v.result.stringValue = text;
    return v;
}

V8Response V8Response_FromBoolean(bool value) {
    V8Response v = {};
    v.type = V8ResponseType::BooleanValue;
    v.result.booleanValue = value;
    return v;
}

V8Response V8Response_FromInteger(int value) {
    V8Response v = {};
    v.type = V8ResponseType::IntegerValue;
    v.result.intValue = value;
    return v;
}
