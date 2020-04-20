//
// Created by ackav on 19-04-2020.
//

#ifndef ANDROID_V8DISPATCHMESSAGETASK_H
#define ANDROID_V8DISPATCHMESSAGETASK_H

#include "common.h"
#include "InspectorChannel.h"


class V8DispatchMessageTask: public v8::Task {
private:
    V8Context* _context;
    XString _message;
public:

    V8DispatchMessageTask(V8Context* context, XString message) {
        _context = context;
        _message = message;
    }

    void Run() override  {
        _context->SendDebugMessage(_message, false);
    }
};

#endif //ANDROID_V8DISPATCHMESSAGETASK_H
