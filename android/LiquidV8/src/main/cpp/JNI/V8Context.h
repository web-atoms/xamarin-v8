//
// Created by ackav on 08-04-2020.
//

#ifndef LIQUIDCORE_MASTER_V8CONTEXT_H
#define LIQUIDCORE_MASTER_V8CONTEXT_H

#include "common.h"
#include "HashMap.h"

#include "v8-inspector.h"
class XV8InspectorClient;

class V8Response;

typedef V8Response(*ExternalCall)(V8Response fx, V8Response target, V8Response args);

extern "C" {

    typedef void (*BreakPauseOn)(bool value);

    struct __ClrEnv {
        AllocateMemory allocateMemory;
        FreeMemory freeMemory;

        // release wrapped handle...
        FreeMemory freeHandle;

        ExternalCall externalCall;
        LoggerCallback loggerCallback;
        // wait for debug message
        ReadDebugMessage readDebugMessage;
        SendDebugMessage sendDebugMessage;
        FatalErrorCallback fatalErrorCallback;

        BreakPauseOn breakPauseOn;
    };

    typedef __ClrEnv *ClrEnv;
}

class V8Context {
protected:
    Platform* _platform;
    Isolate* _isolate;
    Global<Context> _context;
    Global<Symbol> _wrapSymbol;
    Global<v8::Object> _global;
    Global<v8::Value> _undefined;
    Global<v8::Value> _null;
    XV8InspectorClient* inspectorClient = nullptr;

    std::vector<__Utf16Value> dirtyStrings;

    // try to reuse buffer... if value below 1024 for most cases it will be
    uint16_t ReturnValue[1024];

    // delete array allocator
    ArrayBuffer::Allocator* _arrayBufferAllocator;

    LoggerCallback _logger;

public:

    V8Response CreateStringFrom(Local<v8::String> &value);

    V8Response FromException(Local<Context> &context, TryCatch &tc, const char* file, const int line);

    V8Response FromError(const char* msg);

    inline Platform* GetPlatform() {
        return _platform;
    }

    inline Local<Context> GetContext() {
        return _context.Get(_isolate);
    }

    inline Isolate* GetIsolate() {
        return _isolate;
    }

    Global<Private> wrapField;

    static V8Context* From(Isolate* isolate) {
        return static_cast<V8Context*>(isolate->GetData(0));
    }

    V8Context(
            bool debug,
            ClrEnv env
            );
    void Dispose();

    V8Response Release(V8Handle handle, bool post);
    void FreeWrapper(V8Handle value, bool force);

    V8Response CreateObject();
    V8Response CreateSymbol(Utf16Value name);
    V8Response CreateNull();
    V8Response CreateUndefined();
    V8Response CreateArray();
    V8Response CreateBoolean(bool value);
    V8Response CreateNumber(double value);
    V8Response CreateString(Utf16Value value);
    V8Response CreateDate(int64_t value);
    V8Response CreateFunction(ExternalCall function, Utf16Value debugHelper);
    V8Response DefineProperty(
            V8Handle target,
            Utf16Value name,
            NullableBool configurable,
            NullableBool enumerable,
            NullableBool writable,
            V8Handle get,
            V8Handle set,
            V8Handle value            );
    V8Response DeleteProperty(V8Handle target, Utf16Value name);
    V8Response Evaluate(Utf16Value script,Utf16Value location);
    V8Response InvokeFunction(V8Handle target, V8Handle thisValue, int len, void** args);
    V8Response InvokeMethod(V8Handle target, Utf16Value name, int len, void** args);
    V8Response IsInstanceOf(V8Handle target, V8Handle jsClass);
    V8Response Equals(V8Handle left, V8Handle right);
    V8Response GetGlobal();
    V8Response GetArrayLength(V8Handle target);
    V8Response NewInstance(V8Handle target, int len, void** args);
    V8Response Has(V8Handle target, V8Handle index);
    V8Response Get(V8Handle target, V8Handle index);
    V8Response Set(V8Handle target, V8Handle index, V8Handle value);
    V8Response HasProperty(V8Handle target, Utf16Value name);
    V8Response GetProperty(V8Handle target, Utf16Value name);
    V8Response SetProperty(V8Handle target, Utf16Value name, V8Handle value);
    V8Response GetPropertyAt(V8Handle target, int index);
    V8Response SetPropertyAt(V8Handle target, int index, V8Handle value);
    V8Response DispatchDebugMessage(Utf16Value message, bool post);
    V8Response Wrap(void* value);
    V8Response ToString(V8Handle target);
    V8Response GC();

private:

};

