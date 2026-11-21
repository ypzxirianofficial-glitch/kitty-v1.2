#pragma once
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <mutex>
#include <windows.h>

#include "globals.h"

enum class log_type {
    debug,
    info,
    warn,
    err
};

inline std::mutex& log_mutex() {
    static std::mutex m;
    return m;
}

class debugger {
public:
    debugger(log_type type, int) : type_(type) {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
        localtime_s(&tm, &t);

        switch (type) {
        case log_type::debug: stream_ << "[DEBUG]"; break;
        case log_type::info:  stream_ << "[INFO]";  break;
        case log_type::warn:  stream_ << "[WARN]";  break;
        case log_type::err:   stream_ << "[ERROR]"; break;
        }

        stream_ << "[" << std::setfill('0') << std::setw(2) << tm.tm_hour << ':'
            << std::setfill('0') << std::setw(2) << tm.tm_min << ':'
            << std::setfill('0') << std::setw(2) << tm.tm_sec << ']'
            << "[" << GetCurrentProcessId() << "] ";
    }

    ~debugger() {
        stream_ << '\n';
        std::lock_guard<std::mutex> lk(log_mutex());

        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO info{};
        bool have_info = GetConsoleScreenBufferInfo(hConsole, &info);

        if (type_ == log_type::err) {
            SetConsoleTextAttribute(hConsole,
                BACKGROUND_RED | FOREGROUND_RED | FOREGROUND_GREEN |
                FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        }

        std::cout << stream_.str();
        std::cout.flush();

        if (have_info)
            SetConsoleTextAttribute(hConsole, info.wAttributes);
    }

    template <typename T>
    debugger& operator<<(const T& msg) {
        stream_ << msg;
        return *this;
    }

    debugger& operator<<(std::ostream& (*manip)(std::ostream&)) {
        stream_ << manip;
        return *this;
    }

private:
    log_type type_;
    std::ostringstream stream_;
};

#define log_debug() debugger(log_type::debug, __LINE__)
#define log_info()  debugger(log_type::info,  __LINE__)
#define log_warn()  debugger(log_type::warn,  __LINE__)
#define log_err()   debugger(log_type::err,   __LINE__)