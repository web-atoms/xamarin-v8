//
// Created by ackav on 08-04-2020.
//

// #include <android/log.h>
#include "V8Context.h"
#include "V8Response.h"
#include "InspectorChannel.h"
#include "ExternalX16String.h"
#include "log.h"

#define RETURN_EXCEPTION(e) \
    return FromException(context, e, __FILE__, __LINE__);                    \

static bool _V8Initialized = false;

static ExternalCall clrExternalCall;
static FreeMemory  clrFreeMemory;
// static AllocateMemory clrAllocateMemory;
static AllocateString clrAllocateString;

static FreeMemory clrFreeHandle;

// static TV8Platform* _platform;
static std::unique_ptr<v8::Platform> sPlatform;
static FatalErrorCallback fatalErrorCallback;
void LogAndroid(const char* location, const char* message) {
    _log("%s %s", location, message);
    // fatalErrorCallback(CopyString(location), CopyString(message));
}

bool CanAbort(Isolate* isolate) {
    return false;
}

void FatalErrorLogger(Local<Message> message, Local<Value> data) {
    _log("Some Error");
}

static __ClrEnv clrEnv;

V8Context::V8Context(
        bool debug,
        ClrEnv env) {
    if (!_V8Initialized) // (the API changed: https://groups.google.com/forum/#!topic/v8-users/wjMwflJkfso)
    {
        fatalErrorCallback = env->fatalErrorCallback;
        // clrAllocateMemory = env->allocateMemory;
        V8::InitializeICU();

        sPlatform = v8::platform::NewDefaultPlatform();

        V8::InitializePlatform(sPlatform.get());

        V8::Initialize();
        clrExternalCall = env->externalCall;
        clrFreeMemory = env->freeMemory;
        clrFreeHandle = env->freeHandle;
        clrAllocateString = env->allocateString;
        _V8Initialized = true;
    }
    // ReturnValue = (uint16_t*) malloc(2048);
    _logger = env->loggerCallback;
    _platform = sPlatform.get();
    Isolate::CreateParams params;
    _arrayBufferAllocator = ArrayBuffer::Allocator::NewDefaultAllocator();
    params.array_buffer_allocator = _arrayBufferAllocator;

    _isolate = Isolate::New(params);

    // uint32_t here;
    // _isolate->SetStackLimit(reinterpret_cast<uintptr_t>(&here - kWorkerMaxStackSize / sizeof(uint32_t*)));
    // V8_HANDLE_SCOPE

    auto cb = [](PromiseRejectMessage msg){
        auto self = V8Context::From(Isolate::GetCurrent());
        auto ctx = Isolate::GetCurrent()->GetCurrentContext();
        HandleScope hs(self->_isolate);
        Local<v8::String> msgText = TO_CHECKED(msg.GetValue()->ToString(self->GetContext()));
        v8::String::Value v(self->_isolate, msgText);
        self->_logger(*v, v.length());
    };

    _isolate->SetPromiseRejectCallback(cb);

    // v8::Isolate::Scope isolate_scope(_isolate);
    /// Isolate::Scope iscope(_isolate);
    _isolate->Enter();

    HandleScope scope(_isolate);

    _isolate->SetFatalErrorHandler(&LogAndroid);

    _isolate->AddMessageListener(FatalErrorLogger);

    _isolate->SetAbortOnUncaughtExceptionCallback(CanAbort);

    // _isolate->SetMicrotasksPolicy(MicrotasksPolicy::kScoped);

    _isolate->SetCaptureStackTraceForUncaughtExceptions(true, 10, v8::StackTrace::kOverview);

    Local<v8::ObjectTemplate> global = ObjectTemplate::New(_isolate);
    Local<v8::Context> c = Context::New(_isolate, nullptr, global);
    // v8::Context::Scope context_scope(c);
    _context.Reset(_isolate, c);

    c->Enter();


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
    _emptyString.Reset(_isolate, v8::String::Empty(_isolate));

    if (debug) {
        inspectorClient = new XV8InspectorClient(
                this,
                true,
                sPlatform.get(),
                env);
    }


}

