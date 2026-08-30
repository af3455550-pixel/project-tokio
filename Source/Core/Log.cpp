#include "Core/Log.h"
#include <cstdio>
#include <atomic>
#include <string>
#include <utility>

namespace ink {

namespace {
std::atomic<int> g_errorCount{0};
bool g_quiet = false;
} // namespace

void LogWrite(LogLevel level, const std::string& msg) {
    if (g_quiet && level == LogLevel::Info)
        return;
    const char* tag = (level == LogLevel::Info) ? "[info] " : (level == LogLevel::Warn) ? "[warn] " : "[err]  ";
    // stderr: unbuffered, so crash-time logs survive.
    std::fprintf(stderr, "%s%s\n", tag, msg.c_str());
    if (level == LogLevel::Error)
        g_errorCount.fetch_add(1);
}

void LogSetQuiet(bool quiet) { g_quiet = quiet; }

int LogErrorCount() { return g_errorCount.load(); }
void LogResetErrorCount() { g_errorCount.store(0); }

} // namespace ink
