// Runner: executes every registered test, prints a summary, returns non-zero
// on failure so `ctest` / CI can gate on it.
#include "Test.h"

#include <string>

int main() {
    int ran = 0;
    for (const auto& c : inkt::Registry()) {
        const int before = inkt::Failures();
        std::printf("[test] %s\n", c.name);
        c.fn();
        ++ran;
        std::printf("       %s\n", inkt::Failures() == before ? "ok" : "FAILED");
    }
    std::printf("==== %d tests, %d failure(s) ====\n", ran, inkt::Failures());
    return inkt::Failures() == 0 ? 0 : 1;
}
