//
// Created by ackav on 08-04-2020.
//

#ifndef LIQUIDCORE_MASTER_V8CONTEXT_H
#define LIQUIDCORE_MASTER_V8CONTEXT_H

#include "common.h"
#include "HashMap.h"

#include "v8-inspector.h"

class V8Response;

typedef V8Response(*ExternalCall)(V8Response fx, V8Response target, V8Response args);

typedef V8Response(*DebugReceiver)(V8Response msg);



class V8Context {
protected:
    std::unique_ptr<Platform> _platform;
    Isolate* _isolate;
    Global<Context> _context;
    Global<Symbol> _wrapSymbol;

    // delete array allocator
    ArrayBuffer::Allocator* _arrayBufferAllocator;

    LoggerCallback _logger;

    Local<Context> GetContext() {
        return _context.Get(_isolate);
    }
public:
    V8Context(
            bool debug,
            LoggerCallback loggerCallback,
            ExternalCall externalCall,
            FreeMemory freeMemory,
            FatalErrorCallback errorCallback,
            ReadDebugMessage readDebugMessage);
    void Dispose();

    V8Response Release(V8Handle handle);
    void FreeWrapper(V8Handle value, bool force);

    V8Response CreateObject();
    V8Response CreateSymbol(XString name);
    V8Response CreateNull();
    V8Response CreateUndefined();
    V8Response CreateBoolean(bool value);
    V8Response CreateNumber(double value);
    V8Response CreateString(XString value);
    V8Response CreateDate(int64_t value);
    V8Response CreateFunction(ExternalCall function, XString debugHelper);
    V8Response DefineProperty(
            V8Handle target,
            XString name,
            NullableBool configurable,
            NullableBool enumerable,
            NullableBool writable,
            V8Handle get,
            V8Handle set,
            V8Handle value            );
    V8Response Evaluate(XString script, XString location);
    V8Response InvokeFunction(V8Handle target, V8Handle thisValue, int len, V8Handle* args);
    V8Response InvokeMethod(V8Handle target, XString name, int len, V8Handle* args);
    V8Response GetGlobal();
    V8Response GetArrayLength(V8Handle target);
    V8Response NewInstance(V8Handle target, int len, V8Handle* args);
    V8Response Has(V8Handle target, V8Handle index);
    V8Response Get(V8Handle target, V8Handle index);
    V8Response Set(V8Handle target, V8Handle index, V8Handle value);
    V8Response HasProperty(V8Handle target, XString name);
    V8Response GetProperty(V8Handle target, XString name);
    V8Response SetProperty(V8Handle target, XString name, V8Handle value);
    V8Response GetPropertyAt(V8Handle target, int index);
    V8Response SetPropertyAt(V8Handle target, int index, V8Handle value);
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

    static Local<v8::Value> Wrap(Local<Context> context, void* data) {
        Isolate* isolate = context->GetIsolate();
        V8External* ex = new V8External();
        ex->_data= data;
        Local<External> ev = External::New(isolate, ex);
        ex->selfValue.Reset(isolate, ev);
        ex->selfValue.SetWrapperClassId(WRAPPED_CLASS);
        ex->AddRef();
        return ev;
    }

    static void CheckoutExternal(Local<Context> context, Local<Value> value, bool force) {
        if (value.IsEmpty())
            return;
        if (!value->IsExternal())
            return;
        Local<External> evalue = Local<External>::Cast(value);
        V8External* external = (V8External*)evalue->Value();
        if (force) {
            external->selfValue.ClearWeak();
            external->selfValue.Reset();
            Release(external->_data);
            external->_data = nullptr;
            return;
        }
        external->Release();
    }

    static V8External* CheckInExternal(Local<Context> context, Local<v8::Value> ex) {
        Local<External> ex1 = Local<External>::Cast(ex);
        V8External* e1 = (V8External*)ex1->Value();
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

    static void Release(void* data);

    inline void MakeWeak() {
        if (!selfValue.IsWeak()) {
            selfValue.SetWeak(this, WeakCallback, WeakCallbackType::kParameter);
        }
    }

    static void WeakCallback(
            const v8::WeakCallbackInfo<V8External>& data) {
        V8External* wrap = data.GetParameter();
        wrap->selfValue.Reset();
        Release(wrap->_data);
        delete wrap;

    }
};

#endif //LIQUIDCORE_MASTER_V8CONTEXT_H