V8Response V8Context::FromException(Local<Context> &context, TryCatch &tc, const char* file, const int line) {
    HandleScope s(_isolate);
    Local<Value> ex = tc.Exception();
    if (ex.IsEmpty()) {
        return FromError("No error specified");
    }
    Local<v8::Object> exObj = Local<v8::Object>::Cast(ex);
    Local<Value> st;
    Local<v8::String> key = V8_STRING("stack");
    V8Response r;
    if (exObj->Get(context,  key).ToLocal(&st)) {
        Local<v8::String> stack = Checked(file, line, st->ToString(context));
        r = CreateStringFrom(stack);
        if (r.type == V8ResponseType::ConstCharArray) {
            r.type = V8ResponseType::ConstError;
        } else {
            r.type = V8ResponseType::Error;
        }
        return r;
    }
    Local<v8::String> msg = Checked(file, line, exObj->ToString(context));
    r = CreateStringFrom(msg);
    if (r.type == V8ResponseType::ConstCharArray) {
        r.type = V8ResponseType::ConstError;
    } else {
        r.type = V8ResponseType::Error;
    }
    return r;
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

V8Response V8Context::DeleteProperty(V8Handle target, Utf16Value name) {
    V8_CONTEXT_SCOPE
    Local<Value> t = target->Get(_isolate);
    Local<v8::Object> tobj = Local<v8::Object>::Cast(t);
    Local<v8::String> n = V8_UTF16STRING(name);
    bool r;
    if(!tobj->Delete(context, n).To(&r)) {
        RETURN_EXCEPTION(tryCatch)
    }
    return V8Response_FromBoolean(r);
}

V8Response V8Context::Equals(V8Handle left, V8Handle right) {
    V8_CONTEXT_SCOPE
    Local<Value> l = left->Get(_isolate);
    Local<Value> r = right->Get(_isolate);
    bool v;
    if(!l->Equals(context, r).To(&v)) {
        RETURN_EXCEPTION(tryCatch)
    }
    return V8Response_FromBoolean(v);
}

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
    HandleScope s(_isolate);
    {

        if (inspectorClient != nullptr) {
            delete inspectorClient;
        }

        V8WrappedVisitor v;
        v.context = this;
        v.force = true;
        _isolate->VisitHandlesWithClassIds(&v);
        v.context = nullptr;
        ///Local<Context> cc = _context.Get(_isolate);
        _context.Reset();

        _wrapSymbol.Reset();
        _global.Reset();
        _undefined.Reset();
        _null.Reset();
        _emptyString.Reset();
        wrapField.Reset();
        // cc->Exit();
    }
    // _isolate->Exit();
    _isolate->Dispose();
    // delete _isolate;
    delete _arrayBufferAllocator;
    // free(ReturnValue);

}

V8Response V8Context::CreateObject() {
    V8_CONTEXT_SCOPE
    Local<Value> r = Object::New(_isolate);
    return V8Response_From(context, r);
}

