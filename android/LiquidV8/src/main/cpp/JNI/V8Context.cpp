//
// Created by ackav on 08-04-2020.
//

#include <android/log.h>
#include "V8Context.h"
#include "V8Response.h"
#include "V8External.h"
#include "InspectorChannel.h"
// #include "V8Hack.h"
#include "V8DispatchMessageTask.h"
// #include "V8ReleaseHandleTask.h"

#define RETURN_EXCEPTION(e) \
    Local<v8::String> exToString;  \
    exToString = e.Exception()->ToDetailString(context).ToLocalChecked(); \
    return V8Response_FromError(V8StringToXString(_isolate, exToString));

static bool _V8Initialized = false;

static ExternalCall externalCall;
static FreeMemory  freeMemory;
// static TV8Platform* _platform;
static std::unique_ptr<v8::Platform> sPlatform;
static FatalErrorCallback fatalErrorCallback;
void LogAndroid(const char* location, const char* message) {
    __android_log_print(ANDROID_LOG_ERROR, "V8", "%s %s", location, message);
    // fatalErrorCallback(CopyString(location), CopyString(message));
}

void LogAndroid(const char* location, const char* message, int64_t n) {
    __android_log_print(ANDROID_LOG_ERROR, "V8", "%d %s %s",n, location, message);
    // fatalErrorCallback(CopyString(location), CopyString(message));
}

bool CanAbort(Isolate* isolate) {
    return false;
}

void FatalErrorLogger(Local<Message> message, Local<Value> data) {
    __android_log_print(ANDROID_LOG_ERROR, "V8", "Some Error");
}

