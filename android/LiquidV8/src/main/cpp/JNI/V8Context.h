//
// Created by ackav on 08-04-2020.
//

#ifndef LIQUIDCORE_MASTER_V8CONTEXT_H
#define LIQUIDCORE_MASTER_V8CONTEXT_H

#include "common.h"

class V8Response;

typedef V8Response(*ExternalCall)(V8Response target, V8Response args);


class V8Context {
protected:
    int32_t _engineID;

    std::unique_ptr<Platform> _platform;
    Isolate* _isolate;
    Global<Context> _context;

    // delete array allocator
    ArrayBuffer::Allocator* _arrayBufferAllocator;

    LoggerCallback _logger;

    Local<Context> GetContext() {
        return _context.Get(_isolate);
    }
public:
    V8Context(bool debug, LoggerCallback loggerCallback);
    void Dispose();

    void Release(V8Handle handle);

    V8Response CreateObject();
    V8Response CreateNull();
    V8Response CreateUndefined();
    V8Response CreateBoolean(bool value);
    V8Response CreateNumber(double value);
    V8Response CreateString(XString value);
    V8Response CreateDate(int64_t value);
    V8Response CreateFunction(ExternalCall function, XString debugHelper);
    V8Response Evaluate(XString script, XString location);
    V8Response GetProperty(V8Handle target, XString name);
    V8Response SetProperty(V8Handle target, XString name, V8Handle value);
    V8Response GetPropertyAt(V8Handle target, int index);
    V8Response SetPropertyAt(V8Handle target, int index, V8Handle value);
    V8Response Wrap(void* value);
private:

};

#endif //LIQUIDCORE_MASTER_V8CONTEXT_H
