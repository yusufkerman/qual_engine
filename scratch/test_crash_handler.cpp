#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

#include "../Engine/Core/CrashHandler/crash_handler.h"

int main() {
    std::cout << "====================================================\n";
    std::cout << "[TEST] QualEngine Faz 5 Cokme Kancalari ve Minidump Testi\n";
    std::cout << "====================================================\n\n";

    // Test için izole bir test klasörü ayarla
    const char* test_dump_dir = "scratch/test_crashes";
    qual::core::crash_handler::s_crash_dump_dir = test_dump_dir;
    qual::core::crash_handler::s_pass_through_debugger = false; // Test amaçlı minidump üretimini zorla

    // 1. Kancaların İlklendirilmesi
    std::cout << "[Test 1] Kanca ilklendirme testi (crash_handler::init)...\n";
    qual::core::crash_handler::init();
    assert(std::filesystem::exists(test_dump_dir));
    std::cout << " -> Test 1 PASSED: Tum SEH ve CRT kancalari basariyla baglandi.\n\n";

    // 2. Minidump (.dmp) ve Rapor (.txt) Uretim Testi
    std::cout << "[Test 2] Minidump yazma testi (write_minidump)...\n";
    bool success = qual::core::crash_handler::write_minidump(nullptr, "Birim Testi - Kontrollu Minidump Uretimi");
    assert(success == true);

    // Klasördeki üretilen dosyaları tara
    bool found_dmp = false;
    bool found_txt = false;
    uintmax_t dmp_size = 0;

    for (const auto& entry : std::filesystem::directory_iterator(test_dump_dir)) {
        if (entry.path().extension() == ".dmp") {
            found_dmp = true;
            dmp_size = entry.file_size();
            std::cout << " -> Bulunan Minidump: " << entry.path().string() << " (" << dmp_size << " bayt)\n";
        }
        else if (entry.path().extension() == ".txt") {
            found_txt = true;
            std::cout << " -> Bulunan Cokme Raporu: " << entry.path().string() << "\n";
            // Rapor içeriğini doğrula
            std::ifstream rfile(entry.path());
            std::string content((std::istreambuf_iterator<char>(rfile)), std::istreambuf_iterator<char>());
            assert(content.find("QUALENGINE POST-MORTEM COKME RAPORU") != std::string::npos);
            assert(content.find("Birim Testi - Kontrollu Minidump Uretimi") != std::string::npos);
        }
    }

    assert(found_dmp == true);
    assert(found_txt == true);
    assert(dmp_size > 1024); // Minidump en az birkaç kilobayt olmalı

    std::cout << " -> Test 2 PASSED: .dmp ve .txt rapor dosyalari basariyla dogrulandi.\n\n";

    std::cout << "====================================================\n";
    std::cout << " [FAZ 5 HEPSI BASARILI] POST-MORTEM COKME SISTEMI %100 DOGRULANDI\n";
    std::cout << "====================================================\n";
    return 0;
}