V8Context::V8Context(
        bool debug,
        LoggerCallback loggerCallback,
        ExternalCall  _externalCall,
        FreeMemory _freeMemory,
        FatalErrorCallback errorCallback,
        ReadDebugMessage readDebugMessage,
        ::SendDebugMessage sendDebugMessage,
        QueueTask queueTask) {
    if (!_V8Initialized) // (the API changed: https://groups.google.com/forum/#!topic/v8-users/wjMwflJkfso)
    {
        fatalErrorCallback = errorCallback;
        V8::InitializeICU();

        //?v8::V8::InitializeExternalStartupData(PLATFORM_TARGET "\\");
        // (Startup data is not included by default anymore)

        // _platform = platform::NewDefaultPlatform();
        // _platform = new TV8Platform(queueTask);
        // _platform = std::make_unique<TV8Platform>(p1);
        sPlatform = v8::platform::NewDefaultPlatform();

        V8::InitializePlatform(sPlatform.get());

        V8::Initialize();
        externalCall = _externalCall;
        freeMemory = _freeMemory;
        _V8Initialized = true;
    }
    _logger = loggerCallback;
    _platform = sPlatform.get();
    Isolate::CreateParams params;
    _arrayBufferAllocator = ArrayBuffer::Allocator::NewDefaultAllocator();
    params.array_buffer_allocator = _arrayBufferAllocator;

    _isolate = Isolate::New(params);

    // V8_HANDLE_SCOPE

    // v8::Isolate::Scope isolate_scope(_isolate);
    Isolate::Scope iscope(_isolate);

    HandleScope scope(_isolate);

    _isolate->SetFatalErrorHandler(&LogAndroid);

    _isolate->AddMessageListener(FatalErrorLogger);

    _isolate->SetAbortOnUncaughtExceptionCallback(CanAbort);
    // _isolate->SetMicrotasksPolicy(MicrotasksPolicy::kScoped);

    _isolate->SetCaptureStackTraceForUncaughtExceptions(true, 10, v8::StackTrace::kOverview);

    Local<v8::ObjectTemplate> global = ObjectTemplate::New(_isolate);
    Local<v8::Context> c = Context::New(_isolate, nullptr, global);
    v8::Context::Scope context_scope(c);
    _context.Reset(_isolate, c);


    Local<v8::Object> g = c->Global();

    Local<v8::String> gn = V8_STRING("global");

    g->Set(c, gn, g).ToChecked();

    _global.Reset(_isolate, c->Global());

    Local<v8::Symbol> s = v8::Symbol::New(_isolate, V8_STRING("WrappedInstance"));
    _wrapSymbol.Reset(_isolate, s);

    // store wrap symbol at 0
    // _isolate->SetData(0, &_wrapSymbol);

    _isolate->SetData(0, this);

    Local<Private> pWrapField =
            Private::New(_isolate, V8_STRING("WA_V8_WrappedInstance"));

    wrapField.Reset(_isolate, pWrapField);

    _undefined.Reset(_isolate, v8::Undefined(_isolate));
    _null.Reset(_isolate, v8::Null(_isolate));

    if (debug) {
        _sendDebugMessage = sendDebugMessage;
        inspectorClient = new XV8InspectorClient(
                this,
                true,
                sPlatform.get(),
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
//            if (!value->IsNearDeath())
//                return;
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
    Local<Value> v = value->Get(_isolate);
    if (v.IsEmpty())
        return;
    if (!V8External::CheckoutExternal(context, v, force)) {
         // LogAndroid("FreeWrapper", "Exit");
        if (force) {
            delete value;
        }
    }
    // LogAndroid("FreeWrapper", "Exit");
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
    Local<Value> r = Object::New(_isolate);
    return V8Response_From(context, r);
}

V8Response V8Context::CreateNumber(double value) {
    V8_HANDLE_SCOPE
    Local<Value> r = Number::New(_isolate, value);
    return V8Response_From(context, r);
}

V8Response V8Context::CreateBoolean(bool value) {
    V8_HANDLE_SCOPE
    Local<Value> r = v8::Boolean::New(_isolate, value);
    return V8Response_From(context, r);
}

V8Response V8Context::CreateUndefined() {
    V8_HANDLE_SCOPE
    Local<Value> r = _undefined.Get(_isolate);
    return V8Response_From(context, r);
}

V8Response V8Context::CreateNull() {
    V8_HANDLE_SCOPE
    Local<Value> r = _null.Get(_isolate);
    return V8Response_From(context, r);
}

V8Response V8Context::CreateString(XString value) {
    V8_HANDLE_SCOPE
    Local<Value> r = V8_STRING(value);
    return V8Response_From(context, r);
}

V8Response V8Context::CreateSymbol(XString name) {
    V8_HANDLE_SCOPE
    Local<Value> symbol = Symbol::New(_isolate, V8_STRING(name));
    return V8Response_From(context, symbol);
}

V8Response V8Context::CreateDate(int64_t value) {
    V8_HANDLE_SCOPE
    Local<Value> r = v8::Date::New(GetContext(), (double)value).ToLocalChecked();
    return V8Response_From(context, r);
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
            RETURN_EXCEPTION(tryCatch)
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
            RETURN_EXCEPTION(tryCatch)
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
    // h->SetWrapperClassId(WRAPPED_CLASS);
    h->Reset(_isolate, external);
    r.result.handle.handle = h;
    r.result.handle.handleType = V8HandleType::Wrapped;
    r.result.handle.value.refValue = value;
    return r;
}

//void V8Context::PostBackgroundTask(std::unique_ptr<Task> task) {
//    _platform
//        ->GetBackgroundTaskRunner(_isolate)
//        ->PostTask(std::move(task));
//}
//
//void V8Context::PostForegroundTask(std::unique_ptr<Task> task) {
//    _platform
//            ->GetForegroundTaskRunner(_isolate)
//            ->PostTask(std::move(task));
//}
//
//void V8Context::PostWorkerTask(std::unique_ptr<Task> task) {
//    _platform
//            ->GetWorkerThreadsTaskRunner(_isolate)
//            ->PostTask(std::move(task));
//}

void X8Call(const FunctionCallbackInfo<v8::Value> &args) {
    Isolate* isolate = args.GetIsolate();
    Isolate* _isolate = isolate;
    HandleScope scope(isolate);
    Isolate::Scope iscope(_isolate);
    V8Context* cc = V8Context::From(isolate);
    Local<Context> context = cc->GetContext();
    Context::Scope context_scope(context);
    Local<Value> data = args.Data();

    uint32_t n = (uint)args.Length();
    Local<v8::Array> a = v8::Array::New(isolate, n);
    for (uint32_t i = 0; i < n; i++) {
        a->Set(context, i, args[i]).ToChecked();
    }
    Local<Value> _this = args.This();
    V8Response target = V8Response_From(context, _this);
    Local<Value> av = a;
    V8Response handleArgs = V8Response_From(context, av);
    Local<Value> dv = data;
    V8Response fx = V8Response_From(context, dv);
    V8Response r = externalCall(fx, target, handleArgs);

    if (r.type == V8ResponseType::Error) {
        Local<v8::String> error = V8_STRING(r.result.error.message);
        free(r.result.error.message);
        Local<Value> ex = Exception::Error(error);
        isolate->ThrowException(ex);
    } else {
        if (r.result.handle.handle != nullptr) {
            V8Handle h = static_cast<V8Handle>(r.result.handle.handle);
            Local<Value> rx = h->Get(isolate);
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
    Local<Value> v = f;
    return V8Response_From(context, v);
}

V8Response V8Context::Evaluate(XString script, XString location) {
    V8_HANDLE_SCOPE

    TryCatch tryCatch(_isolate);
    // 
    Local<v8::String> v8ScriptSrc = V8_STRING(script);
    Local<v8::String> v8ScriptLocation = V8_STRING(location);

    // Context::Scope context_scope(context);

    ScriptOrigin origin(v8ScriptLocation, v8::Integer::New(_isolate, 0) );

    Local<Script> s;
    if (!Script::Compile(context, v8ScriptSrc, &origin).ToLocal(&s)) {
        RETURN_EXCEPTION(tryCatch)
    }
    Local<Value> result;
    if (!s->Run(context).ToLocal(&result)) {
        RETURN_EXCEPTION(tryCatch)
    }
    return V8Response_From(context, result);
}


V8Response V8Context::Release(V8Handle handle, bool post) {
//    if (post) {
//        V8ReleaseHandle::Post(this, handle);
//        return V8Response_FromBoolean(true);
//    }
    // LogAndroid("CLR", "Release Handle");
    V8_CONTEXT_SCOPE
    try {
        FreeWrapper(handle, false);
        // LogAndroid("Release", "Handle Deleted");
        delete handle;
        V8Response r = {};
        r.type = V8ResponseType ::BooleanValue;
        r.result.booleanValue = true;
        return r;
    } catch (std::exception const &ex) {
        return V8Response_FromError(ex.what());
    }
}

V8Response V8Context::InvokeMethod(V8Handle target, XString name, int len, void** args) {

    V8_CONTEXT_SCOPE
    Local<Value> targetValue = target->Get(_isolate);
    if (!targetValue->IsObject()) {
        return V8Response_FromError("Target is not an Object");
    }
    Local<v8::String> jsName = V8_STRING(name);

    Local<v8::Object> fxObj = Local<v8::Object>::Cast(targetValue);
    Local<v8::Value> fxValue;
    if(!fxObj->Get(context, jsName).ToLocal(&fxValue)) {
        return V8Response_FromError("Method does not exist");
    }
    Local<v8::Function> fx = Local<v8::Function>::Cast(fxValue);

    std::vector<Local<v8::Value>> argList;
    for (int i = 0; i < len; ++i) {
        V8Handle h = TO_HANDLE(args[i]);
        argList.push_back(h->Get(_isolate));
    }
    Local<Value> result;
    if(!fx->Call(context, fxObj, len, argList.data()).ToLocal(&result)) {
        RETURN_EXCEPTION(tryCatch)
    }
    return V8Response_From(context, result);
}

V8Response V8Context::InvokeFunction(V8Handle target, V8Handle thisValue, int len, void** args) {
    V8_CONTEXT_SCOPE
    Local<Value> targetValue = target->Get(_isolate);
    if (!targetValue->IsFunction()) {
        return V8Response_FromError("Target is not a function");
    }
    Local<v8::Object> thisValueValue =
        (thisValue == nullptr || thisValue->IsEmpty())
        ? _global.Get(_isolate)
        : thisValue->Get(_isolate)->ToObject(context).ToLocalChecked();

    if (thisValueValue->IsUndefined()) {
        thisValueValue = _global.Get(_isolate);
    }

    Local<v8::Function> fx = Local<v8::Function>::Cast(targetValue);

    std::vector<Local<v8::Value>> argList;
    for (int i = 0; i < len; ++i) {
        V8Handle h = TO_HANDLE(args[i]);
        argList.push_back(h->Get(_isolate));
    }
    Local<Value> result;
    if(!fx->Call(context, thisValueValue, len, argList.data()).ToLocal(&result)) {
        RETURN_EXCEPTION(tryCatch)
    }
    return V8Response_From(context, result);
}

V8Response V8Context::GetArrayLength(V8Handle target) {
    V8_HANDLE_SCOPE
    // 
    Local<Value> value = target->Get(_isolate);
    if (!value->IsArray()) {
        return V8Response_FromError("Target is not an array");
    }
    Local<v8::Object> jsObj = value->ToObject(context).ToLocalChecked();
    Local<v8::Array> array = Local<v8::Array>::Cast(jsObj);
    return V8Response_FromInteger(array->Length());
}

V8Response V8Context::GetGlobal() {
    V8_CONTEXT_SCOPE
    Local<Value> g = _global.Get(_isolate);
    return V8Response_From(context, g);
}

V8Response V8Context::NewInstance(V8Handle target, int len, void** args) {
    V8_CONTEXT_SCOPE
    Local<Value> targetValue = target->Get(_isolate);
    if (!targetValue->IsFunction()) {
        return V8Response_FromError("Target is not a function");
    }
    Local<v8::Object> fxObj = targetValue->ToObject(context).ToLocalChecked();
    Local<v8::Function> fx = Local<v8::Function>::Cast(fxObj);

    std::vector<Local<v8::Value>> argList;
    for (int i = 0; i < len; ++i) {
        V8Handle h = TO_HANDLE(args[i]);
        argList.push_back(h->Get(_isolate));
    }
    Local<Value> result;
    if(!fx->CallAsConstructor(context, len, argList.data()).ToLocal(&result)) {
        RETURN_EXCEPTION(tryCatch)
    }
    return V8Response_From(context, result);
}

V8Response V8Context::Has(V8Handle target, V8Handle index) {
    V8_HANDLE_SCOPE
    // 
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
    // 
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
    // 
    if (!t->IsObject())
        return V8Response_FromError("This is not an object");
    Local<v8::Name> key = Local<v8::Name>::Cast(index->Get(_isolate));
    Local<v8::Object> obj = t->ToObject(context).ToLocalChecked();
    obj->Set(context, key, v).ToChecked();
    return V8Response_From(context, v);
}


V8Response V8Context::HasProperty(V8Handle target, XString name) {
    V8_HANDLE_SCOPE
    // 
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
    // 
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
    // 
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
    
    if (!t->IsArray())
        return V8Response_FromError("This is not an array");
    Local<v8::Object> obj = t->ToObject(context).ToLocalChecked();
    obj->Set(context, (uint)index, v).ToChecked();
    return V8Response_From(context, v);
}

void V8Context::OutputInspectorMessage(std::unique_ptr<v8_inspector::StringBuffer> &msg) {
    V8_CONTEXT_SCOPE

    auto string = msg->string();

    int length = string.length();
    v8::Local<v8::String> message;
    v8::MaybeLocal<v8::String> maybeString =
            (string.is8Bit()
             ? v8::String::NewFromOneByte(
                            _isolate,
                            reinterpret_cast<const uint8_t*>(string.characters8()),
                            v8::NewStringType::kNormal, length)
             : v8::String::NewFromTwoByte(
                            _isolate,
                            reinterpret_cast<const uint16_t*>(string.characters16()),
                            v8::NewStringType::kNormal, length));
    Local<v8::String> v8Msg = maybeString.ToLocalChecked();

    char* charMsg = V8StringToXString(_isolate, v8Msg);
    _sendDebugMessage(charMsg);
    free(charMsg);
}

V8Response V8Context::DispatchDebugMessage(XString msg, bool post) {

//    if (post) {
//        _platform
//                ->GetWorkerThreadsTaskRunner(_isolate)
//                ->PostTask(
//                        std::make_unique<V8DispatchMessageTask>(this, msg));
//        return V8Response_FromBoolean(true);
//    }

    // LogAndroid("SendDebugMessage", "Begin");
    V8_CONTEXT_SCOPE
    // LogAndroid("SendDebugMessage", "Locked");
    if (inspectorClient != nullptr) {
        // LogAndroid("SendDebugMessage", "Creating String");
        Local<v8::String> message = V8_STRING(msg);
        if (message.IsEmpty()) {
            // LogAndroid("SendDebugMessage", "Could not create String");
            return V8Response_FromError("Could not create string");
        }
        // LogAndroid("SendDebugMessage", "Creating Bufffer");
        String::Value buffer(_isolate, message);
        // LogAndroid("SendDebugMessage", "Creating MessageView");
        v8_inspector::StringView messageView(*buffer, buffer.length());
        // LogAndroid("SendDebugMessage", "Dispatching Message");
        inspectorClient->SendDebugMessage(messageView);

    }
    if (tryCatch.HasCaught()) {
        RETURN_EXCEPTION(tryCatch)
    }
    return V8Response_FromBoolean(true);
}

V8Response V8Context::ToString(V8Handle target) {
    V8_CONTEXT_SCOPE
    
    Local<Value> value = target->Get(_isolate);
    if (!value->IsString()) {
        Local<v8::String> vstr;
        if(!value->ToString(context).ToLocal(&vstr)) {
            RETURN_EXCEPTION(tryCatch)
        }
        return V8Response_ToString(V8StringToXString(_isolate, vstr));
    }
    Local<v8::String> str = Local<v8::String>::Cast(value);
    return V8Response_ToString(V8StringToXString(_isolate, str));
}

void V8External::Log(const char *msg) {
    LogAndroid("Log", msg);
}

void V8External::Release(void *data) {
    LogAndroid("V8External", "Wrapper Released");
    if (data != nullptr) {
        freeMemory(data);
    }
}
