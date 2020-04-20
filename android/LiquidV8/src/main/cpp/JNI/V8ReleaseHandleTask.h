//
// Created by ackav on 19-04-2020.
//

#ifndef ANDROID_V8RELEASEHANDLETASK_H
#define ANDROID_V8RELEASEHANDLETASK_H

#include "common.h"
#include "V8Context.h"

class V8ReleaseHandle: public v8::Task {
    V8Context* _context;
    V8Handle  _handle;
public:

    V8ReleaseHandle(V8Context* context, V8Handle handle) {
        _context = context;
        _handle = handle;
    }

    static void Post(V8Context* context, V8Handle handle) {
        context->PostBackgroundTask(std::make_unique<V8ReleaseHandle>(context, handle));
    }

    void Run() override {
        _context->Release(_handle, false);
    }
};

#endif //ANDROID_V8RELEASEHANDLETASK_H
