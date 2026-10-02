#pragma once

/**
 * @file asset_manager.h
 * @brief (LEGACY) Eski nesil varlık yönetim sistemi.
 * @deprecated Bu modül yerini daha gelişmiş olan 'Aria' (aria::system) varlık boru hattına bırakmıştır.
 * Yalnızca geriye dönük uyumluluk (backward compatibility) için tutulmaktadır.
 */

#include <cassert>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

 // --------------------
 // TİP TANIMLAMALARI (LEGACY)
 // --------------------

 /**
  * @typedef asset_id
  * @brief Eski nesil sistemde varlıkları tanımlamak için kullanılan basit tam sayı indeksi.
  */
using asset_id = uint32_t;

/**
 * @brief Geçersiz bir varlık indeksini temsil eder.
 */
inline constexpr asset_id invalid_asset_id = std::numeric_limits<asset_id>::max();

// --------------------
// GENEL DEPOLAMA ARAYÜZÜ
// --------------------

/**
 * @class IAssetStorage
 * @brief Farklı tipteki varlık depolarını tek bir polimorfik listede tutabilmek için kullanılan temel arayüz.
 */
class IAssetStorage
{
public:
    virtual ~IAssetStorage() = default;
};

// --------------------
// ŞABLON TABANLI DEPO
// --------------------

/**
 * @class AssetStorage
 * @brief Belirli bir tipteki varlıkları (std::vector kullanarak) ardışık bellekte saklayan depo sınıfı.
 * @tparam T Saklanacak varlığın tipi.
 */
template<typename T>
class AssetStorage : public IAssetStorage
{
public:
    /**
     * @brief Yeni bir varlığı depoya ekler.
     * @param asset Eklenecek varlık.
     * @return asset_id Eklenen varlığın dizin indeksi.
     */
    asset_id add(const T& asset)
    {
        if (data.size() >= static_cast<std::size_t>(std::numeric_limits<asset_id>::max()))
        {
            throw std::overflow_error("AssetStorage kapasitesi doldu.");
        }

        data.push_back(asset);
        return static_cast<asset_id>(data.size() - 1);
    }

    /**
     * @brief Belirtilen ID'ye sahip varlığı güvenli bir şekilde döndürür.
     * @param id Varlığın kimliği.
     * @return T* Varlık bulunursa işaretçisi, bulunamazsa nullptr döner.
     */
    T* try_get(asset_id id)
    {
        if (id >= data.size())
            return nullptr;

        return &data[id];
    }

    /**
     * @brief Belirtilen ID'ye sahip varlığı güvenli bir şekilde (salt okunur) döndürür.
     */
    const T* try_get(asset_id id) const
    {
        if (id >= data.size())
            return nullptr;

        return &data[id];
    }

    /**
     * @brief Belirtilen ID'ye sahip varlığın referansını döndürür (Bulunamazsa hata fırlatır).
     */
    T& get(asset_id id)
    {
        if (auto* ptr = try_get(id))
            return *ptr;

        throw std::out_of_range("Gecersiz asset_id");
    }

private:
    std::vector<T> data; ///< Varlıkları bellek üzerinde bitişik (contiguous) tutan dizi.
};

// --------------------
// ESKİ NESİL VARLIK YÖNETİCİSİ
// --------------------

/**
 * @class asset_manager
 * @brief (LEGACY) Tiplere göre ayrılmış alt depoları (AssetStorage) yöneten merkezi sınıf.
 */
class asset_manager
{
public:
    template<typename T>
    asset_id add(const T& asset)
    {
        return get_storage<T>().add(asset);
    }

    template<typename T>
    T* try_get(asset_id id)
    {
        auto* storage = get_storage_ptr<T>();
        return storage ? storage->try_get(id) : nullptr;
    }

    template<typename T>
    const T* try_get(asset_id id) const
    {
        auto* storage = get_storage_ptr<T>();
        return storage ? storage->try_get(id) : nullptr;
    }

    template<typename T>
    T& get(asset_id id)
    {
        return get_storage<T>().get(id);
    }

    template<typename T>
    bool contains(asset_id id)
    {
        return try_get<T>(id) != nullptr;
    }

    /**
     * @brief Kayıtlı tüm varlıkları bellekten temizler.
     */
    void clear()
    {
        storages.clear();
    }

private:
    /**
     * @brief İstenen tipe ait deponun işaretçisini döndürür.
     */
    template<typename T>
    AssetStorage<T>* get_storage_ptr() const
    {
        auto key = std::type_index(typeid(T));

        auto it = storages.find(key);
        if (it == storages.end())
            return nullptr;

        auto* ptr = dynamic_cast<AssetStorage<T>*>(it->second.get());

        // Dinamik tip dönüşümünde (cast) hata olursa programın çökmesini önlemek için doğrulama
        assert(ptr && "Varlik tipi uyumsuzlugu (Asset type mismatch) tespit edildi!");
        return ptr;
    }

    /**
     * @brief İstenen tipe ait depoyu bulur, yoksa yeni oluşturup referansını döndürür.
     */
    template<typename T>
    AssetStorage<T>& get_storage()
    {
        auto key = std::type_index(typeid(T));

        auto it = storages.find(key);
        if (it != storages.end())
        {
            return *static_cast<AssetStorage<T>*>(it->second.get());
        }

        auto storage = std::make_unique<AssetStorage<T>>();
        auto* raw = storage.get();
        storages.emplace(key, std::move(storage));
        return *raw;
    }

private:
    /// Her bir C++ tipine (type_index) karşılık gelen depoları tutan harita.
    std::unordered_map<std::type_index, std::unique_ptr<IAssetStorage>> storages;
};