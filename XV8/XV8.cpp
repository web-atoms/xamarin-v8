#include "v8.h"
#include <stdlib.h>
#include "libplatform.h"

using namespace v8;

typedef char* XString;

/**
   Everything is sent as a pointer to Persistent object, reason is, JavaScript engine should
   not destroy it till it is explicitly destroyed by host application.
*/
typedef Persistent<Value, CopyablePersistentTraits<Value>>* V8Handle;

enum V8ResponseType : int16_t {
	Error = 0,
	Handle = 1,
	String = 2
};

enum V8HandleType : int16_t {
	None = 0,
	Undefined = 1,
	Null = 2,
	Number = 3,
	NotANumber = 4,
	BigInt = 5,
	Boolean = 6,
	String = 0xFF,
	Object = 0xF0,
	Function = 0xF1,
	Array = 0xF2,
	Remote = 0xF3,
	Date = 0xF4
};

typedef union {
	bool boolValue;
	int32_t intValue;
	int64_t longValue;
	double doubleValue;
	XString stringValue;
} V8Value;

XString V8StringToXString(Local<Context> context, Local<v8::String> text);

/*
When a call is made from outside, response will indicate success/failure
and it will contain the value. In case of string, the response must be
disposed by the caller by calling V8Context_Release method.
*/
struct V8Response
{
public:
	V8ResponseType type;
	union {
		struct {
			V8Handle handle;
			V8HandleType handleType;
			V8Value value;
		} handle;
		struct {
			XString message;
			XString stack;
		} error;
		XString stringValue;
		long longValue;
	} result;

	static V8Response From(Local<Context> context, Local<Value> handle);

	static V8Response FromError(Local<Context> context, Local<Value> error);

	static V8Response ToString(Local<Context> context, Local<Value> error);
};

static bool _V8Initialized = false;

typedef V8Response(*ExternalCall)(V8Response target, V8Response args);

class V8Context {
protected:
	int32_t _engineID;

	std::unique_ptr<Platform> _platform;
	Isolate* _isolate;
	Persistent<Context, CopyablePersistentTraits<Context>> _context;

	// delete array allocator
	ArrayBuffer::Allocator* _arrayBufferAllocator;

	Local<Context> GetContext() {
		return _context.Get(_isolate);
	}
public:
	V8Context();
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
private:

};



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

V8Response V8Response::From(Local<Context> context, Local<Value> handle)
{
	V8Response v = V8Response();
	v.type = V8ResponseType::Handle;

	Isolate* isolate = context->GetIsolate();

	if (handle.IsEmpty()) {
		return FromError(context, v8::String::NewFromUtf8(isolate, "Unexpected empty value").ToLocalChecked());
	}


	// for handle, we need to set the type..
	if (handle->IsUndefined()) {
		v.result.handle.handleType = V8HandleType::Undefined;
	}
	else if (handle->IsNull()) {
		v.result.handle.handleType = V8HandleType::Null;
	}
	else if (handle->IsString() || handle->IsStringObject()) {
		v.result.handle.handleType = V8HandleType::String;
	}
	else if (handle->IsBoolean() || handle->IsBooleanObject()) {
		v.result.handle.handleType = V8HandleType::Boolean;
		v.result.handle.value.boolValue = handle->BooleanValue(isolate);
	}
	else if (handle->IsNumber() || handle->IsNumberObject()) {
		double d;
		if (handle->NumberValue(context).To(&d)) {
			v.result.handle.handleType = V8HandleType::Number;
			v.result.handle.value.doubleValue = d;
		}
		else {
			v.result.handle.handleType = V8HandleType::NotANumber;
		}
	}
	else if (handle->IsDate()) {
		v.result.handle.handleType = V8HandleType::Date;
		v.result.handle.value.doubleValue = handle->ToObject(context).ToLocalChecked().As<v8::Date>()->ValueOf();
	}
	else if (handle->IsArray()
		|| handle->IsArgumentsObject()
		|| handle->IsBigInt64Array()) {
		v.result.handle.handleType = V8HandleType::Array;
	}
	else if (handle->IsObject()) {
		v.result.handle.handleType = V8HandleType::Object;
	}

	v.result.handle.handle = new Persistent<Value, CopyablePersistentTraits<Value>>();
	v.result.handle.handle->Reset(isolate, handle);
	return v;
}

V8Response V8Response::FromError(Local<Context> context, Local<Value> error) {
	V8Response v = V8Response();
	v.type = V8ResponseType::Error;
	Isolate* isolate = context->GetIsolate();
	MaybeLocal<v8::Object> obj = error->ToObject(context);
	Local<v8::Object> local = obj.ToLocalChecked();
	Local<v8::Name> name = v8::String::NewFromUtf8(isolate, "stack").ToLocalChecked();
	if (local->HasOwnProperty(context, name).ToChecked()) {
		Local<v8::Value> stack = local->Get(context, name).ToLocalChecked();
		v.result.error.stack = V8StringToXString(context, stack.As<v8::String>());
	}
	else {
		v.result.error.stack = nullptr;
	}
	Local<v8::String> msg = local->ToString(context).ToLocalChecked();
	v.result.error.message = V8StringToXString(context, msg);
	return v;
}

XString V8StringToXString(Local<Context> context, Local<v8::String> text) {
	if (text.IsEmpty())
		return nullptr;
	Isolate* isolate = context->GetIsolate();
	int len = text->Utf8Length(isolate);
	char* atext = (char*)malloc(len);
	text->WriteUtf8(isolate, atext, len);
	return atext;
}

