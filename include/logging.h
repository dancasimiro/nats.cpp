#ifndef __LOGGING_H__
#define __LOGGING_H__

#include <functional>
#include <string>

enum class LogLevel {
    INFO,
    WARN,
    ERROR
};
typedef std::function<void(LogLevel level, const std::string& msg)> Logger;

#endif // __LOGGING_H__
