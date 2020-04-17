//
// Created by ackav on 08-04-2020.
//

#include <android/log.h>
#include "V8Context.h"
#include "V8Response.h"
#include "V8External.h"
#include "InspectorChannel.h"
#include "V8Hack.h"


static bool _V8Initialized = false;

static ExternalCall externalCall;
static FreeMemory  freeMemory;
static TV8Platform* _platform;
static FatalErrorCallback fatalErrorCallback;
void LogAndroid(const char* location, const char* message) {
    __android_log_print(ANDROID_LOG_ERROR, "V8", "%s %s", location, message);
    // fatalErrorCallback(CopyString(location), CopyString(message));
}

V8Context::V8Context(
        bool debug,
        LoggerCallback loggerCallback,
        ExternalCall  _externalCall,
        FreeMemory _freeMemory,
        FatalErrorCallback errorCallback,
        ReadDebugMessage readDebugMessage,
        LoggerCallback sendDebugMessage,
        QueueTask queueTask) {
    if (!_V8Initialized) // (the API changed: https://groups.google.com/forum/#!topic/v8-users/wjMwflJkfso)
    {
        fatalErrorCallback = errorCallback;
        V8::InitializeICU();

        //?v8::V8::InitializeExternalStartupData(PLATFORM_TARGET "\\");
        // (Startup data is not included by default anymore)

        // _platform = platform::NewDefaultPlatform();
        _platform = new TV8Platform(queueTask);
        // _platform = std::make_unique<TV8Platform>(p1);

        V8::InitializePlatform(_platform);

        V8::Initialize();
        externalCall = _externalCall;
        freeMemory = _freeMemory;
        _V8Initialized = true;
    }
    _logger = loggerCallback;

    Isolate::CreateParams params;
    _arrayBufferAllocator = ArrayBuffer::Allocator::NewDefaultAllocator();
    params.array_buffer_allocator = _arrayBufferAllocator;

    _isolate = Isolate::New(params);
    _isolate->Enter();

    V8_HANDLE_SCOPE

    _isolate->SetFatalErrorHandler(&LogAndroid);


    // _isolate->SetMicrotasksPolicy(MicrotasksPolicy::kScoped);

    _isolate->SetCaptureStackTraceForUncaughtExceptions(true, 10, v8::StackTrace::kOverview);

    Local<v8::ObjectTemplate> global = ObjectTemplate::New(_isolate);
    Local<v8::Context> c = Context::New(_isolate, nullptr, global);
    c->Enter();
    _context.Reset(_isolate, c);

    Local<v8::Symbol> s = v8::Symbol::New(_isolate, V8_STRING("WrappedInstance"));
    _wrapSymbol.Reset(_isolate, s);

    // store wrap symbol at 0
    // _isolate->SetData(0, &_wrapSymbol);

    _isolate->SetData(0, this);

    if (debug) {
        inspectorClient = new XV8InspectorClient(
                c,
                true,
                _platform,
                readDebugMessage,
                sendDebugMessage);
    }


}

class V8WrappedVisitor: public PersistentHandleVisitor {
public:

    V8Context* context;
    bool force;

    // to do delete...
    virtual void VisitPersistentHandle(Persistent<Value>* value,
                                       uint16_t class_id) {

        if (!force) {
            if (!value->IsNearDeath())
                return;
        }
        context->FreeWrapper((Global<Value>*)value, force);
    }
};

V8Response V8Context::GC() {

    V8_CONTEXT_SCOPE

    V8WrappedVisitor v;
    v.context = this;
    _isolate->VisitHandlesWithClassIds(&v);
    v.context = nullptr;
    V8Response r = {};
    return r;
}

void V8Context::FreeWrapper(V8Handle value, bool force) {
    V8_CONTEXT_SCOPE
    LogAndroid("FreeWrapper", "Begin");
    auto i = reinterpret_cast<std::uintptr_t>(value);
    __android_log_print(ANDROID_LOG_ERROR, "V8", "Inspecting Pointer %d", i);
    Local<Value> v = value->Get(_isolate);
    LogAndroid("FreeWrapper", "Check Empty");
    if (v.IsEmpty())
        return;
    LogAndroid("FreeWrapper", "Check External");
    if (!v->IsExternal()) {
        if (force) {
            delete value;
        }
        LogAndroid("FreeWrapper", "Not External");
        return;
    }
    LogAndroid("FreeWrapper", "Checkout External");
    V8External::CheckoutExternal(context, v, force);
    LogAndroid("FreeWrapper", "Checkout External Done");
}

