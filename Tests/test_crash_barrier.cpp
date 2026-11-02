#include <doctest/doctest.h>
#include "Scripting/script_invoker.h"

TEST_CASE("Script Crash Barrier - Normal Execution") {
    bool executed = false;
    std::string err;

    bool ok = qual::scripting::script_invoker::invoke("TestScript", 1, "on_update", [&]() {
        executed = true;
    }, &err);

    CHECK(ok);
    CHECK(executed);
    CHECK(err.empty());
}

#ifdef _WIN32
TEST_CASE("Script Crash Barrier - Hardware Exception Containment") {
    // Force crash barrier enabled and do not pass through to debugger
    qual::scripting::script_invoker::s_boundary_enabled = true;
    qual::scripting::script_invoker::s_pass_through_when_debugging = false;

    uint32_t prev_crashes = qual::scripting::script_invoker::s_crashes_prevented_count;

    SUBCASE("Null Pointer Dereference (Access Violation 0xC0000005) is caught safely") {
        std::string err;
        bool ok = qual::scripting::script_invoker::invoke("NullDereferenceScript", 42, "on_update", [&]() {
            volatile int* null_ptr = nullptr;
            volatile int val = *null_ptr;
            (void)val;
        }, &err);

        CHECK_FALSE(ok);
        CHECK(qual::scripting::script_invoker::s_crashes_prevented_count == prev_crashes + 1);
        CHECK_FALSE(err.empty());
        CHECK(err.find("0xC0000005") != std::string::npos);
    }

    SUBCASE("Integer Division by Zero (0xC0000094) is caught safely") {
        std::string err;
        bool ok = qual::scripting::script_invoker::invoke("DivZeroScript", 99, "on_update", [&]() {
            volatile int zero = 0;
            volatile int result = 100 / zero;
            (void)result;
        }, &err);

        CHECK_FALSE(ok);
        CHECK_FALSE(err.empty());
        CHECK(err.find("0xC0000094") != std::string::npos);
    }
}
#endif

