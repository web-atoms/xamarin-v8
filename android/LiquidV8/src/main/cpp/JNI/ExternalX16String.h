//
// Created by ackav on 23-04-2020.
//

#ifndef ANDROID_EXTERNALX16STRING_H
#define ANDROID_EXTERNALX16STRING_H

#include "common.h"


class ExternalX16String : public v8::String::ExternalStringResource {
private:
    const uint16_t* _data;
    const size_t _len;
public:

    ExternalX16String(const uint16_t* d, int len):
        _data(d),
        _len(static_cast<size_t>(len))
    {
    }

    ~ExternalX16String() override {
        // do nothing...
    }

    virtual const uint16_t* data() const override {
        return _data;
    }

    /**
     * The length of the string. That is, the number of two-byte characters.
     */
    virtual size_t length() const override {
        return _len;
    }
};

#endif //ANDROID_EXTERNALX16STRING_H
