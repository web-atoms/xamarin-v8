
#include "V8Response.h"
#include "V8Context.h"
#include "HashMap.h"

using namespace v8;

static bool _V8Initialized = false;

char* CopyString (const char* msg){
uint len = strlen(msg);
char* t = (char*) malloc(len);
strcpy(t, msg);
return t;
}

CTSL::HashMap<uint64_t,int> map;

bool IsContextDisposed(uint64_t c) {
    int n;
    return !map.find(c, n);
}

#define VerifyContext(c) \
    if (IsContextDisposed(c)) {\
        V8Response r = {};\
        r.type = V8ResponseType::Error;\
        r.result.error.message = CopyString("Context is disposed");\
        return r;\
    }\


extern "C" {

    V8Context *V8Context_Create(
            bool debug,
            LoggerCallback loggerCallback,
            ExternalCall externalCall,
            FreeMemory  freeMemory,
            DebugReceiver  debugReceiver,
            FatalErrorCallback errorCallback) {
        V8Context*c = new V8Context(
                debug,
                loggerCallback,
                externalCall,
                freeMemory,
                debugReceiver,
                errorCallback);
        map.insert((uint64_t)c, 1);
        return c;
    }


    void V8Context_Dispose(V8Context *context) {
        map.erase((uint64_t)context);
        context->Dispose();
        delete context;
    }

    V8Response V8Context_CreateString(V8Context *context, XString value) {
        return context->CreateString(value);
    }

    V8Response V8Context_CreateSymbol(V8Context *context, XString value) {
        return context->CreateSymbol(value);
    }

    V8Response V8Context_CreateDate(V8Context *context, int64_t value) {
        return context->CreateDate(value);
    }

    V8Response V8Context_CreateBoolean(V8Context *context, bool value) {
        return context->CreateBoolean(value);
    }


    V8Response V8Context_CreateNumber(V8Context *context, double value) {
        return context->CreateNumber(value);
    }

    V8Response V8Context_CreateObject(V8Context *context) {
        return context->CreateObject();
    }

    V8Response V8Context_CreateNull(V8Context *context) {
        return context->CreateNull();
    }

    V8Response V8Context_CreateUndefined(V8Context *context) {
        return context->CreateUndefined();
    }

    V8Response V8Context_CreateFunction(V8Context *context, ExternalCall fn, XString debugDisplay) {
        return context->CreateFunction(fn, debugDisplay);
    }

    V8Response V8Context_DefineProperty(V8Context* context,
                                        V8Handle target,
                                        XString name,
                                        NullableBool configurable,
                                        NullableBool enumerable,
                                        NullableBool writable,
                                        V8Handle get,
                                        V8Handle set,
                                        V8Handle value) {
        return context->DefineProperty(
                target, name, configurable, enumerable, writable, get, set, value);
    }

    V8Response V8Context_GetArrayLength(V8Context* context, V8Handle target) {
        return context->GetArrayLength(target);
    }

    V8Response V8Context_GetGlobal(V8Context* context) {
        return context->GetGlobal();
    }

    V8Response V8Context_NewInstance(
            V8Context* context,
            V8Handle target,
            int len,
            V8Handle* args) {
        return context->NewInstance(target, len, args);
    }


    V8Response V8Context_InvokeFunction(
            V8Context* context,
            V8Handle target,
            V8Handle thisValue,
            int len,
            V8Handle* args) {
        return context->InvokeFunction(target, thisValue, len, args);
    }

V8Response V8Context_InvokeMethod(
        V8Context* context,
        V8Handle target,
        XString name,
        int len,
        V8Handle* args) {
    return context->InvokeMethod(target, name, len, args);
}

    V8Response V8Context_Has(
            V8Context* context,
            V8Handle  target,
            V8Handle index
    ) {
        return context->Has(target, index);
    }


V8Response V8Context_HasProperty(
            V8Context* context,
            V8Handle  target,
            XString text
            ) {
        return context->HasProperty(target, text);
    }

    V8Response V8Context_Get(
            V8Context* context,
            V8Handle target,
            V8Handle index) {
        return context->Get(target, index);
    }

    V8Response V8Context_Set(
            V8Context* context,
            V8Handle target,
            V8Handle index,
            V8Handle value) {
        return context->Set(target, index, value);
    }

    V8Response V8Context_GetProperty(
            V8Context* context,
            V8Handle target,
            XString text) {
        return context->GetProperty(target, text);
    }

    V8Response V8Context_GetPropertyAt(
            V8Context* context,
            V8Handle target,
            int index) {
        return context->GetPropertyAt(target, index);
    }

    V8Response V8Context_SetProperty(
            V8Context* context,
            V8Handle target,
            XString text,
            V8Handle value) {
        return context->SetProperty(target, text, value);
    }

    V8Response V8Context_SetPropertyAt(
            V8Context* context,
            V8Handle target,
            int index,
            V8Handle value) {
        return context->SetPropertyAt(target, index, value);
    }

    V8Response V8Context_ToString(
            V8Context* context,
            V8Handle target
            ) {
        return context->ToString(target);
    }

    V8Response V8Context_Evaluate(V8Context* context, XString script, XString location) {
        return context->Evaluate(script, location);
    }

    int V8Context_Release(V8Response r) {
        if (r.type == V8ResponseType::Error) {
            if (r.result.error.message != nullptr) {
                free(r.result.error.message);
            }
            if (r.result.error.stack != nullptr) {
                free(r.result.error.stack);
            }
        } else {
            if (r.type == V8ResponseType::StringValue) {
                free(r.result.stringValue);
            }
        }
        return 0;
    }

    V8Response V8Context_ReleaseHandle(V8Context* context, V8Handle h) {
        try {
            if (IsContextDisposed((uint64_t)context)) {
                return V8Response_FromBoolean(true);
            }
            return context->Release(h);
        }  catch (std::exception const &ex) {
            return V8Response_FromError(ex.what());
        }
    }


    V8Response V8Context_Wrap(V8Context *context, void* value) {
        return context->Wrap(value);
    }

    XString V8StringToXString(Local<Context> context, Local<v8::String> text) {
        if (text.IsEmpty())
            return nullptr;
        Isolate *isolate = context->GetIsolate();
        int len = text->Utf8Length(isolate) + 1;
        char *atext = (char*)malloc((uint)len);
        text->WriteUtf8(isolate, atext, len);
        return atext;
    }
}

