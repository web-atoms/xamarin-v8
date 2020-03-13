#pragma once
#include "v8.h"
using namespace v8;

#pragma comment(lib, "libicudata.a");
#pragma comment(lib, "libv8_base.a");
#pragma comment(lib, "libv8_init.a");
#pragma comment(lib, "libv8_initializers.a");
#pragma comment(lib, "libv8_libbase.a");
#pragma comment(lib, "libv8_libplatform.a");
#pragma comment(lib, "libv8_libsampler.a");
#pragma comment(lib, "libv8_nosnapshot.a");
#pragma comment(lib, "libv8_snapshot.a");


template<class T>
struct V8Handle {
	Persistent<T, CopyablePersistentTraits<T>> handle;
};

template<class T>
V8Handle<T> ToHandle(Local<T> value);

template<class T>
void DisposeHandle(V8Handle<T> handle);


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

	V8Handle<Object> CreateObject();
	V8Handle<Number> CreateNumber(double value);
};