V8Response V8Context::CreateArray() {
    V8_CONTEXT_SCOPE
    Local<Value> r = v8::Array::New(_isolate);
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

V8Response V8Context::CreateString(Utf16Value value) {
    V8_HANDLE_SCOPE
    Local<Value> r = V8_UTF16STRING(value);
    return V8Response_From(context, r);
}

V8Response V8Context::CreateStringFrom(Local<v8::String> &value) {
    V8Response r = {};
    r.type = V8ResponseType::CharArray;
    auto length = value->Length();
    if (length == 0) {
        // this tells CLR that this is an empty string.. it is not null
        r.result.booleanValue = 1;
        return r;
    }

    // is it an external string?
    if (value->IsExternal()) {
        // value->GetExternalStringResource()
        ExternalX16String* ext = dynamic_cast<ExternalX16String*>(value->GetExternalStringResource());
        if (ext != nullptr) {
            r.type = V8ResponseType::ConstCharArray;
            r.address = (void *) ext->Handle();
            return r;
        }
    }

    auto es = clrAllocateString(length);
    value->Write(_isolate, (uint16_t*)es.Value, 0, length);
    r.length = length;
    r.address = (void*)es.Handle;
//    int n = value->Length();
//    r.length = n;
//    if (n < 1024) {
//
//        // should not be deallocated by receiver...
//        r.type = V8ResponseType ::ConstCharArray;
//        // copy to internal buffer...
//        ReturnValue[n] = 0;
//        value->Write(_isolate, ReturnValue);
//        r.address = ReturnValue;
//        // not allocating string here ...
//        return r;
//    }
//    uint16_t* buffer = (uint16_t*)clrAllocateMemory((n+1)*2);
//    value->Write(_isolate, buffer);
//    r.address = buffer;
    return r;
}

V8Response V8Context::CreateSymbol(Utf16Value name) {
    V8_HANDLE_SCOPE
    Local<Value> symbol = Symbol::New(_isolate, V8_UTF16STRING(name));
    return V8Response_From(context, symbol);
}

V8Response V8Context::CreateDate(int64_t value) {
    V8_HANDLE_SCOPE
    Local<Value> r = v8::Date::New(GetContext(), (double)value).ToLocalChecked();
    return V8Response_From(context, r);
}

V8Response V8Context::FromError(const char *msg) {
    V8Response r = {};
    r.type = V8ResponseType::Error;
    int n = strlen(msg);
    auto es = clrAllocateString(n);
    uint16_t* buffer = (uint16_t*)es.Value;
    for(int i=0; i<n; i++) {
        buffer[i] = static_cast<uint16_t>(msg[i]);
    }
    r.length = n;
    r.address = (void*)es.Handle;
//    if (n < 1023) {
//        buffer = ReturnValue;
//        r.type = V8ResponseType::ConstError;
//    } else {
//        buffer = (uint16_t*)clrAllocateMemory((n+1)*2);
//        r.address = buffer;
//    }
//    for (int i = 0; i < n; ++i) {
//        buffer[i] = static_cast<uint16_t>(msg[i]);
//    }
//    buffer[n] = 0;
//    r.length = n;
//    r.address = buffer;
    return r;
}

V8Response V8Context::DefineProperty(
        V8Handle target,
        Utf16Value name,
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
        return FromError("Target is not an object");
    }
    Local<v8::Object> jsObj = t.As<v8::Object>();
    Local<v8::String> key = V8_UTF16STRING(name);

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
            if (tryCatch.HasCaught()) {
                RETURN_EXCEPTION(tryCatch)
            }
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
            if (tryCatch.HasCaught()) {
                RETURN_EXCEPTION(tryCatch)
            }
        }
    }

    return V8Response_FromBoolean(true);
}

V8Response V8Context::Wrap(void *value) {
    V8_CONTEXT_SCOPE

    Local<v8::Value> external = V8External::Wrap(context, value, nullptr);

    V8Response r = {};
    r.type = V8ResponseType::Wrapped;
    V8Handle h = new Global<Value>();
    h->SetWrapperClassId(WRAPPED_CLASS);
    h->Reset(_isolate, external);
    r.address = h;
    r.result.refValue = value;
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
    // auto params = (V8Response*)malloc(sizeof(V8Response)*n*2);
    // std::vector<V8Response*> paramList;
    // V8Response* p1 = params;
    std::vector<V8Response> params;
    for (uint32_t i = 0; i < n; i++) {
        Local<Value> v = args[i];
        // *p1 =V8Response_From(context, v);
        params.push_back(V8Response_From(context, v));
        // p1 += sizeof(V8Response);
    }
    Local<Value> _this = args.This();
    V8Response target = V8Response_From(context, _this);
    // Local<Value> av = a;
    V8Response handleArgs = {};
    handleArgs.type = V8ResponseType::ResponseArray;
    handleArgs.address = (void*)params.data();
    handleArgs.length = n;
    // Local<Value> dv = data;
    // V8Response fx = V8Response_From(context, dv);
    Local<External> ext = Local<External>::Cast(data);
    ExternalCall exCall = (ExternalCall)((V8External*)ext->Value())->Data();
    V8Response r = exCall(target, handleArgs);

    // free(params);

    if (r.type == V8ResponseType::Error || r.type == V8ResponseType::ConstError) {
        // error will be sent as UTF8
        Local<v8::String> error =
                TO_CHECKED(v8::String::NewFromTwoByte(
                        _isolate,
                        (const uint16_t*)r.address,
                        NewStringType::kNormal,
                        r.length));
        // free(r.result.error.message);
        if (r.type == V8ResponseType::Error) {
            clrFreeMemory((void *) r.address);
        }
        Local<Value> ex = Exception::Error(error);
        isolate->ThrowException(ex);
    } else {
        if (r.address != nullptr) {
            V8Handle h = static_cast<V8Handle>(r.address);
            Local<Value> rx = h->Get(isolate);
            args.GetReturnValue().Set(rx);
        } else {
            args.GetReturnValue().SetUndefined();
        }
    }
}

