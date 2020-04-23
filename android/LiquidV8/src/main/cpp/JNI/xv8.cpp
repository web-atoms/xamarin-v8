
#include "V8Response.h"
#include "V8Context.h"
#include "HashMap.h"
// #include "V8Hack.h"

#define INIT_CONTEXT V8Context* context = static_cast<V8Context*>(ctx);

using namespace v8;

static bool _V8Initialized = false;

char* CopyString (const char* msg){
    if (msg == nullptr)
        return nullptr;
uint len = strlen(msg);
char* t = (char*) malloc(len + 1);
strcpy(t, msg);
t[len] = 0;
return t;
}

static CTSL::HashMap<uintptr_t,int> map;

bool IsContextDisposed(V8Context* c) {
    int n;
    auto i = reinterpret_cast<std::uintptr_t>(c);
    return !map.find(i, n);
}

#define VerifyContext(c) \
    if (IsContextDisposed(c)) {\
        V8Response r = {};\
        r.type = V8ResponseType::Error;\
        r.result.error.message = CopyString("Context is disposed");\
        return r;\
    }\


LoggerCallback _logger;

extern "C" {

    V8Context* V8Context_Create(
            bool debug,
            LoggerCallback loggerCallback,
            ExternalCall externalCall,
            FreeMemory  freeMemory,
            ReadDebugMessage readDebugMessage,
            SendDebugMessage sendDebugMessage,
            QueueTask queueTask,
            FatalErrorCallback errorCallback) {
        V8Context*c = new V8Context(
                debug,
                loggerCallback,
                externalCall,
                freeMemory,
                errorCallback,
                readDebugMessage,
                sendDebugMessage,
                queueTask);
        _logger = loggerCallback;
        auto i = reinterpret_cast<std::uintptr_t>(c);
        map.insert(i, 1);
        return c;
    }


    void V8Context_Dispose(ClrPointer ctx) {
        INIT_CONTEXT
        auto i = reinterpret_cast<std::uintptr_t>(context);
        map.erase(i);
        context->Dispose();
        delete context;
    }

    V8Response V8Context_CreateString(ClrPointer ctx, XString value) {
        INIT_CONTEXT
        return context->CreateString(value);
    }

    V8Response V8Context_CreateSymbol(ClrPointer ctx, XString value) {
        INIT_CONTEXT
        return context->CreateSymbol(value);
    }

    V8Response V8Context_CreateDate(ClrPointer ctx, int64_t value) {
        INIT_CONTEXT
        return context->CreateDate(value);
    }

    V8Response V8Context_CreateBoolean(ClrPointer ctx, bool value) {
        INIT_CONTEXT
        return context->CreateBoolean(value);
    }

    void V8Context_PostTask(ClrPointer tsk) {
        // V8Task* task = static_cast<V8Task*>(tsk);
        // task->taskRunner->PostTaskWithInterrupt(task);
    }

    V8Response V8Context_CreateNumber(ClrPointer ctx, double value) {
        INIT_CONTEXT
        return context->CreateNumber(value);
    }

    V8Response V8Context_CreateObject(ClrPointer ctx) {
        INIT_CONTEXT
        return context->CreateObject();
    }

    V8Response V8Context_CreateNull(ClrPointer ctx) {
        INIT_CONTEXT
        return context->CreateNull();
    }

    V8Response V8Context_CreateUndefined(ClrPointer ctx) {
        INIT_CONTEXT
        return context->CreateUndefined();
    }

    V8Response V8Context_CreateFunction(ClrPointer ctx, ExternalCall fn, XString debugDisplay) {
        INIT_CONTEXT
        return context->CreateFunction(fn, debugDisplay);
    }

    V8Response V8Context_DefineProperty(ClrPointer ctx,
                                        ClrPointer target,
                                        XString name,
                                        NullableBool configurable,
                                        NullableBool enumerable,
                                        NullableBool writable,
                                        ClrPointer get,
                                        ClrPointer set,
                                        ClrPointer value) {
        INIT_CONTEXT
        return context->DefineProperty(
                TO_HANDLE(target),
                name,
                configurable,
                enumerable,
                writable,
                TO_HANDLE(get),
                TO_HANDLE(set),
                TO_HANDLE(value));
    }

    V8Response V8Context_GetArrayLength(ClrPointer ctx, ClrPointer target) {
        INIT_CONTEXT
        return context->GetArrayLength(TO_HANDLE(target));
    }

    V8Response V8Context_GetGlobal(ClrPointer ctx) {
        INIT_CONTEXT
        return context->GetGlobal();
    }

    V8Response V8Context_NewInstance(
            ClrPointer ctx,
            ClrPointer target,
            int len,
            ClrPointer* args) {
        INIT_CONTEXT

        return context->NewInstance(TO_HANDLE(target), len, args);
    }


    V8Response V8Context_InvokeFunction(
            ClrPointer ctx,
            ClrPointer target,
            ClrPointer thisValue,
            int len,
            ClrPointer* args) {
        INIT_CONTEXT
        return context->InvokeFunction(
                TO_HANDLE(target),
                TO_HANDLE(thisValue), len, args);
    }

    V8Response V8Context_InvokeMethod(
            ClrPointer ctx,
            ClrPointer target,
            XString name,
            int len,
            ClrPointer* args) {
        INIT_CONTEXT
        return context->InvokeMethod(
                TO_HANDLE(target), name, len, args);
    }

    V8Response V8Context_Has(
            ClrPointer ctx,
            ClrPointer  target,
            ClrPointer index
    ) {
        INIT_CONTEXT
        return context->Has(TO_HANDLE(target), TO_HANDLE(index));
    }

    V8Response V8Context_SendDebugMessage(
            ClrPointer ctx,
            XString message) {
        INIT_CONTEXT
        try {
            if (IsContextDisposed(context))
                return V8Response_FromError("Context disposed");
            return context->DispatchDebugMessage(message, true);
        } catch (std::exception const &ex) {
            _logger(CopyString(ex.what()));
        } catch (...){
            _logger(CopyString("Something went wrong"));
        }
        return V8Response_FromError("Something went wrong");
    }


    V8Response V8Context_HasProperty(
            ClrPointer ctx,
            ClrPointer  target,
            XString text
            ) {
        INIT_CONTEXT
        return context->HasProperty(TO_HANDLE(target), text);
    }

    V8Response V8Context_DeleteProperty(
            ClrPointer ctx,
            ClrPointer target,
            XString name) {
        INIT_CONTEXT
        return context->DeleteProperty(TO_HANDLE(target), name);
    }

    V8Response V8Context_Get(
            ClrPointer ctx,
            ClrPointer target,
            ClrPointer index) {
        INIT_CONTEXT
        return context->Get(TO_HANDLE(target), TO_HANDLE(index));
    }

    V8Response V8Context_Equals(
            ClrPointer ctx,
            ClrPointer left,
            ClrPointer right) {
        INIT_CONTEXT
        return context->Equals(TO_HANDLE(left), TO_HANDLE(right));
    }

    V8Response V8Context_Set(
            ClrPointer ctx,
            ClrPointer target,
            ClrPointer index,
            ClrPointer value) {
        INIT_CONTEXT
        return context->Set(TO_HANDLE(target), TO_HANDLE(index), TO_HANDLE(value));
    }

    V8Response V8Context_GetProperty(
            ClrPointer ctx,
            ClrPointer target,
            XString text) {
        INIT_CONTEXT
        return context->GetProperty(TO_HANDLE(target), text);
    }

    V8Response V8Context_GetPropertyAt(
            ClrPointer ctx,
            ClrPointer target,
            int index) {
        INIT_CONTEXT
        return context->GetPropertyAt(TO_HANDLE(target), index);
    }

    V8Response V8Context_SetProperty(
            ClrPointer ctx,
            ClrPointer target,
            XString text,
            ClrPointer value) {
        INIT_CONTEXT
        return context->SetProperty(TO_HANDLE(target), text, TO_HANDLE(value));
    }

    V8Response V8Context_SetPropertyAt(
            ClrPointer ctx,
            ClrPointer target,
            int index,
            ClrPointer value) {
        INIT_CONTEXT
        return context->SetPropertyAt(TO_HANDLE(target), index, TO_HANDLE(value));
    }

    V8Response V8Context_ToString(
            ClrPointer ctx,
            ClrPointer target
            ) {
        INIT_CONTEXT
        return context->ToString(TO_HANDLE(target));
    }

    V8Response V8Context_Evaluate(ClrPointer ctx, XString script, XString location) {
        INIT_CONTEXT
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

    V8Response V8Context_ReleaseHandle(ClrPointer ctx, ClrPointer h) {
        INIT_CONTEXT
        try {
            if (IsContextDisposed(context)) {
                return V8Response_FromBoolean(true);
            }
            return context->Release(TO_HANDLE(h), true);
        }  catch (std::exception const &ex) {
            return V8Response_FromError(ex.what());
        }
    }


    V8Response V8Context_Wrap(ClrPointer ctx, ClrPointer value) {
        INIT_CONTEXT
        return context->Wrap(value);
    }

}

XString V8StringToXString(Isolate* isolate, Local<v8::String> &text) {
    if (text.IsEmpty())
        return nullptr;
    int len = text->Utf8Length(isolate);
    char *atext = (char*)malloc((uint)len + 1);
    text->WriteUtf8(isolate, atext, len);
    atext[len] = 0;
    return atext;
}