void V8Context::Dispose() {

    if (inspectorClient != nullptr) {
        delete inspectorClient;
    }

    V8WrappedVisitor v;
    v.context = this;
    v.force = true;
    _isolate->VisitHandlesWithClassIds(&v);
    v.context = nullptr;

    _isolate->Dispose();
    // delete _Isolate;
    delete _arrayBufferAllocator;

}

V8Response V8Context::CreateObject() {
    V8_CONTEXT_SCOPE
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

V8Response V8Context::CreateSymbol(XString name) {
    V8_HANDLE_SCOPE
    Local<Symbol> symbol = Symbol::New(_isolate, V8_STRING(name));
    return V8Response_From(GetContext(), symbol);
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
        return V8Response_FromError("Target is not an object");
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

    return V8Response_FromBoolean(true);
}

//class V8Wrapper {
//private:
//    V8Context* _context;
//    Global<Value>* _value;
//public:
//
//    V8Wrapper(V8Context* context, Global<Value>* value) {
//        _context = context;
//        _value = value;
//        _value->SetWrapperClassId(WRAPPED_CLASS);
//        // _value->SetWeak(this, V8Wrapper::WeakCallback, WeakCallbackType::kInternalFields);
//    }
//
//    static void WeakCallback(const WeakCallbackInfo<V8Wrapper> &data) {
//        V8Wrapper* d = data.GetParameter();
//        Isolate* _isolate = data.GetIsolate();
//        d->_context->FreeWrapper(d->_value);
//        delete d;
//    }
//};


V8Response V8Context::Wrap(void *value) {
    V8_CONTEXT_SCOPE

    Local<v8::Value> external = V8External::Wrap(context, value);

    V8Response r = {};
    r.type = V8ResponseType::Handle;
    V8Handle h = new Global<Value>();
    h->SetWrapperClassId(WRAPPED_CLASS);
    h->Reset(_isolate, external);
    r.result.handle.handle = h;
    r.result.handle.handleType = V8HandleType::Wrapped;
    r.result.handle.value.refValue = value;
    return r;
}

void X8Call(const FunctionCallbackInfo<v8::Value> &args) {
    Isolate* isolate = args.GetIsolate();
    Isolate* _isolate = isolate;
    Local<Context> context(isolate->GetCurrentContext());
    Context::Scope context_scope(context);
    Local<Value> data = args.Data();

    HandleScope scope(isolate);
    uint32_t n = (uint)args.Length();
    Local<v8::Array> a = v8::Array::New(isolate, n);
    for (uint32_t i = 0; i < n; i++) {
        a->Set(context, i, args[i]).ToChecked();
    }
    V8Response target = V8Response_From(context, args.This());
    V8Response handleArgs = V8Response_From(context, a);
    V8Response fx = V8Response_From(context, data);
    V8Response r = externalCall(fx, target, handleArgs);

    if (r.type == V8ResponseType::Error) {
        Local<v8::String> error = V8_STRING(r.result.error.message);
        delete r.result.error.message;
        Local<Value> ex = Exception::Error(error);
        isolate->ThrowException(ex);
    } else {
        if (r.result.handle.handle != nullptr) {
            Local<Value> rx = r.result.handle.handle->Get(isolate);
            V8_FREE_HANDLE(r.result.handle.handle);
            args.GetReturnValue().Set(rx);
        }
    }
}

V8Response V8Context::CreateFunction(ExternalCall function, XString debugHelper) {
    V8_CONTEXT_SCOPE
    Local<Value> e = V8External::Wrap(context, (void*)function);
    // Local<External> e = External::New(_isolate, (void*)function);

    Local<v8::Function> f = v8::Function::New(context, X8Call, e).ToLocalChecked();
    Local<v8::String> n = V8_STRING(debugHelper);
    f->SetName(n);

    return V8Response_From(context, f);
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
    return V8Response_From(context, result);
}


V8Response V8Context::Release(V8Handle handle) {
    try {
        LogAndroid("Release", "Begin");
        FreeWrapper(handle, false);
        LogAndroid("Release", "Delete");
        delete handle;
        LogAndroid("Release", "Delete Done");
        V8Response r = {};
        r.type = V8ResponseType ::BooleanValue;
        r.result.booleanValue = true;
        return r;
    } catch (std::exception const &ex) {
        return V8Response_FromError(ex.what());
    }
}

V8Response V8Context::InvokeMethod(V8Handle target, XString name, int len, V8Handle* args) {

    V8_CONTEXT_SCOPE
    Local<Value> targetValue = target->Get(_isolate);
    if (!targetValue->IsObject()) {
        return V8Response_FromError("Target is not an Object");
    }
    Local<v8::String> jsName = V8_STRING(name);

    Local<v8::Object> fxObj = targetValue->ToObject(context).ToLocalChecked();
    Local<v8::Value> fxValue;
    if(!fxObj->Get(context, jsName).ToLocal(&fxValue)) {
        return V8Response_FromError("Method does not exist");
    }
    Local<v8::Function> fx = Local<v8::Function>::Cast(fxValue);

    Local<v8::Value>* argList = new Local<v8::Value> [len];
    for (int i = 0; i < len; ++i) {
        V8Handle h = args[i];
        argList[0] = h->Get(_isolate);
    }
    Local<Value> result;
    if(!fx->Call(context, fxObj, len, argList).ToLocal(&result)) {
        delete[] argList;
        return V8Response_FromError(context, tryCatch.Exception());
    }
    delete[] argList;
    return V8Response_From(context, result);
}

V8Response V8Context::InvokeFunction(V8Handle target, V8Handle thisValue, int len, V8Handle* args) {
    V8_CONTEXT_SCOPE
    Local<Value> targetValue = target->Get(_isolate);
    if (!targetValue->IsFunction()) {
        return V8Response_FromError("Target is not a function");
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
        delete[] argList;
        return V8Response_FromError(context, tryCatch.Exception());
    }
    delete[] argList;
    return V8Response_From(context, result);
}

V8Response V8Context::GetArrayLength(V8Handle target) {
    V8_HANDLE_SCOPE
    Local<Context> context = GetContext();
    Local<Value> value = target->Get(_isolate);
    if (!value->IsArray()) {
        return V8Response_FromError("Target is not an array");
    }
    Local<v8::Object> jsObj = value->ToObject(context).ToLocalChecked();
    Local<v8::Array> array = Local<v8::Array>::Cast(jsObj);
    return V8Response_FromInteger(array->Length());
}

V8Response V8Context::GetGlobal() {
    V8_HANDLE_SCOPE
    Local<Context> context = GetContext();
    Local<v8::Object> g = context->Global();
    Local<v8::String> key = V8_STRING("global");
    if (!g->HasOwnProperty(context, key).ToChecked()) {
        if(!g->Set(context, key, g).ToChecked()) {
            return V8Response_FromError("Failed to create global reference !!");
        }
    }
    return V8Response_From(context, context->Global());
}

V8Response V8Context::NewInstance(V8Handle target, int len, V8Handle *args) {
    V8_CONTEXT_SCOPE
    Local<Value> targetValue = target->Get(_isolate);
    if (!targetValue->IsFunction()) {
        return V8Response_FromError("Target is not a function");
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
        delete[] argList;
        return V8Response_FromError(context, tryCatch.Exception());
    }
    delete[] argList;
    return V8Response_From(context, result);
}

V8Response V8Context::Has(V8Handle target, V8Handle index) {
    V8_HANDLE_SCOPE
    Local<Context> context = GetContext();
    Local<Value> value = target->Get(_isolate);
    if (!value->IsObject()) {
        return V8Response_FromError("Target is not an object ");
    }
    Local<v8::Object> obj = value->ToObject(context).ToLocalChecked();
    Local<v8::Name> key = Local<v8::Name>::Cast(index->Get(_isolate));
    return V8Response_FromBoolean(obj->HasOwnProperty(context, key).ToChecked());
}

V8Response V8Context::Get(V8Handle target, V8Handle index) {
    V8_HANDLE_SCOPE
    Local<Value> v = target->Get(_isolate);
    Local<Context> context = GetContext();
    if (!v->IsObject())
        return V8Response_FromError("This is not an object");
    Local<v8::Name> key = Local<v8::Name>::Cast(index->Get(_isolate));
    Local<v8::Object> jsObj = v->ToObject(context).ToLocalChecked();
    Local<Value> item = jsObj->Get(context, key).ToLocalChecked();
    return V8Response_From(context, item);
}

V8Response V8Context::Set(V8Handle target, V8Handle index, V8Handle value) {
    V8_HANDLE_SCOPE
    Local<Value> t = target->Get(_isolate);
    Local<Value> v = value->Get(_isolate);
    Local<Context> context = GetContext();
    if (!t->IsObject())
        return V8Response_FromError("This is not an object");
    Local<v8::Name> key = Local<v8::Name>::Cast(index->Get(_isolate));
    Local<v8::Object> obj = t->ToObject(context).ToLocalChecked();
    obj->Set(context, key, v).ToChecked();
    return V8Response_From(context, v);
}


V8Response V8Context::HasProperty(V8Handle target, XString name) {
    V8_HANDLE_SCOPE
    Local<Context> context = GetContext();
    Local<Value> value = target->Get(_isolate);
    if (!value->IsObject()) {
        return V8Response_FromError("Target is not an object ");
    }
    Local<v8::Object> obj = value->ToObject(context).ToLocalChecked();
    Local<v8::String> key = V8_STRING(name);
    return V8Response_FromBoolean(obj->HasOwnProperty(context, key).ToChecked());
}

V8Response V8Context::GetProperty(V8Handle target, XString name) {
    V8_HANDLE_SCOPE
    Local<Value> v = target->Get(_isolate);
    Local<Context> context = GetContext();
    if (!v->IsObject())
        return V8Response_FromError("This is not an object");
    Local<v8::String> jsName = V8_STRING(name);
    Local<v8::Object> jsObj = v->ToObject(context).ToLocalChecked();
    Local<Value> item = jsObj->Get(context, jsName).ToLocalChecked();
    return V8Response_From(context, item);
}

V8Response V8Context::SetProperty(V8Handle target, XString name, V8Handle value) {
    V8_HANDLE_SCOPE
    Local<Value> t = target->Get(_isolate);
    Local<Value> v = value->Get(_isolate);
    Local<Context> context = GetContext();
    if (!t->IsObject())
        return V8Response_FromError("This is not an object");
    Local<v8::String> jsName = V8_STRING(name);
    Local<v8::Object> obj = t->ToObject(context).ToLocalChecked();
    obj->Set(context, jsName, v).ToChecked();
    return V8Response_From(context, v);
}

V8Response V8Context::GetPropertyAt(V8Handle target, int index) {
    V8_HANDLE_SCOPE
    Local<Value> v = target->Get(_isolate);
    Local<Context> context = GetContext();
    if (!v->IsArray())
        return V8Response_FromError("This is not an array");
    Local<v8::Object> a = v->ToObject(context).ToLocalChecked();
    Local<Value> item = a->Get(context, (uint) index).ToLocalChecked();
    return V8Response_From(context, item);
}

V8Response V8Context::SetPropertyAt(V8Handle target, int index, V8Handle value) {
    V8_HANDLE_SCOPE
    Local<Value> t = target->Get(_isolate);
    Local<Value> v = value->Get(_isolate);
    Local<Context> context = GetContext();
    if (!t->IsArray())
        return V8Response_FromError("This is not an array");
    Local<v8::Object> obj = t->ToObject(context).ToLocalChecked();
    obj->Set(context, (uint)index, v).ToChecked();
    return V8Response_From(context, v);
}

V8Response V8Context::SendDebugMessage(XString msg) {
    V8_CONTEXT_SCOPE
    if (inspectorClient != nullptr) {
        Local<v8::String> message = V8_STRING(msg);
        String::Value buffer(_isolate, message);
        v8_inspector::StringView messageView(*buffer, buffer.length());
        inspectorClient->SendDebugMessage(messageView);
    }
    if (tryCatch.HasCaught()) {
        return V8Response_FromError(context, tryCatch.Exception());
    }
    return V8Response_FromBoolean(true);
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

void V8External::Log(const char *msg) {
    LogAndroid("Log", msg);
}

void V8External::Release(void *data) {
    LogAndroid("V8External", "Release");
    if (data != nullptr) {
        freeMemory(data);
    }
}
