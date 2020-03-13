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

V8Handle V8Context_CreateNumber(V8Context* context, double value) {
	return context->CreateNumber(value);
}

V8Handle V8Context_CreateObject(V8Context* context) {
	return context->CreateObject();
}

V8Handle V8Context_GetProperty(
	V8Context* context,
	V8Handle target,
	XString text) {
	return context->GetProperty(target, text);
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
	Local<v8::Object> global = Object::New(_isolate);
	Local<Context> c = Context::New(_isolate, nullptr, global);
	_context.Reset(_isolate, c);
	

}

void V8Context::Dispose() {

	_context.Reset();

	_isolate->Dispose();
	// delete _Isolate;
	delete _arrayBufferAllocator;

}

V8Response V8Context::CreateObject() {
	return V8Response::From(Object::New(_isolate));
}

V8Response V8Context::CreateNumber(double value) {
	return V8Response::From(Number::New(_isolate, value));
}

void V8Context::Release(V8Handle handle) {
	handle->Reset();
	delete handle;
}

V8Response V8Context::GetProperty(V8Handle target, XString name) {
	Local<Value> v = target->Get(_isolate);
	Local<Context> context = _context.Get(_isolate);
	// v->ToObject(context).ToLocalChecked()->Get()
	if (!v->IsObject())
		return V8Response::FromError(context, v8::String::NewFromUtf8Literal(_isolate, "This is not object"));
}
