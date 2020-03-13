#pragma once
#include "v8.h"
#include "V8Response.h"
using namespace v8;

class V8Context {
protected:
	int32_t _engineID;

	std::unique_ptr<Platform> _platform;
	Isolate* _isolate;
	Persistent<Context, CopyablePersistentTraits<Context>> _context;
	EscapableHandleScope* _scope;

	// delete array allocator
	ArrayBuffer::Allocator* _arrayBufferAllocator;
public:
	V8Context();
	void Dispose();

	void Release(V8Handle handle);

	V8Response CreateObject();
	V8Response CreateNumber(double value);
	V8Response GetProperty(V8Handle target, XString name);
private:

};

