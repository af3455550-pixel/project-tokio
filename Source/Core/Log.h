#pragma once
// Lightweight structured logging with an error counter (graceful-failure policy, §74).
#include <cstddef>
#include <initializer_list>
#include <string>

namespace ink {

enum class LogLevel { Info, Warn, Error };

void LogWrite(LogLevel level, const std::string& msg);
void LogSetQuiet(bool quiet);
int LogErrorCount();
void LogResetErrorCount();

// Substitutes "{}" placeholders in `fmt` with the args, in order.
// std::string args are inserted verbatim; everything else via std::to_string.
namespace detail {
inline void AppendArg(std::string* out, const std::string& a) { out->append(a); }
inline void AppendArg(std::string* out, const char* a) { out->append(a ? a : ""); }
template <typename T>
inline void AppendArg(std::string* out, const T& v) {
    out->append(std::to_string(v));
}
template <typename T, typename... Rest>
std::string DoFormat(const std::string& fmt, const T& first, const Rest&... rest) {
    std::size_t pos = fmt.find("{}");
    std::string head = fmt.substr(0, pos);
    std::string tail = pos == std::string::npos ? fmt : fmt.substr(pos + 2);
    std::string argStr;
    AppendArg(&argStr, first);
    if constexpr (sizeof...(Rest) == 0)
        return head + argStr + tail;
    else
        return head + argStr + DoFormat(tail, rest...);
}
} // namespace detail

template <typename... Args>
std::string LogFormat(const std::string& fmt, const Args&... args) {
    std::string out = fmt;
    (void)std::initializer_list<int>{(out = detail::DoFormat(out, args), 0)...};
    return out;
}

} // namespace ink

#define INK_LOG_INFO(...) ::ink::LogWrite(::ink::LogLevel::Info, ::ink::LogFormat(__VA_ARGS__))
#define INK_LOG_WARN(...) ::ink::LogWrite(::ink::LogLevel::Warn, ::ink::LogFormat(__VA_ARGS__))
#define INK_LOG_ERROR(...) ::ink::LogWrite(::ink::LogLevel::Error, ::ink::LogFormat(__VA_ARGS__))

#define INK_LINE_STR(n) #n
#define INK_LOC_STR(loc) __FILE__ ":" INK_LINE_STR(loc)

#ifdef INK_ASSERTS
#include <cstdlib>
#define INK_ASSERT(cond, msg)                                                     \
    do {                                                                          \
        if (!(cond)) {                                                            \
            ::ink::LogWrite(::ink::LogLevel::Error,                               \
                            std::string("ASSERT FAILED: ") + (msg) + " @ " + INK_LOC_STR(__LINE__)); \
            std::abort();                                                         \
        }                                                                         \
    } while (0)
#else
#define INK_ASSERT(cond, msg) ((void)0)
#endif
