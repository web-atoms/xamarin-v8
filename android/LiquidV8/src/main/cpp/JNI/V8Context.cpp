//
// Created by ackav on 08-04-2020.
//

#include "V8Context.h"
#include "V8Response.h"

static bool _V8Initialized = false;

V8Context::V8Context(bool debug, LoggerCallback loggerCallback) {
    if (!_V8Initialized) // (the API changed: https://groups.google.com/forum/#!topic/v8-users/wjMwflJkfso)
    {
        V8::InitializeICU();

        //?v8::V8::InitializeExternalStartupData(PLATFORM_TARGET "\\");
        // (Startup data is not included by default anymore)

        _platform = platform::NewDefaultPlatform();
        V8::InitializePlatform(_platform.get());

        V8::Initialize();

        _V8Initialized = true;
    }
    _logger = loggerCallback;
    Isolate::CreateParams params;
    _arrayBufferAllocator = ArrayBuffer::Allocator::NewDefaultAllocator();
    params.array_buffer_allocator = _arrayBufferAllocator;

    _isolate = Isolate::New(params);

    V8_HANDLE_SCOPE

    Local<v8::ObjectTemplate> global = ObjectTemplate::New(_isolate);
    Local<v8::Context> c = Context::New(_isolate, nullptr, global);
    _context.Reset(_isolate, c);


}

void V8Context::Dispose() {

    _context.Reset();

    _isolate->Dispose();
    // delete _Isolate;
    delete _arrayBufferAllocator;

}

V8Response V8Context::CreateObject() {
    V8_HANDLE_SCOPE
    return V8Response_From(GetContext(), Object::New(_isolate));
}

V8Response V8Context::CreateNumber(double value) {
    V8_HANDLE_SCOPE
    return V8Response_From(GetContext(), Number::New(_isolate, value));
}

V8Response V8Context::CreateBoolean(bool value) {
    V8_HANDLE_SCOPE
    return V8Response_From(GetContext(), v8::Boolean::New(_isolate, value));
}

V8Response V8Context::CreateUndefined() {
    V8_HANDLE_SCOPE
    return V8Response_From(GetContext(), v8::Undefined(_isolate));
}

V8Response V8Context::CreateNull() {
    V8_HANDLE_SCOPE
    return V8Response_From(GetContext(), v8::Null(_isolate));
}

V8Response V8Context::CreateString(XString value) {
    V8_HANDLE_SCOPE
    return V8Response_From(GetContext(), V8_STRING(value));
}

V8Response V8Context::CreateDate(int64_t value) {
    V8_HANDLE_SCOPE
    return V8Response_From(GetContext(), v8::Date::New(GetContext(), (double)value).ToLocalChecked());
}

V8Response V8Context::DefineProperty(
        V8Handle target,
        XString name,
        NullableBool configurable,
        NullableBool enumerable,
        NullableBool writable,
        V8Handle get,
        V8Handle set,
        V8Handle value
        ) {
    V8_CONTEXT_SCOPE

    Local<Value> t = target->Get(_isolate);
    if (!t->IsObject()) {
        return V8Response_FromError(context, "Target is not an object");
    }
    Local<v8::Object> jsObj = t.As<v8::Object>();
    Local<v8::String> key = V8_STRING(name);

    // PropertyDescriptor pd;

    if (value != nullptr) {
        Local<Value> v = value->Get(_isolate);
        PropertyDescriptor pd(v, writable == NullableBool::True);

        if (configurable != NullableBool::NotSet) {
            pd.set_configurable(configurable == NullableBool::True);
        }
        if (enumerable != NullableBool::NotSet) {
            pd.set_enumerable(enumerable == NullableBool::True);
        }

        if (!jsObj->DefineProperty(context, key, pd).ToChecked()) {
            return V8Response_FromError(context, tryCatch.Exception());
        }

    } else {
        Local<Value> getValue;
        Local<Value> setValue;
        if (get != nullptr) {
            getValue = get->Get(_isolate);
        }
        if (set != nullptr) {
            setValue = set->Get(_isolate);
        }
        PropertyDescriptor pd(getValue, setValue);

        if (configurable != NullableBool::NotSet) {
            pd.set_configurable(configurable == NullableBool::True);
        }
        if (enumerable != NullableBool::NotSet) {
            pd.set_enumerable(enumerable == NullableBool::True);
        }

        if (!jsObj->DefineProperty(context, key, pd).ToChecked()) {
            return V8Response_FromError(context, tryCatch.Exception());
        }
    }

    return V8Response_FromBoolean(context, true);
}

V8Response V8Context::Wrap(void *value) {
    V8_HANDLE_SCOPE

    return V8Response_From(GetContext(),v8::External::New(_isolate, value));
}

