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

V8Response V8Context_CreateString(V8Context* context, XString value) {
	return context->CreateString(value);
}

V8Response V8Context_CreateDate(V8Context* context, int64_t value) {
	return context->CreateDate(value);
}

V8Response V8Context_CreateBoolean(V8Context* context, bool value) {
	return context->CreateBoolean(value);
}


V8Response V8Context_CreateNumber(V8Context* context, double value) {
	return context->CreateNumber(value);
}

V8Response V8Context_CreateObject(V8Context* context) {
	return context->CreateObject();
}

V8Response V8Context_CreateNull(V8Context* context) {
	return context->CreateNull();
}

V8Response V8Context_CreateUndefined(V8Context* context) {
	return context->CreateUndefined();
}


V8Response V8Context_GetProperty(
	V8Context* context,
	V8Handle target,
	XString text) {
	return context->GetProperty(target, text);
}

V8Response V8Context_GetPropertyAt(
	V8Context* context,
	V8Handle target,
	int index) {
	return context->GetPropertyAt(target, index);
}

V8Response V8Context_SetProperty(
	V8Context* context,
	V8Handle target,
	XString text,
	V8Handle value) {
	return context->SetProperty(target, text, value);
}

V8Response V8Context_SetPropertyAt(
	V8Context* context,
	V8Handle target,
	int index,
	V8Handle value) {
	return context->SetPropertyAt(target, index, value);
}

V8Response V8Context_Evaluate(V8Context* context, XString script, XString location) {
	return context->Evaluate(script, location);
}

int V8Context_Release(V8Response r) {
	if (r.type == V8ResponseType::Error) {
		if (r.result.error.message != nullptr) {
			delete r.result.error.message;
		}
		if (r.result.error.stack != nullptr) {
			delete r.result.error.stack;
		}
	} else {
		 if (r.type == V8ResponseType::String) {
			 delete r.result.stringValue;
		 }
	}
}

int V8Context_ReleaseHandle(V8Handle r) {
	r->Reset();
	delete r;
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
	Local<v8::Context> c = Context::New(_isolate, nullptr, global);
	_context.Reset(_isolate, c);
	

}

void V8Context::Dispose() {

	_context.Reset();

	_isolate->Dispose();
	// delete _Isolate;
	delete _arrayBufferAllocator;

}

V8Response V8Context::CreateObject() {
	return V8Response::From(GetContext(), Object::New(_isolate));
}

V8Response V8Context::CreateNumber(double value) {
	return V8Response::From(GetContext(), Number::New(_isolate, value));
}

V8Response V8Context::CreateBoolean(bool value) {
	return V8Response::From(GetContext(), v8::Boolean::New(_isolate, value));
}

V8Response V8Context::CreateUndefined() {
	return V8Response::From(GetContext(), v8::Undefined(_isolate));
}

V8Response V8Context::CreateNull() {
	return V8Response::From(GetContext(), v8::Null(_isolate));
}

V8Response V8Context::CreateString(XString value) {
	return V8Response::From(GetContext(), v8::String::NewFromUtf8(_isolate, value).ToLocalChecked());
}

V8Response V8Context::CreateDate(int64_t value) {
	return V8Response::From(GetContext(), v8::Date::New(GetContext(), (double)value).ToLocalChecked());
}

void X8Call(const FunctionCallbackInfo<v8::Value>& args) {
	Isolate* isolate = args.GetIsolate();
	Local<Context> context = isolate->GetCurrentContext();
	ExternalCall function = (ExternalCall)(args.Data()->ToBigInt(context).ToLocalChecked()->Uint64Value());

	HandleScope scope(isolate);
	uint32_t n = args.Length();
	Local<v8::Array> a = v8::Array::New(isolate, n);
	for (uint32_t i = 0; i < n; i++) {
		a->Set(context, i, args[i]);
	}
	V8Handle target = new Persistent<Value, CopyablePersistentTraits<Value>>(isolate, args.This());
	V8Handle handleArgs = new Persistent<Value, CopyablePersistentTraits<Value>>(isolate, a);
	V8Response r = function(target, handleArgs);

	if (r.type == V8ResponseType::Error) {
		Local<Value> error = v8::String::NewFromUtf8(isolate, r.result.error.message).ToLocalChecked();
		// delete 
		isolate->ThrowException(Exception::Error(error));
	} else if (r.type == V8ResponseType::String) {
		args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, r.result.stringValue).ToLocalChecked());
	}
	else {
		args.GetReturnValue().Set(r.result.handle.handle);
	}
}

V8Response V8Context::CreateFunction(ExternalCall function, XString debugHelper) {
	Local<Value> v = v8::BigInt::NewFromUnsigned(_isolate, (uint64_t)function);
	Local<FunctionTemplate> f = FunctionTemplate::New(_isolate, X8Call, v);
	Local<Value> n = v8::String::NewFromUtf8(_isolate, debugHelper).ToLocalChecked();
	f->Set(_isolate, "name", n);
	return V8Response::From(GetContext(), f);
}

V8Response V8Context::Evaluate(XString script, XString location) {
	HandleScope scoppe(_isolate);
	TryCatch tryCatch(_isolate);
	Local<Context> context(_isolate->GetCurrentContext());
	ScriptOrigin origin(String::NewFromUtf8Literal(_isolate, "External"));
	Local<v8::String> l = String::NewFromUtf8(_isolate, location).ToLocalChecked();
	Local<Script> s = Script::Compile(context, s, &origin).ToLocalChecked();
}


void V8Context::Release(V8Handle handle) {
	handle->Reset();
	delete handle;
}

V8Response V8Context::GetProperty(V8Handle target, XString name) {
	Local<Value> v = target->Get(_isolate);
	Local<Context> context = GetContext();
	if (!v->IsObject())
		return V8Response::FromError(context, v8::String::NewFromUtf8Literal(_isolate, "This is not an object"));
	Local<v8::String> jsName = v8::String::NewFromUtf8(_isolate, name).ToLocalChecked();
	Local<Value> item = v->ToObject(context).ToLocalChecked()->Get(context, jsName).ToLocalChecked();
	return V8Response::From(context, item);
}

V8Response V8Context::GetPropertyAt(V8Handle target, int index) {
	Local<Value> v = target->Get(_isolate);
	Local<Context> context = GetContext();
	if (!v->IsArray())
		return V8Response::FromError(context, v8::String::NewFromUtf8Literal(_isolate, "This is not an array"));
	Local<Value> item = v->ToObject(context).ToLocalChecked()->Get(context, index).ToLocalChecked();
	return V8Response::From(context, item);
}

V8Response V8Context::SetProperty(V8Handle target, XString name, V8Handle value) {
	Local<Value> t = target->Get(_isolate);
	Local<Value> v = value->Get(_isolate);
	Local<Context> context = GetContext();
	if (!t->IsObject())
		return V8Response::FromError(context, v8::String::NewFromUtf8Literal(_isolate, "This is not an object"));
	Local<v8::String> jsName = v8::String::NewFromUtf8(_isolate, name).ToLocalChecked();
	t->ToObject(context).ToLocalChecked()->Set(context, jsName, v);
	return V8Response::From(context, v);
}

V8Response V8Context::SetPropertyAt(V8Handle target, int index, V8Handle value) {
	Local<Value> t = target->Get(_isolate);
	Local<Value> v = value->Get(_isolate);
	Local<Context> context = GetContext();
	if (!t->IsArray())
		return V8Response::FromError(context, v8::String::NewFromUtf8Literal(_isolate, "This is not an array"));
	t->ToObject(context).ToLocalChecked()->Set(context, index, v);
	return V8Response::From(context, v);
}