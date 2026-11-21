#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <queue>
#include <string>

inline std::string last_script = "";

inline void execute(const std::string& source) {
    last_script = source;
}