#include <doctest/doctest.h>
#include "Core/Logger/logger.h"
#include <thread>
#include <vector>

TEST_CASE("Engine Logger - Basic Logging and Level Formatting") {
    auto& log = qual::log::engine_logger::instance();
    log.clear();
    CHECK(log.get_log_count() == 0);

    log.log(qual::log::log_level::info, qual::log::log_category::core, __FILE__, __LINE__, 0, "Engine init test {}", 42);
    CHECK(log.get_log_count() == 1);

    std::vector<qual::log::log_entry> snap;
    log.get_snapshot(snap);
    REQUIRE(snap.size() == 1);
    CHECK(snap[0].level == qual::log::log_level::info);
    CHECK(snap[0].category == qual::log::log_category::core);
    CHECK(std::string(snap[0].message) == "Engine init test 42");
    CHECK(snap[0].repeat_count == 1);
}

TEST_CASE("Engine Logger - Consecutive Spam Deduplication") {
    auto& log = qual::log::engine_logger::instance();
    log.clear();

    const char* dummy_file = "test_loop.cpp";
    const uint32_t dummy_line = 100;

    // Simulate 50 calls with the same error in a loop
    for (int i = 0; i < 50; ++i) {
        log.log(qual::log::log_level::warn, qual::log::log_category::renderer,
                dummy_file, dummy_line, 0, "Shader compile warning");
    }

    // Must be deduplicated into 1 entry with repeat_count == 50
    CHECK(log.get_log_count() == 1);

    std::vector<qual::log::log_entry> snap;
    log.get_snapshot(snap);
    REQUIRE(snap.size() == 1);
    CHECK(snap[0].repeat_count == 50);
    CHECK(std::string(snap[0].message) == "Shader compile warning");
}

TEST_CASE("Engine Logger - Thread Safety under High Concurrency") {
    auto& log = qual::log::engine_logger::instance();
    log.clear();

    constexpr int NUM_THREADS = 4;
    constexpr int LOGS_PER_THREAD = 100;

    std::vector<std::thread> workers;
    workers.reserve(NUM_THREADS);

    for (int t = 0; t < NUM_THREADS; ++t) {
        workers.emplace_back([t, &log]() {
            for (int i = 0; i < LOGS_PER_THREAD; ++i) {
                log.log(qual::log::log_level::debug, qual::log::log_category::physics,
                        "worker.cpp", static_cast<uint32_t>(10 + t), 0,
                        "Thread {} step {}", t, i);
            }
        });
    }

    for (auto& w : workers) {
        w.join();
    }

    std::vector<qual::log::log_entry> snap;
    log.get_snapshot(snap);
    CHECK(snap.size() > 0);
    CHECK(snap.size() <= qual::log::engine_logger::BUFFER_CAPACITY);
}