void X8Call(const FunctionCallbackInfo<v8::Value> &args) {
    Isolate* isolate = args.GetIsolate();
    Isolate* _isolate = isolate;
    Local<Context> context(isolate->GetCurrentContext());
    Local<v8::External> b = args.Data().As<External>();
    ExternalCall function = (ExternalCall) b->Value();

    HandleScope scope(isolate);
    uint32_t n = args.Length();
    Local<v8::Array> a = v8::Array::New(isolate, n);
    for (uint32_t i = 0; i < n; i++) {
        a->Set(context, i, args[i]).ToChecked();
    }
    V8Response target = V8Response_From(context, args.This());
    V8Response handleArgs = V8Response_From(context, a);
    V8Response r = function(target, handleArgs);

    if (r.type == V8ResponseType::Error) {
        Local<v8::String> error = V8_STRING(r.result.error.message);
        Local<Value> ex = Exception::Error(error);
        isolate->ThrowException(ex);
        delete r.result.error.message;
    } else if (r.type == V8ResponseType::StringValue) {
        args.GetReturnValue().Set(V8_STRING(r.result.stringValue));
        delete r.result.stringValue;
    } else {
        Local<Value> rx = r.result.handle.handle->Get(isolate);
        args.GetReturnValue().Set(rx);
        delete r.result.handle.handle;
    }
}

V8Response V8Context::CreateFunction(ExternalCall function, XString debugHelper) {
    V8_HANDLE_SCOPE
    Local<External> e = External::New(_isolate, (void*)function);
    Local<Context> context(_isolate->GetCurrentContext());
    ;	Local<v8::Function> f = v8::Function::New(context, X8Call, e).ToLocalChecked();
    Local<v8::String> n = V8_STRING(debugHelper);
    f->SetName(n);
    return V8Response_From(GetContext(), f);
}

V8Response V8Context::Evaluate(XString script, XString location) {
    V8_HANDLE_SCOPE

    TryCatch tryCatch(_isolate);
    Local<Context> context = GetContext();
    Local<v8::String> v8ScriptSrc = V8_STRING(script);
    Local<v8::String> v8ScriptLocation = V8_STRING(location);

    Context::Scope context_scope(context);

    ScriptOrigin origin(v8ScriptLocation, v8::Integer::New(_isolate, 0) );

    Local<Script> s;
    if (!Script::Compile(context, v8ScriptSrc, &origin).ToLocal(&s)) {
        return V8Response_FromError(context, tryCatch.Exception());
    }
    Local<Value> result;
    if (!s->Run(context).ToLocal(&result)) {
        return V8Response_FromError(context, tryCatch.Exception());
    }
    Local<v8::String> r = result->ToString(context).ToLocalChecked();
    uint length = (uint)r->Utf8Length(_isolate);
    char* str = (char*)malloc(length);
    r->WriteUtf8(_isolate, str, length, nullptr, 0);
    _logger(str);
    delete str;
    return V8Response_From(context, result);
}


void V8Context::Release(V8Handle handle) {
    handle->Reset();
    delete handle;
}

V8Response V8Context::InvokeFunction(V8Handle target, V8Handle thisValue, int len, V8Handle* args) {
    V8_CONTEXT_SCOPE
    Local<Value> targetValue = target->Get(_isolate);
    if (!targetValue->IsFunction()) {
        return V8Response_FromError(context, "Target is not a function");
    }
    Local<Value> thisValueValue;
    if (thisValue != nullptr) {
        thisValueValue = thisValue->Get(_isolate);
    }
    Local<v8::Object> fxObj = targetValue->ToObject(context).ToLocalChecked();
    Local<v8::Function> fx = Local<v8::Function>::Cast(fxObj);

    Local<v8::Value>* argList = new Local<v8::Value> [len];
    for (int i = 0; i < len; ++i) {
        V8Handle h = args[i];
        argList[0] = h->Get(_isolate);
    }
    Local<Value> result;
    if(!fx->Call(context, thisValueValue, len, argList).ToLocal(&result)) {
        delete argList;
        return V8Response_FromError(context, tryCatch.Exception());
    }
    delete argList;
    return V8Response_From(context, result);
}

V8Response V8Context::GetArrayLength(V8Handle target) {
    V8_HANDLE_SCOPE
    Local<Context> context = GetContext();
    Local<Value> value = target->Get(_isolate);
    if (!value->IsArray()) {
        return V8Response_FromError(context, "Target is not an array");
    }
    Local<v8::Object> jsObj = value->ToObject(context).ToLocalChecked();
    Local<v8::Array> array = Local<v8::Array>::Cast(jsObj);
    return V8Response_FromInteger(context,array->Length());
}

