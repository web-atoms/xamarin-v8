//
// Created by ackav on 08-04-2020.
//

#ifndef LIQUIDCORE_MASTER_V8CONTEXT_H
#define LIQUIDCORE_MASTER_V8CONTEXT_H

#include "common.h"
#include "HashMap.h"

class V8Response;

typedef V8Response(*ExternalCall)(V8Response fx, V8Response target, V8Response args);


class V8Context {
protected:
    std::unique_ptr<Platform> _platform;
    Isolate* _isolate;
    Global<Context> _context;
    Global<Symbol> _wrapSymbol;

    // delete array allocator
    ArrayBuffer::Allocator* _arrayBufferAllocator;

    LoggerCallback _logger;
    ExternalCall _externalCall;

    Local<Context> GetContext() {
        return _context.Get(_isolate);
    }
public:
    V8Context(
            bool debug,
            LoggerCallback loggerCallback,
            ExternalCall ec,
            FreeMemory fm);
    void Dispose();

    V8Response Release(V8Handle handle);
    void FreeWrapper(V8Handle value);

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

#endif //LIQUIDCORE_MASTER_V8CONTEXT_H