/**
 * The class holds reference to external object.
 *
 * External reference must increase the value
 * Delete must decrease the reference till it is zero
 * **/
class V8External {
private:
    int _ref = 0;
    void* _data;
    Global<Value> selfValue;

public:

    inline void* Data() {
        return _data;
    }

    static Local<v8::Value> Wrap(Local<Context> &context, void* data, bool makeWeak) {
        Isolate* isolate = context->GetIsolate();
        HandleScope handleScope(isolate);
        V8External* ex = new V8External();
        ex->_data= data;
        Local<External> ev = External::New(isolate, ex);
        Local<v8::Object> wrapper = v8::Object::New(isolate);

        V8Context* v8Context = V8Context::From(isolate);
        Local<Private> wrapField = v8Context->wrapField.Get(isolate);
        wrapper->SetPrivate(context, wrapField, ev);
        ex->selfValue.Reset(isolate, wrapper);
        ex->selfValue.SetWrapperClassId(WRAPPED_CLASS);
        if (makeWeak) {
            ex->MakeWeak();
        } else {
            ex->AddRef();
        }
        return wrapper;
    }

    static bool CheckoutExternal(Local<Context> &context, Local<Value> &value, bool force) {
        Isolate* isolate = context->GetIsolate();
        HandleScope handleScope(isolate);
        if (value.IsEmpty())
            return false;
        if (!value->IsObject())
            return false;
        Local<v8::Object> wrapper = TO_CHECKED(value->ToObject(context));
        V8Context* v8Context = V8Context::From(isolate);
        Local<Private> wrapField = v8Context->wrapField.Get(isolate);
        if (!wrapper->HasPrivate(context, wrapField).ToChecked())
            return false;
        if(!wrapper->GetPrivate(context, wrapField).ToLocal(&value))
            return false;
        Local<External> evalue = Local<External>::Cast(value);
        V8External* external = (V8External*)evalue->Value();
        if (force) {
            external->selfValue.ClearWeak();
            external->selfValue.Reset();
            Release(external->_data);
            external->_data = nullptr;
            return true;
        }
        external->Release();
        return true;
    }

    static V8External* CheckInExternal(Local<Context> &context, Local<v8::Value> &ex) {
        Isolate* isolate = context->GetIsolate();
        HandleScope handleScope(isolate);
        if (!ex->IsObject())
            return nullptr;
        Local<v8::Object> obj = TO_CHECKED(ex->ToObject(context));
        V8Context* v8Context = V8Context::From(isolate);
        Local<Private> wrapField = v8Context->wrapField.Get(isolate);
        if (!obj->HasPrivate(context, wrapField).ToChecked()) {
            return nullptr;
        }
        Local<Value> value;
        if (!obj->GetPrivate(context, wrapField).ToLocal(&value))
            return nullptr;
        Local<External> ex1 = Local<External>::Cast(value);
        V8External* e1 = static_cast<V8External*>(ex1->Value());
        e1->AddRef();
        return e1;
    }

    inline void AddRef() {
        _ref++;
        selfValue.ClearWeak();
    }
    inline void Release() {
        _ref--;

        if (_ref <= 0) {
            MakeWeak();
        }
    }

private:

    static void Log(const char* msg);

    static void Release(void* data);

    inline void MakeWeak() {
        if (!selfValue.IsWeak()) {
            selfValue.SetWeak(this, WeakCallback, WeakCallbackType::kParameter);
        }
    }

    static void WeakCallback(
            const v8::WeakCallbackInfo<V8External>& data) {

        Log("Weak Callback");

        V8External* wrap = static_cast<V8External*>(data.GetParameter());
        wrap->selfValue.ClearWeak();
        wrap->selfValue.Reset();
        Release(wrap->_data);
        delete wrap;

    }
};

#endif //LIQUIDCORE_MASTER_V8CONTEXT_H
