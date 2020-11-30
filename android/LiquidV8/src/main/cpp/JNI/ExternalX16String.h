//
// Created by ackav on 23-04-2020.
//

#ifndef ANDROID_EXTERNALX16STRING_H
#define ANDROID_EXTERNALX16STRING_H

#include "common.h"

class VolatileString: public v8::String::ExternalStringResource {
private:
    const uint16_t * _data;
    const size_t _len;
public:
    VolatileString(const uint16_t*d, const int len):
        _data(d),
        _len(len)
    {

    }

    virtual const uint16_t* data() const override {
        return _data;
    };

    /**
     * The length of the string. That is, the number of two-byte characters.
     */
    virtual size_t length() const override {
        return _len;
    }

};

class ExternalX16String : public v8::String::ExternalStringResource {
private:
    const uint16_t* _data;
    const size_t _len;
    const void* _handle;
    FreeMemory _freeMemory;
public:

    inline const void* Handle() {
        return _handle;
    }

    ExternalX16String(const uint16_t* d, int len, const void* handle, FreeMemory freeMemory):
        _data(d),
        _len(static_cast<size_t>(len)),
        _handle(handle),
        _freeMemory(freeMemory)
    {
    }

    ~ExternalX16String() override {
        // do nothing...
        if (_handle != nullptr) {
            _freeMemory(_handle);
        }
    }

    virtual const uint16_t* data() const override {
        return _data;
    };

    /**
     * The length of the string. That is, the number of two-byte characters.
     */
    virtual size_t length() const override {
        return _len;
    }
};

class GlobalX16String : public v8::String::ExternalStringResource {
private:
    const uint16_t* _data;
    const size_t _len;
    const void* _handle;
public:

    inline const void* Handle() {
        return _handle;
    }

    GlobalX16String(const uint16_t* d, int len, const void* handle):
            _data(d),
            _len(static_cast<size_t>(len)),
            _handle(handle)
    {
    }

    ~GlobalX16String() override {
    }

    virtual const uint16_t* data() const override {
        return _data;
    };

    /**
     * The length of the string. That is, the number of two-byte characters.
     */
    virtual size_t length() const override {
        return _len;
    }
};

#endif //ANDROID_EXTERNALX16STRING_H