V8Response V8Response::ToString(Local<Context> context, Local<Value> value) {
	V8Response v = V8Response();
	v.type = V8ResponseType::String;
	Local<v8::String> s = value->ToString(context).ToLocalChecked();
	v.result.stringValue = V8StringToXString(context, s);
	return v;
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
	return 0;
}

int V8Context_ReleaseHandle(V8Handle r) {
	r->Reset();
	delete r;
	return 0;
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
	Local<v8::ObjectTemplate> global = ObjectTemplate::New(_isolate);
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
	Local<Context> context(isolate->GetCurrentContext());
	ExternalCall function = (ExternalCall)(args.Data()->ToBigInt(context).ToLocalChecked()->Uint64Value());

	HandleScope scope(isolate);
	uint32_t n = args.Length();
	Local<v8::Array> a = v8::Array::New(isolate, n);
	for (uint32_t i = 0; i < n; i++) {
		a->Set(context, i, args[i]).Check();
	}
	V8Response target = V8Response::From(context, args.This());
	V8Response handleArgs = V8Response::From(context, a);
	V8Response r = function(target, handleArgs);

	if (r.type == V8ResponseType::Error) {
		Local<v8::String> error = v8::String::NewFromUtf8(isolate, r.result.error.message).ToLocalChecked();
		Local<Value> ex = Exception::Error(error);
		isolate->ThrowException(ex);
		delete r.result.error.message;
	} else if (r.type == V8ResponseType::String) {
		args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, r.result.stringValue).ToLocalChecked());
		delete r.result.stringValue;
	}
	else {
		Local<Value> rx = r.result.handle.handle->Get(isolate);
		args.GetReturnValue().Set(rx);
		delete r.result.handle.handle;
	}
}

V8Response V8Context::CreateFunction(ExternalCall function, XString debugHelper) {
	Local<Value> v = v8::BigInt::NewFromUnsigned(_isolate, (uint64_t)function);
	Local<Context> context(_isolate->GetCurrentContext());
;	Local<v8::Function> f = v8::Function::New(context, X8Call, v).ToLocalChecked();
	Local<v8::String> n = v8::String::NewFromUtf8(_isolate, debugHelper).ToLocalChecked();
	f->SetName(n);
	return V8Response::From(GetContext(), f);
}

V8Response V8Context::Evaluate(XString script, XString location) {
	HandleScope scoppe(_isolate);
	TryCatch tryCatch(_isolate);
	Local<Context> context(_isolate->GetCurrentContext());
	ScriptOrigin origin(String::NewFromUtf8(_isolate, location).ToLocalChecked());
	Local<v8::String> sc = String::NewFromUtf8(_isolate, script).ToLocalChecked();
	Local<Script> s;
	if (!Script::Compile(context, sc, &origin).ToLocal(&s)) {
		return V8Response::FromError(context, tryCatch.Exception());
	}
	Local<Value> result;
	if (!s->Run(context).ToLocal(&result)) {
		return V8Response::FromError(context, tryCatch.Exception());
	}
	return V8Response::From(context, result);
}


void V8Context::Release(V8Handle handle) {
	handle->Reset();
	delete handle;
}

V8Response V8Context::GetProperty(V8Handle target, XString name) {
	Local<Value> v = target->Get(_isolate);
	Local<Context> context = GetContext();
	if (!v->IsObject())
		return V8Response::FromError(context, v8::String::NewFromUtf8(_isolate, "This is not an object").ToLocalChecked());
	Local<v8::String> jsName = v8::String::NewFromUtf8(_isolate, name).ToLocalChecked();
	Local<Value> item = v->ToObject(context).ToLocalChecked()->Get(context, jsName).ToLocalChecked();
	return V8Response::From(context, item);
}

V8Response V8Context::GetPropertyAt(V8Handle target, int index) {
	Local<Value> v = target->Get(_isolate);
	Local<Context> context = GetContext();
	if (!v->IsArray())
		return V8Response::FromError(context, v8::String::NewFromUtf8(_isolate, "This is not an array").ToLocalChecked());
	Local<Value> item = v->ToObject(context).ToLocalChecked()->Get(context, index).ToLocalChecked();
	return V8Response::From(context, item);
}

V8Response V8Context::SetProperty(V8Handle target, XString name, V8Handle value) {
	Local<Value> t = target->Get(_isolate);
	Local<Value> v = value->Get(_isolate);
	Local<Context> context = GetContext();
	if (!t->IsObject())
		return V8Response::FromError(context, v8::String::NewFromUtf8(_isolate, "This is not an object").ToLocalChecked());
	Local<v8::String> jsName = v8::String::NewFromUtf8(_isolate, name).ToLocalChecked();
	t->ToObject(context).ToLocalChecked()->Set(context, jsName, v).ToChecked();
	return V8Response::From(context, v);
}

V8Response V8Context::SetPropertyAt(V8Handle target, int index, V8Handle value) {
	Local<Value> t = target->Get(_isolate);
	Local<Value> v = value->Get(_isolate);
	Local<Context> context = GetContext();
	if (!t->IsArray())
		return V8Response::FromError(context, v8::String::NewFromUtf8(_isolate, "This is not an array").ToLocalChecked());
	t->ToObject(context).ToLocalChecked()->Set(context, index, v).ToChecked();
	return V8Response::From(context, v);
}