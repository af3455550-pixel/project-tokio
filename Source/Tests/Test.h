#pragma once
// Minimal unit-test framework (§72): auto-registering cases, zero deps.
// Each INK_TEST(name) self-registers at static-init time; the runner in
// TestMain.cpp executes them all and reports failures.
#include <cmath>
#include <cstdio>
#include <vector>

namespace inkt {

struct Case {
    const char* name;
    void (*fn)();
};
inline std::vector<Case>& Registry() {
    static std::vector<Case> r;
    return r;
}
inline int& Failures() {
    static int f = 0;
    return f;
}
inline void Report(bool ok, const char* what, const char* file, int line) {
    if (ok)
        return;
    ++Failures();
    std::printf("  [FAIL] %s (%s:%d)\n", what, file, line);
}
struct Reg {
    Reg(const char* name, void (*fn)()) { Registry().push_back({name, fn}); }
};

} // namespace inkt

#define INK_TEST(name)                                                                        \
    static void ink_test_fn_##name();                                                         \
    static ::inkt::Reg ink_test_reg_##name(#name, ink_test_fn_##name);                        \
    static void ink_test_fn_##name()

#define INK_REQUIRE(cond) ::inkt::Report(static_cast<bool>(cond), #cond, __FILE__, __LINE__)
#define INK_REQUIRE_EQ(a, b) ::inkt::Report((a) == (b), #a " == " #b, __FILE__, __LINE__)
#define INK_REQUIRE_NEAR(a, b, eps)                                                           \
    ::inkt::Report(std::abs(static_cast<double>(a) - static_cast<double>(b)) <= (eps),        \
                   #a " ~= " #b, __FILE__, __LINE__)
