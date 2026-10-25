#pragma once
#include <functional>
#include <sstream>
#include <string>
#include <utility>

namespace mme {

    enum class LogLevel { Trace, Info, Warn, Error };

    // Bu seviyenin altındaki mesajlar yazılmaz (varsayılan: Info)
    void SetLogLevel(LogLevel minLevel);

    // Konsola yazılan her mesajın bir kopyası dinleyicilere de gider (ör. editör
    // konsol paneli). Dinleyici hangi thread'den log atıldıysa orada çağrılır.
    using LogSink = std::function<void(LogLevel level, const std::string& message)>;
    int AddLogSink(LogSink sink);       // dönen kimlikle kaldırılır
    void RemoveLogSink(int id);

    namespace detail {
        void WriteLog(LogLevel level, const std::string& message);
        [[noreturn]] void AssertFailed(const char* expr, const char* file, int line, const std::string& message);
    }

    // Akışa yazılabilen her şeyi birleştirir: MME_INFO("Pencere ", w, "x", h);
    template<typename... Args>
    void Log(LogLevel level, Args&&... args) {
        std::ostringstream ss;
        (ss << ... << std::forward<Args>(args));
        detail::WriteLog(level, ss.str());
    }

    template<typename... Args>
    std::string Concat(Args&&... args) {
        std::ostringstream ss;
        (ss << ... << std::forward<Args>(args));
        return ss.str();
    }
}

#define MME_TRACE(...) ::mme::Log(::mme::LogLevel::Trace, __VA_ARGS__)
#define MME_INFO(...)  ::mme::Log(::mme::LogLevel::Info,  __VA_ARGS__)
#define MME_WARN(...)  ::mme::Log(::mme::LogLevel::Warn,  __VA_ARGS__)
#define MME_ERROR(...) ::mme::Log(::mme::LogLevel::Error, __VA_ARGS__)

// Debug derlemede koşul yanlışsa mesajı yazar ve programı durdurur.
// Kullanım: MME_ASSERT(ptr != nullptr, "ptr bos olamaz, id=", id);
#ifndef NDEBUG
#define MME_ASSERT(cond, ...) \
    do { if (!(cond)) ::mme::detail::AssertFailed(#cond, __FILE__, __LINE__, ::mme::Concat(__VA_ARGS__)); } while (0)
#else
#define MME_ASSERT(cond, ...) ((void)0)
#endif
