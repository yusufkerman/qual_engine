#pragma once

/**
 * @file shader_manager.h
 * @brief [LEGACY] Basit, ID tabanlı shader depolama ve yönetim sınıfı.
 */

#include "shader.h"
#include <unordered_map>

 /**
  * @class shader_manager
  * @brief [LEGACY] Eski motor mimarisinde shader nesnelerini bellek üzerinde tutan yapı.
  * @deprecated Aria Asset System'in tam devreye girmesi ile bu yönetici devredışı bırakılmıştır.
  * Bunun yerine aria_system kullanılmalıdır.
  */
class [[deprecated("Shader yonetimi artik aria_system tarafindan yapilmaktadir.")]] shader_manager
{
public:
    shader_manager() = default;
    ~shader_manager() = default;

    /**
     * @brief Belirtilen yollardan shader yükler ve ID'sini döner.
     */
    unsigned int load(const char* vs, const char* fs);

    /**
     * @brief Belirtilen ID'ye karşılık gelen shader nesnesine referans döner.
     */
    shader& get(unsigned int id);

private:
    std::unordered_map<unsigned int, shader> shaders;
    unsigned int nextID = 0;
};