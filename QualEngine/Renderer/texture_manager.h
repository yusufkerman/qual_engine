#pragma once
#include <string>
#include <unordered_map>

class texture_manager
{
public:
    bool has(unsigned int id) const;
    unsigned int load(const std::string& path);
    unsigned int get(unsigned int id);

    // 🔍 Serializer'ın dosya yollarını çekebilmesi için yeni fonksiyonlar
    const std::string& get_path(unsigned int id) const;
    const std::unordered_map<unsigned int, std::string>& get_all_paths() const { return texturePaths; }

    void clear();

private:
    std::unordered_map<unsigned int, unsigned int> textures; // ID -> OpenGL ID
    std::unordered_map<unsigned int, std::string> texturePaths; // ID -> Dosya Yolu (Yeni!)
    unsigned int nextID = 0;
};