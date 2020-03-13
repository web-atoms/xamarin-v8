#pragma once
#include "v8.h"
using namespace v8;

class V8Context {
protected:
	int32_t _engineID;

	std::unique_ptr<v8::Platform> _Platform;
};