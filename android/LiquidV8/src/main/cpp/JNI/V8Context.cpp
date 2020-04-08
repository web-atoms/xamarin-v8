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

    V8_CONTEXT_SCOPE

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
    V8_CONTEXT_SCOPE
    return V8Response_From(GetContext(), Object::New(_isolate));
}

V8Response V8Context::CreateNumber(double value) {
    V8_CONTEXT_SCOPE
    return V8Response_From(GetContext(), Number::New(_isolate, value));
}

V8Response V8Context::CreateBoolean(bool value) {
    V8_CONTEXT_SCOPE
    return V8Response_From(GetContext(), v8::Boolean::New(_isolate, value));
}

V8Response V8Context::CreateUndefined() {
    V8_CONTEXT_SCOPE
    return V8Response_From(GetContext(), v8::Undefined(_isolate));
}

V8Response V8Context::CreateNull() {
    V8_CONTEXT_SCOPE
    return V8Response_From(GetContext(), v8::Null(_isolate));
}

V8Response V8Context::CreateString(XString value) {
    V8_CONTEXT_SCOPE
    return V8Response_From(GetContext(), v8::String::NewFromUtf8(_isolate, value));
}

V8Response V8Context::CreateDate(int64_t value) {
    V8_CONTEXT_SCOPE
    return V8Response_From(GetContext(), v8::Date::New(GetContext(), (double)value).ToLocalChecked());
}

V8Response V8Context::Wrap(void *value) {
    V8_CONTEXT_SCOPE
    return V8Response_From(GetContext(),v8::External::New(_isolate, value));
}

void X8Call(const FunctionCallbackInfo<v8::Value> &args) {
    Isolate *isolate = args.GetIsolate();
    Local<Context> context(isolate->GetCurrentContext());
    External *b = v8::External::Cast(*args.Data());
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
        Local<v8::String> error = v8::String::NewFromUtf8(isolate, r.result.error.message);
        Local<Value> ex = Exception::Error(error);
        isolate->ThrowException(ex);
        delete r.result.error.message;
    } else if (r.type == V8ResponseType::StringValue) {
        args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, r.result.stringValue));
        delete r.result.stringValue;
    } else {
        Local<Value> rx = r.result.handle.handle->Get(isolate);
        args.GetReturnValue().Set(rx);
        delete r.result.handle.handle;
    }
}

V8Response V8Context::CreateFunction(ExternalCall function, XString debugHelper) {
    V8_CONTEXT_SCOPE
    Local<External> e = External::New(_isolate, (void*)function);
    Local<Context> context(_isolate->GetCurrentContext());
    ;	Local<v8::Function> f = v8::Function::New(context, X8Call, e).ToLocalChecked();
    Local<v8::String> n = v8::String::NewFromUtf8(_isolate, debugHelper);
    f->SetName(n);
    return V8Response_From(GetContext(), f);
}

V8Response V8Context::Evaluate(XString script, XString location) {
    V8_CONTEXT_SCOPE

    TryCatch tryCatch(_isolate);
    Local<Context> context = GetContext();
    Local<v8::String> v8ScriptSrc =
            v8::String::NewFromUtf8(_isolate, script, v8::NewStringType::kNormal)
            .ToLocalChecked();
    Local<v8::String> v8ScriptLocation =
            v8::String::NewFromUtf8(_isolate, location, v8::NewStringType::kNormal)
            .ToLocalChecked();

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
    return V8Response_From(context, result);
}


void V8Context::Release(V8Handle handle) {
    handle->Reset();
    delete handle;
}

V8Response V8Context::GetProperty(V8Handle target, XString name) {
    V8_CONTEXT_SCOPE
    Local<Value> v = target->Get(_isolate);
    Local<Context> context = GetContext();
    if (!v->IsObject())
        return V8Response_FromError(context, v8::String::NewFromUtf8(_isolate, "This is not an object"));
    Local<v8::String> jsName = v8::String::NewFromUtf8(_isolate, name);
    Local<v8::Object> jsObj = v->ToObject(context).ToLocalChecked();
    Local<Value> item = jsObj->Get(context, jsName).ToLocalChecked();
    return V8Response_From(context, item);
}

V8Response V8Context::GetPropertyAt(V8Handle target, int index) {
    V8_CONTEXT_SCOPE
    Local<Value> v = target->Get(_isolate);
    Local<Context> context = GetContext();
    if (!v->IsArray())
        return V8Response_FromError(context, v8::String::NewFromUtf8(_isolate, "This is not an array"));
    // Local<Value> item = v->ToObject(context).ToLocalChecked()->->Get(context, index).ToLocalChecked();
    Local<v8::Object> a = v->ToObject(context).ToLocalChecked();
    Local<Value> item = a->Get(context, (uint) index).ToLocalChecked();
    return V8Response_From(context, item);
}

V8Response V8Context::SetProperty(V8Handle target, XString name, V8Handle value) {
    V8_CONTEXT_SCOPE
    Local<Value> t = target->Get(_isolate);
    Local<Value> v = value->Get(_isolate);
    Local<Context> context = GetContext();
    if (!t->IsObject())
        return V8Response_FromError(context, v8::String::NewFromUtf8(_isolate, "This is not an object"));
    Local<v8::String> jsName = v8::String::NewFromUtf8(_isolate, name);
    Local<v8::Object> obj = t->ToObject(context).ToLocalChecked();
    obj->Set(context, jsName, v).ToChecked();
    return V8Response_From(context, v);
}

V8Response V8Context::SetPropertyAt(V8Handle target, int index, V8Handle value) {
    V8_CONTEXT_SCOPE
    Local<Value> t = target->Get(_isolate);
    Local<Value> v = value->Get(_isolate);
    Local<Context> context = GetContext();
    if (!t->IsArray())
        return V8Response_FromError(context, v8::String::NewFromUtf8(_isolate, "This is not an array"));
    Local<v8::Object> obj = t->ToObject(context).ToLocalChecked();
    obj->Set(context, (uint)index, v).ToChecked();
    return V8Response_From(context, v);
}