V8Response V8Context::CreateFunction(
        ExternalCall function,
        ClrPointer  handle,
        Utf16Value debugHelper) {
    V8_CONTEXT_SCOPE
    Local<Value> e = V8External::Wrap(context, (void*)handle, (void*)function);
    // Local<External> e = External::New(_isolate, (void*)function);

    Local<v8::Function> f = v8::Function::New(context, X8Call, e).ToLocalChecked();
    Local<v8::String> n = V8_UTF16STRING(debugHelper);
    f->SetName(n);
    Local<Value> v = f;
    return V8Response_From(context, v);
}

V8Response V8Context::Evaluate(Utf16Value script,Utf16Value location) {
    V8_HANDLE_SCOPE

    TryCatch tryCatch(_isolate);
    Local<v8::String> v8ScriptSrc = V8_UTF16STRING(script);
    Local<v8::String> v8ScriptLocation = V8_UTF16STRING(location);

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
    try {
        V8_CONTEXT_SCOPE
        FreeWrapper(handle, false);
        // LogAndroid("Release", "Handle Deleted");
        delete handle;
        V8Response r = {};
        r.type = V8ResponseType ::Boolean;
        r.result.booleanValue = true;
        return r;
    } catch (std::exception const &ex) {
        return FromError(ex.what());
    }
}

V8Response V8Context::InvokeMethod(V8Handle target, Utf16Value name, int len, void** args) {

    V8_CONTEXT_SCOPE
    Local<Value> targetValue = target->Get(_isolate);
    if (targetValue.IsEmpty()) {
        return FromError("Target is empty");
    }
    if (!targetValue->IsObject()) {
        return FromError("Target is not an Object");
    }
    Local<v8::String> jsName = V8_UTF16STRING(name);

    Local<v8::Object> fxObj = Local<v8::Object>::Cast(targetValue);
    Local<v8::Value> fxValue;
    if(!fxObj->Get(context, jsName).ToLocal(&fxValue)) {
        return FromError("Method does not exist");
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
        return FromError("Target is not a function");
    }
    Local<v8::Object> thisValueValue =
        (thisValue == nullptr || thisValue->IsEmpty())
        ? _global.Get(_isolate)
        : TO_CHECKED(thisValue->Get(_isolate)->ToObject(context));

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

V8Response V8Context::IsInstanceOf(V8Handle target, V8Handle jsClass) {
    V8_CONTEXT_SCOPE
    Local<Value> targetValue = target->Get(_isolate);
    Local<v8::Object> targetObj = TO_CHECKED(targetValue->ToObject(context));
    Local<Value> classValue = jsClass->Get(_isolate);
    Local<v8::Object> classObj = TO_CHECKED(targetObj->ToObject(context));
    bool v = TO_CHECKED(targetObj->InstanceOf(context, classObj));
    return V8Response_FromBoolean(v);
}

V8Response V8Context::GetArrayLength(V8Handle target) {
    V8_HANDLE_SCOPE
    // 
    Local<Value> value = target->Get(_isolate);
    if (!value->IsArray()) {
        return FromError("Target is not an array");
    }
    Local<v8::Array> array = Local<v8::Array>::Cast(value);
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
        return FromError("Target is not a function");
    }
    Local<v8::Function> fx = Local<v8::Function>::Cast(targetValue);

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
        return V8Response_FromBoolean(false);
    }
    Local<v8::Object> obj = Local<v8::Object>::Cast(value);
    Local<v8::Name> key = Local<v8::Name>::Cast(index->Get(_isolate));
    return V8Response_FromBoolean(obj->HasOwnProperty(context, key).ToChecked());
}

