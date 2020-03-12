#pragma once
#include "v8.h"

using namespace v8;

class XV8
{
private:
    v8::Context* context;
public:
    XV8();
    ~XV8();
};
