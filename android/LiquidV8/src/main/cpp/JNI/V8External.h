//
// Created by ackav on 13-04-2020.
//

#ifndef ANDROID_V8EXTERNAL_H
#define ANDROID_V8EXTERNAL_H

#include "common.h"

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

    static Local<v8::Object> Wrap(Local<Context> context, void* data) {
        Isolate* isolate = context->GetIsolate();
        Global<Symbol>* s = (Global<Symbol>*)isolate->GetData(0);
        Local<Symbol> symbol = s->Get(isolate);
        Local<v8::Object> v = v8::Object::New(isolate);
        V8External* ex = new V8External();
        ex->_data= data;
        Local<External> ev = External::New(isolate, ex);
        v->Set(context, symbol, ev);
        ex->selfValue.Reset(isolate, v);
        ex->AddRef();
        return v;
    }

    static void CheckoutExternal(Local<Context> context, Local<Value> value) {
        if (value.IsEmpty())
            return;
        if (value->IsObject())
            return;
        Isolate* isolate = context->GetIsolate();
        Global<Symbol>* s = (Global<Symbol>*)isolate->GetData(0);
        Local<Symbol> symbol = s->Get(isolate);
        Local<v8::Object> obj = value->ToObject(context).ToLocalChecked();
        Local<Value> ev;
        if (!obj->Get(context, symbol).ToLocal(&ev)) {
            return;
        }
        if (ev.IsEmpty())
            return;
        if(!ev->IsExternal())
            return;
        Local<External> evalue = Local<External>::Cast(ev);
        V8External* external = (V8External*)evalue->Value();
        external->Release();
    }

    static V8External* CheckInExternal(Local<Context> context, Local<v8::Object> ex) {
        Isolate* isolate = context->GetIsolate();
        Global<Symbol>* s = (Global<Symbol>*)isolate->GetData(0);
        Local<Symbol> symbol = s->Get(isolate);
        Local<Value> v;
        if (!ex->Get(context, symbol).ToLocal(&v))  {
            return nullptr;
        }
        Local<External> ev = Local<External>::Cast(v);
        V8External* e1 = (V8External*)ev->Value();
        e1->AddRef();
        return e1;
    }

    inline void AddRef() {
        _ref++;
        selfValue.ClearWeak();
    }
    inline void Release() {
        _ref--;

        if (_ref == 0) {
            selfValue.SetWeak(this, WeakCallback,  WeakCallbackType::kParameter);
        }
    }

private:

    static void WeakCallback(
            const v8::WeakCallbackInfo<V8External>& data) {
        V8External* wrap = data.GetParameter();
        wrap->selfValue.Reset();
        ((FreeMemory)freeMemory)(wrap->_data);
        delete wrap;
    }

};


#endif //ANDROID_V8EXTERNAL_H