V8Response V8Context::GetGlobal() {
    V8_HANDLE_SCOPE
    Local<Context> context = GetContext();
    Local<v8::Object> g = context->Global();
    Local<v8::String> key = V8_STRING("global");
    if (!g->HasOwnProperty(context, key).ToChecked()) {
        if(!g->Set(context, key, g).ToChecked()) {
            return V8Response_FromError(context, "Failed to create global reference !!");
        }
    }
    return V8Response_From(context, context->Global());
}

V8Response V8Context::NewInstance(V8Handle target, int len, V8Handle *args) {
    V8_CONTEXT_SCOPE
    Local<Value> targetValue = target->Get(_isolate);
    if (!targetValue->IsFunction()) {
        return V8Response_FromError(context, "Target is not a function");
    }
    Local<v8::Object> fxObj = targetValue->ToObject(context).ToLocalChecked();
    Local<v8::Function> fx = Local<v8::Function>::Cast(fxObj);

    Local<v8::Value>* argList = new Local<v8::Value> [len];
    for (int i = 0; i < len; ++i) {
        V8Handle h = args[i];
        argList[0] = h->Get(_isolate);
    }
    Local<Value> result;
    if(!fx->CallAsConstructor(context, len, argList).ToLocal(&result)) {
        delete argList;
        return V8Response_FromError(context, tryCatch.Exception());
    }
    delete argList;
    return V8Response_From(context, result);
}

V8Response V8Context::HasProperty(V8Handle target, XString name) {
    V8_HANDLE_SCOPE
    Local<Context> context = GetContext();
    Local<Value> value = target->Get(_isolate);
    if (!value->IsObject()) {
        return V8Response_FromError(context, "Target is not an object ");
    }
    Local<v8::Object> obj = value->ToObject(context).ToLocalChecked();
    Local<v8::String> key = V8_STRING(name);
    return V8Response_FromBoolean(context, obj->HasOwnProperty(context, key).ToChecked());
}

V8Response V8Context::GetProperty(V8Handle target, XString name) {
    V8_HANDLE_SCOPE
    Local<Value> v = target->Get(_isolate);
    Local<Context> context = GetContext();
    if (!v->IsObject())
        return V8Response_FromError(context, "This is not an object");
    Local<v8::String> jsName = V8_STRING(name);
    Local<v8::Object> jsObj = v->ToObject(context).ToLocalChecked();
    Local<Value> item = jsObj->Get(context, jsName).ToLocalChecked();
    return V8Response_From(context, item);
}

V8Response V8Context::GetPropertyAt(V8Handle target, int index) {
    V8_HANDLE_SCOPE
    Local<Value> v = target->Get(_isolate);
    Local<Context> context = GetContext();
    if (!v->IsArray())
        return V8Response_FromError(context, "This is not an array");
    Local<v8::Object> a = v->ToObject(context).ToLocalChecked();
    Local<Value> item = a->Get(context, (uint) index).ToLocalChecked();
    return V8Response_From(context, item);
}

V8Response V8Context::SetProperty(V8Handle target, XString name, V8Handle value) {
    V8_HANDLE_SCOPE
    Local<Value> t = target->Get(_isolate);
    Local<Value> v = value->Get(_isolate);
    Local<Context> context = GetContext();
    if (!t->IsObject())
        return V8Response_FromError(context, "This is not an object");
    Local<v8::String> jsName = V8_STRING(name);
    Local<v8::Object> obj = t->ToObject(context).ToLocalChecked();
    obj->Set(context, jsName, v).ToChecked();
    return V8Response_From(context, v);
}

V8Response V8Context::SetPropertyAt(V8Handle target, int index, V8Handle value) {
    V8_HANDLE_SCOPE
    Local<Value> t = target->Get(_isolate);
    Local<Value> v = value->Get(_isolate);
    Local<Context> context = GetContext();
    if (!t->IsArray())
        return V8Response_FromError(context, "This is not an array");
    Local<v8::Object> obj = t->ToObject(context).ToLocalChecked();
    obj->Set(context, (uint)index, v).ToChecked();
    return V8Response_From(context, v);
}

V8Response V8Context::ToString(V8Handle target) {
    V8_HANDLE_SCOPE
    Local<Context> context = GetContext();
    Local<Value> value = target->Get(_isolate);
    if (!value->IsString()) {
        // we need to invoke to string of the object...
        Context::Scope context_scope(context);
        value = value->ToString(context).ToLocalChecked();
        return V8Response_ToString(GetContext(), value);
    }
    return V8Response_ToString(GetContext(), value);
}