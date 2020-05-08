//
// Created by ackav on 08-05-2020.
//
#include "log.h"
#include <android/log.h>

void _log(const char* format, ... ) {
    va_list args;
    va_start(args, format);
    __android_log_vprint(ANDROID_LOG_ERROR, "V8", format, args );
    va_end(args);
}


