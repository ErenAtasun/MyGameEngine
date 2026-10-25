#include <MME/Log.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <mutex>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace mme {

    namespace {
        LogLevel s_MinLevel = LogLevel::Info;
        std::mutex s_Mutex;
        const auto s_StartTime = std::chrono::steady_clock::now();

        // Konsol renk desteği yalnızca gerçek bir terminale yazarken açılır;
        // çıktı dosyaya yönlendirildiyse kaçış kodları basılmaz.
        bool DetectColorSupport() {
#ifdef _WIN32
            // Dosya yolları ve cihaz adları UTF-8; konsol eski kod sayfasında bozuk gösterirdi
            SetConsoleOutputCP(CP_UTF8);
            HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
            DWORD mode = 0;
            if (out == INVALID_HANDLE_VALUE || !GetConsoleMode(out, &mode)) return false;
            return SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
#else
            return true;
#endif
        }

        const char* LevelTag(LogLevel level) {
            switch (level) {
                case LogLevel::Trace: return "TRACE";
                case LogLevel::Info:  return "INFO ";
                case LogLevel::Warn:  return "WARN ";
                case LogLevel::Error: return "ERROR";
            }
            return "?";
        }

        const char* LevelColor(LogLevel level) {
            switch (level) {
                case LogLevel::Trace: return "\x1b[90m";
                case LogLevel::Info:  return "\x1b[0m";
                case LogLevel::Warn:  return "\x1b[33m";
                case LogLevel::Error: return "\x1b[31m";
            }
            return "\x1b[0m";
        }
    }

    void SetLogLevel(LogLevel minLevel) { s_MinLevel = minLevel; }

    namespace detail {
        void WriteLog(LogLevel level, const std::string& message) {
            if (level < s_MinLevel) return;
            static const bool useColor = DetectColorSupport();

            const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - s_StartTime).count();
            char stamp[32];
            std::snprintf(stamp, sizeof(stamp), "%8.3f", seconds);

            std::lock_guard<std::mutex> lock(s_Mutex);
            if (useColor) std::cout << LevelColor(level);
            std::cout << '[' << stamp << "] [" << LevelTag(level) << "] " << message;
            if (useColor) std::cout << "\x1b[0m";
            std::cout << std::endl;
        }

        void AssertFailed(const char* expr, const char* file, int line, const std::string& message) {
            WriteLog(LogLevel::Error, Concat("Assert basarisiz: ", expr, " (", file, ":", line, ") ", message));
            std::abort();
        }
    }
}
