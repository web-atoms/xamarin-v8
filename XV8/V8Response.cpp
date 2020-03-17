#include "pch.h"
#include "V8Response.h"
#include <stdlib.h>

V8Response V8Response::From(Local<Context> context, Local<Value> handle)
{
	V8Response v = V8Response();
	v.type = V8ResponseType::Handle;

	Isolate* isolate = context->GetIsolate();

	if (handle.IsEmpty()) {
		return FromError(context, v8::String::NewFromUtf8Literal(isolate, "Unexpected empty value"));
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
	else if (handle -> IsDate()) {
		v.result.handle.handleType = V8HandleType::Date;
		v.result.handle.value.doubleValue = handle->ToObject(context).ToLocalChecked().As<v8::Date>()->ValueOf();
	} else if (handle->IsArray()
		|| handle->IsArgumentsObject()
		|| handle->IsBigInt64Array()) {
		v.result.handle.handleType = V8HandleType::Array;
	} else if (handle->IsObject()) {
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
	Local<v8::Name> name = v8::String::NewFromUtf8Literal(isolate, "stack");
	if(local->HasOwnProperty(context, name).ToChecked()) {
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