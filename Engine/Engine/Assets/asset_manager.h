#pragma once

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
// TYPE DEFINITIONS
// --------------------

using asset_id = uint32_t;
inline constexpr asset_id invalid_asset_id = std::numeric_limits<asset_id>::max();

// --------------------
// GENERIC STORAGE BASE
// --------------------

class IAssetStorage
{
public:
    virtual ~IAssetStorage() = default;
};

// --------------------
// TEMPLATE STORAGE
// --------------------

template<typename T>
class AssetStorage : public IAssetStorage
{
public:
    asset_id add(const T& asset)
    {
        if (data.size() >= static_cast<std::size_t>(std::numeric_limits<asset_id>::max()))
            throw std::overflow_error("AssetStorage full");

        data.push_back(asset);
        return static_cast<asset_id>(data.size() - 1);
    }

    T* try_get(asset_id id)
    {
        if (id >= data.size())
            return nullptr;

        return &data[id];
    }

    const T* try_get(asset_id id) const
    {
        if (id >= data.size())
            return nullptr;

        return &data[id];
    }

    T& get(asset_id id)
    {
        if (auto* ptr = try_get(id))
            return *ptr;

        throw std::out_of_range("Invalid asset_id");
    }

private:
    std::vector<T> data;
};

// --------------------
// ASSET MANAGER
// --------------------

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

    void clear()
    {
        storages.clear();
    }

private:
    template<typename T>
    AssetStorage<T>* get_storage_ptr() const
    {
        auto key = std::type_index(typeid(T));

        auto it = storages.find(key);
        if (it == storages.end())
            return nullptr;

        auto* ptr = dynamic_cast<AssetStorage<T>*>(it->second.get());

        // 🔥 CRASH PREVENTION
        assert(ptr && "Asset type mismatch detected!");
        return ptr;
    }

    template<typename T>
    AssetStorage<T>& get_storage()
    {
        auto key = std::type_index(typeid(T));

        auto it = storages.find(key);
        if (it != storages.end())
            return *static_cast<AssetStorage<T>*>(it->second.get());

        auto storage = std::make_unique<AssetStorage<T>>();
        auto* raw = storage.get();
        storages.emplace(key, std::move(storage));
        return *raw;
    }

private:
    std::unordered_map<std::type_index, std::unique_ptr<IAssetStorage>> storages;
};