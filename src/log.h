//
// Created by 16182 on 1/10/2021.
//

#ifndef V8DEBUGGER_LOG_H
#define V8DEBUGGER_LOG_H

#include <android/log.h>
#include <chrono>

#define LOG_I(fmt, ...) __android_log_print(ANDROID_LOG_INFO, "cpp", fmt, ##__VA_ARGS__)
#define LOG_E(fmt, ...) __android_log_print(ANDROID_LOG_ERROR, "cpp", fmt, ##__VA_ARGS__)
#define LOG_TIME_NS(tag) __android_log_print(ANDROID_LOG_INFO, "cpp", "%s at %lldns", tag, std::chrono::high_resolution_clock::now().time_since_epoch().count())

#endif //V8DEBUGGER_LOG_H
