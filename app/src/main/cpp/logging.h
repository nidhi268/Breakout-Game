#pragma once

#include <android/log.h>

#ifndef BREAKOUT_LOGGING_H
#define BREAKOUT_LOGGING_H

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "DEBUG", __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "LOG", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARNING, "LOG", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "LOG", __VA_ARGS__)

#endif //BREAKOUT_LOGGING_H
