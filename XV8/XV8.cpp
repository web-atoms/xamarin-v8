#include "XV8.h"
#include "libplatform.h"

using namespace v8;
static bool _V8Initialized = false;

V8Context* V8Context_Create() {
	return new V8Context();
}

void V8Context_Dispose(V8Context* context) {
	context->Dispose();
	delete context;
}

V8Handle<Value> V8Context_CreateNumber(V8Context* context, double value) {
	return context->CreateNumber(value);
}

template<class T>
V8Handle<T> ToHandle(Local<T> value) {
	V8Handle<T> h;
	h.handle.Reset(value);
	reutrn h;
}

template<class T>
void DisposeHandle(V8Handle<T> handle) {
	handle.handle.Reset();
}


V8Context::V8Context() {
	if (!_V8Initialized) // (the API changed: https://groups.google.com/forum/#!topic/v8-users/wjMwflJkfso)
	{
		V8::InitializeICU();

		//?v8::V8::InitializeExternalStartupData(PLATFORM_TARGET "\\");
		// (Startup data is not included by default anymore)

		_platform = platform::NewDefaultPlatform();
		V8::InitializePlatform(_platform.get());

		V8::Initialize();

		_V8Initialized = true;
	}
	
	Isolate::CreateParams params;
	_arrayBufferAllocator = ArrayBuffer::Allocator::NewDefaultAllocator();
	params.array_buffer_allocator = _arrayBufferAllocator;
	
	_isolate = Isolate::New(params);
	Local<Object> global = Object::New(_isolate);
	Local<Context> c = Context::New(_isolate, nullptr, global);
	_context.Reset(_isolate, c);
	

}

void V8Context::Dispose() {

	_context.Reset();

	_isolate->Dispose();
	// delete _Isolate;
	delete _arrayBufferAllocator;

}

V8Handle<Object> V8Context::CreateObject() {
	return ToHandle(Object::New(_isolate));
}

V8Handle<Number> V8Context::CreateNumber(double value) {
	return ToHandle(Number::New(_isolate));
}