V8Response V8Context::Get(V8Handle target, V8Handle index) {
    V8_HANDLE_SCOPE
    Local<Value> v = target->Get(_isolate);
    // 
    if (!v->IsObject())
        return FromError("This is not an object");
    Local<v8::Name> key = Local<v8::Name>::Cast(index->Get(_isolate));
    Local<v8::Object> jsObj = TO_CHECKED(v->ToObject(context));
    Local<Value> item = TO_CHECKED(jsObj->Get(context, key));
    return V8Response_From(context, item);
}

V8Response V8Context::Set(V8Handle target, V8Handle index, V8Handle value) {
    V8_HANDLE_SCOPE
    Local<Value> t = target->Get(_isolate);
    Local<Value> v = value->Get(_isolate);
    //
    if (!t->IsObject())
        return FromError("This is not an object");
    Local<v8::Name> key = Local<v8::Name>::Cast(index->Get(_isolate));
    Local<v8::Object> obj = TO_CHECKED(t->ToObject(context));
    obj->Set(context, key, v).ToChecked();
    return V8Response_From(context, v);
}


V8Response V8Context::HasProperty(V8Handle target, Utf16Value name) {
    V8_HANDLE_SCOPE
    // 
    Local<Value> value = target->Get(_isolate);
    if (!value->IsObject()) {
        return FromError("Target is not an object ");
    }
    Local<v8::Object> obj = TO_CHECKED(value->ToObject(context));
    Local<v8::String> key = V8_UTF16STRING(name);
    return V8Response_FromBoolean(TO_CHECKED(obj->HasOwnProperty(context, key)));
}

V8Response V8Context::GetProperty(V8Handle target, Utf16Value name) {
    V8_HANDLE_SCOPE
    Local<Value> v = target->Get(_isolate);
    // 
    if (!v->IsObject())
        return FromError("This is not an object");
    Local<v8::String> jsName = V8_UTF16STRING(name);
    Local<v8::Object> jsObj = TO_CHECKED(v->ToObject(context));
    Local<Value> item = TO_CHECKED(jsObj->Get(context, jsName));
    return V8Response_From(context, item);
}

V8Response V8Context::SetProperty(V8Handle target, Utf16Value name, V8Handle value) {
    V8_HANDLE_SCOPE
    Local<Value> t = target->Get(_isolate);
    Local<Value> v = value->Get(_isolate);
    // 
    if (!t->IsObject())
        return FromError("This is not an object");
    Local<v8::String> jsName = V8_UTF16STRING(name);
    Local<v8::Object> obj = Local<v8::Object>::Cast(t);
    TO_CHECKED(obj->Set(context, jsName, v));
    return V8Response_From(context, v);
}

V8Response V8Context::GetPropertyAt(V8Handle target, int index) {
    V8_HANDLE_SCOPE
    Local<Value> v = target->Get(_isolate);
    
    if (!v->IsArray())
        return FromError("This is not an array");
    Local<v8::Object> a = TO_CHECKED(v->ToObject(context));
    Local<Value> item = TO_CHECKED(a->Get(context, (uint) index));
    return V8Response_From(context, item);
}

