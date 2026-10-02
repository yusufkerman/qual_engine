#pragma once

/**
 * @file material_manager.h
 * @brief Çalışma zamanı bellek içi materyal yöneticisi.
 * @deprecated Materyal yönetimi Aria Varlık Sistemi'ne (Aria Asset System) devredilmiştir.
 */

#include "material.h"
#include <unordered_map>

 /**
  * @class material_manager
  * @brief Eski bellek içi materyal deposu.
  * @deprecated Aria Asset System mimarisine geçildiği için kullanımdan kaldırılmıştır.
  */
class [[deprecated("Materyal islemleri icin Aria Asset System kullaniniz.")]] material_manager
{
public:
    material_manager() = default;
    ~material_manager() = default;

    /**
     * @brief Yeni bir materyal kaydeder ve kimlik döner.
     */
    unsigned int create(const material& mat);

    /**
     * @brief Kayıtlı tüm materyalleri döndürür.
     */
    [[nodiscard]] const std::unordered_map<unsigned int, material>& get_all_materials() const noexcept
    {
        return m_materials;
    }

    /**
     * @brief Kimliği verilen materyale referans döner.
     */
    material& get(unsigned int id);

    /**
     * @brief Kimliği verilen materyale salt okunur referans döner.
     */
    const material& get(unsigned int id) const;

    /**
     * @brief Tüm kayıtları temizler.
     */
    void clear() noexcept;

private:
    std::unordered_map<unsigned int, material> m_materials;
    unsigned int m_nextID = 1;
    static material s_defaultMaterial;
};