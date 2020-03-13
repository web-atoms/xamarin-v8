#include "XV8.h"

static bool _V8Initialized = false;

V8Context* V8Context_Create() {
	return new V8Context();
}

V8Context::V8Context() {
	if (!_V8Initialized) // (the API changed: https://groups.google.com/forum/#!topic/v8-users/wjMwflJkfso)
	{
		v8::V8::InitializeICU();

		//?v8::V8::InitializeExternalStartupData(PLATFORM_TARGET "\\");
		// (Startup data is not included by default anymore)

		_Platform = v8::Platform::
		v8::V8::InitializePlatform(_Platform.get());

		v8::V8::Initialize();

		_V8Initialized = true;
	}

}