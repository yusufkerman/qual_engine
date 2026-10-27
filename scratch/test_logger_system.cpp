#include <iostream>
#include <cassert>
#include <thread>
#include <vector>
#include <string_view>
#include <cstring>

#include "../Engine/Core/Logger/logger.h"

int main() {
    std::cout << "====================================================\n";
    std::cout << "[TEST] QualEngine Faz 2 Merkezi Logger Dogrulama Testi\n";
    std::cout << "====================================================\n\n";

    auto& logger = qual::log::engine_logger::instance();
    logger.clear();

    // 1. Temel Loglama & Seviye/Kategori Doğrulaması
    std::cout << "[Test 1] Temel log kayitlari ve formatlama...\n";
    QLOG_INFO(core, "Sistem baslatildi. Versiyon: {}", "0.1.0");
    QLOG_WARN(renderer, "Shader uyarisı: baglayici indeks {}", 2);
    QLOG_ERROR(physics, "Carpisma tespit edilemedi: {}", 404);
    QLOG_SCRIPT_ERROR(42, "Betik baslatilamadi: {}", "MovementScript");

    std::vector<qual::log::log_entry> snapshot;
    logger.get_snapshot(snapshot);

    assert(snapshot.size() == 4);
    assert(snapshot[0].level == qual::log::log_level::info);
    assert(snapshot[0].category == qual::log::log_category::core);
    assert(std::string_view(snapshot[0].message) == "Sistem baslatildi. Versiyon: 0.1.0");

    assert(snapshot[1].level == qual::log::log_level::warn);
    assert(snapshot[1].category == qual::log::log_category::renderer);

    assert(snapshot[2].level == qual::log::log_level::error);
    assert(snapshot[2].category == qual::log::log_category::physics);

    assert(snapshot[3].level == qual::log::log_level::error);
    assert(snapshot[3].category == qual::log::log_category::script);
    assert(snapshot[3].entity_id == 42);
    assert(std::string_view(snapshot[3].message) == "Betik baslatilamadi: MovementScript");
    std::cout << " -> Test 1 PASSED: Temel loglar ve Entity ID dogrulandi.\n\n";

    // 2. Log Seli (Flood / Spam) Dedup Mekanizması Doğrulaması
    std::cout << "[Test 2] Log seli deduplication (Spam engelleme) testi...\n";
    logger.clear();

    for (int i = 0; i < 1000; ++i) {
        // Aynı dosya ve satırdan 1000 defa aynı mesajı logla
        QLOG_ERROR(script, "Access Violation tespit edildi: nullptr dereference!");
    }

    logger.get_snapshot(snapshot);
    assert(snapshot.size() == 1);
    assert(snapshot[0].repeat_count == 1000);
    std::cout << " -> Test 2 PASSED: 1000 tekrar tek satira indirgendi (repeat_count = " << snapshot[0].repeat_count << ").\n\n";

    // 3. Halka Tampon (Circular Ring Buffer) Kapasite ve Sınır Aşımı Doğrulaması
    std::cout << "[Test 3] Halka tampon kapasite asimi (2048 sınır testi)...\n";
    logger.clear();

    for (size_t i = 0; i < 2100; ++i) {
        // Her seferinde farklı mesaj logla (dedup engellemek için i ekle)
        QLOG_INFO(core, "Girdi indeksi: {}", i);
    }

    logger.get_snapshot(snapshot);
    assert(snapshot.size() == qual::log::engine_logger::BUFFER_CAPACITY);
    // İlk elenenlerden sonra en eski girdinin indeksi 2100 - 2048 = 52 olmalı
    assert(std::string_view(snapshot.front().message) == "Girdi indeksi: 52");
    assert(std::string_view(snapshot.back().message) == "Girdi indeksi: 2099");
    std::cout << " -> Test 3 PASSED: Halka tampon 2048 sinirini korudu, en eski 52 girdi basariyla ezildi.\n\n";

    // 4. Coklu Is Parcacikli Guvenlik (Thread-Safety) Dogrulamasi
    std::cout << "[Test 4] Coklu is parcacikli eszamanlilik (Multi-thread stress test)...\n";
    logger.clear();

    constexpr int NUM_THREADS = 8;
    constexpr int LOGS_PER_THREAD = 100;
    std::vector<std::thread> workers;
    workers.reserve(NUM_THREADS);

    for (int t = 0; t < NUM_THREADS; ++t) {
        workers.emplace_back([t]() {
            for (int i = 0; i < LOGS_PER_THREAD; ++i) {
                QLOG_DEBUG(system, "Thread {} log {}", t, i);
            }
        });
    }

    for (auto& th : workers) {
        th.join();
    }

    logger.get_snapshot(snapshot);
    assert(snapshot.size() == NUM_THREADS * LOGS_PER_THREAD);
    std::cout << " -> Test 4 PASSED: 8 is parcacigi eszamanli 800 logu veri yarisi olmadan basariyla yazdi.\n\n";

    std::cout << "====================================================\n";
    std::cout << " [FAZ 2 HEPSI BASARILI] MERKEZI LOGGER %100 DOGRULANDI\n";
    std::cout << "====================================================\n";
    return 0;
}

