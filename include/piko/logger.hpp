#pragma once

#include <iostream>
#include <string>
#include <cstdarg>
#include <cstdint>
#include <cinttypes>
#include <vector>

namespace piko {
    enum class LogLevel {
        INFO,
        WARN,
        ERR,
        DEBG
    };

    inline void LogMsg(LogLevel level, const char* format, ...) {
        // Initialize C-style variadic argument parsing
        va_list args;
        va_start(args, format);

        // Determine required buffer size by simulating a write pass
        va_list args_copy;
        va_copy(args_copy, args);
        int size = vsnprintf(nullptr, 0, format, args_copy);
        va_end(args_copy);

        if (size <= 0) {
            va_end(args);
            return;
        }

        // Allocate a dynamic local buffer string and format the text
        std::string buffer(size, '\0');
        vsnprintf(&buffer[0], size + 1, format, args);
        va_end(args);

        switch (level) {
            case LogLevel::INFO:
                std::cout << "[PBOX INFO]: " << buffer << "\n";
                break;
            case LogLevel::WARN:
                std::cout << "[PBOX WARN]: \033[33m" << buffer << "\033[0m\n"; // Yellow accent
                break;
            case LogLevel::ERR:
                std::cerr << "[PBOX ERRO]: \033[31m" << buffer << "\033[0m\n"; // Red accent
                break;
            case LogLevel::DEBG:
                std::cout << "[PBOX DEBG]: \033[32m" << buffer << "\033[0m\n"; // Green accent
                break;
        }
    }
}

// MACRO for LogMsg for  Pass __VA_ARGS__ smoothly down to the underlying va_list parser
#define PBOX_INFO(format, ...)  ::piko::LogMsg(::piko::LogLevel::INFO, format, ##__VA_ARGS__)
#define PBOX_WARN(format, ...)  ::piko::LogMsg(::piko::LogLevel::WARN, format, ##__VA_ARGS__)
#define PBOX_ERROR(format, ...) ::piko::LogMsg(::piko::LogLevel::ERR, format, ##__VA_ARGS__)
#define PBOX_DEBUG(format, ...) ::piko::LogMsg(::piko::LogLevel::DEBG, format, ##__VA_ARGS__)