V8Response V8Context::SetPropertyAt(V8Handle target, int index, V8Handle value) {
    V8_HANDLE_SCOPE
    Local<Value> t = target->Get(_isolate);
    Local<Value> v = value->Get(_isolate);
    
    if (!t->IsArray())
        return FromError("This is not an array");
    Local<v8::Object> obj = TO_CHECKED(t->ToObject(context));
    obj->Set(context, (uint)index, v).ToChecked();
    return V8Response_From(context, v);
}

V8Response V8Context::DispatchDebugMessage(Utf16Value msg, bool post) {

    V8_CONTEXT_SCOPE
    // LogAndroid("SendDebugMessage", "Locked");
    if (inspectorClient != nullptr) {
        v8_inspector::StringView messageView(msg->Value, msg->Length);
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
        return CreateStringFrom(vstr);
    }
    Local<v8::String> str = Local<v8::String>::Cast(value);
    return CreateStringFrom(str);
}

void V8External::Log(const char *msg) {
    LogAndroid("Log", msg);
}

void V8External::Release(void *data) {
    // LogAndroid("V8External", "Wrapper Released");
    if (data != nullptr) {
        clrFreeHandle(data);
    }
}

V8Response V8Response_From(Local<Context> &context, Local<Value> &handle)
{
    V8Response v = {};
    if (handle.IsEmpty()) {
        return v;
    }

    // this is just for reference
    // this is only used in CLR to compare two object
    // if address is same, objects are considered equal
    // otherwise JavaScript equals method is invoked
    v.result.refValue = *handle;

    Isolate* isolate = context->GetIsolate();

    HandleScope hs(isolate);

    // for handle, we need to set the type..
    if (handle->IsUndefined()) {
        v.type = V8ResponseType::Undefined;
    }
    else if (handle->IsNull()) {
        v.type = V8ResponseType::Null;
    }
    else if (handle->IsString()) {
        v.type = V8ResponseType::String;
        // Local<v8::String> vstr = Local<v8::String>::Cast(handle);
        // v8::String::Value value(isolate, vstr);
        // v.stringValue = *value;
    } else if (handle->IsStringObject()) {
        v.type = V8ResponseType::String;
    }
    else if (handle->IsBoolean() || handle->IsBooleanObject()) {
        v.type = V8ResponseType::Boolean;
        v.result.booleanValue = (uint8_t)handle->BooleanValue(isolate);
    } else if (handle->IsInt32()) {
        v.type = V8ResponseType::Integer;
        v.result.intValue = handle->Int32Value(context).ToChecked();
    }
    else if (handle->IsNumber() || handle->IsNumberObject()) {
        double d;
        if (handle->NumberValue(context).To(&d)) {
            v.type = V8ResponseType::Number;
            v.result.doubleValue = d;
        }
        else {
            v.type = V8ResponseType::NotANumber;
        }
    }
    else if (handle->IsDate()) {
        v.type = V8ResponseType::Date;
        v.result.doubleValue = TO_CHECKED(handle->ToObject(context)).As<v8::Date>()->ValueOf();
    }
    else if (handle->IsArray()
             || handle->IsArgumentsObject()
             || handle->IsBigInt64Array()) {
        v.type = V8ResponseType::Array;
    }
    else if (handle->IsSymbol()) {
        v.type = V8ResponseType::TypeSymbol;
    }
    else if (handle->IsExternal()) {
        v.type = V8ResponseType::Wrapped;
        V8External* e = V8External::CheckInExternal(context, handle);
        if (e != nullptr) {
            v.result.refValue = e->Handle();
        }
    }
    else if (handle->IsObject()) {
//        v.type = V8ResponseType::Wrapped;
//        V8External* e = V8External::CheckInExternal(context, handle);
//        if (e != nullptr) {
//            v.result.refValue = e->Handle();
//        }
        v.type = V8ResponseType::Object;
    }

    V8Handle h = new Global<Value>();
    h->SetWrapperClassId(WRAPPED_CLASS);
    v.address = (void*)h;
    // this is just to skip equals when we need to compare two array items
    h->Reset(isolate, handle);
    return v;
}
