#pragma once
#include <string>

// İleri bildirimler (Forward Declarations)
// Sadece pointer veya referans kullanacağımız için dosyaları include edip şişirmiyoruz.
class scene;
struct EngineContext;

class scene_serializer
{
public:
    // Sahneyi diske JSON formatında yazar
    static bool serialize(const std::string& filepath, const scene& currentScene, EngineContext& ctx);

    // Sahneyi diskten okur, mevcut sahneyi temizler ve yeniden inşa eder
    static bool deserialize(const std::string& filepath, scene& currentScene, EngineContext& ctx);
};