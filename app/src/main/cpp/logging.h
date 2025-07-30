//
// Created by nidshu01 on 7/29/25.
//

#ifndef BREAKOUTGAME_LOGGING_H
#define BREAKOUTGAME_LOGGING_H

#pragma once

#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "LOG", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARNING, "LOG", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "LOG", __VA_ARGS__)

#endif //BREAKOUTGAME_LOGGING_H
