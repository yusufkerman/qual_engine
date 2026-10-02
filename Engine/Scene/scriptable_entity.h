#pragma once

/**
 * @file scriptable_entity.h
 * @brief Kullanıcı tanımlı C++ oyun betikleri (native script) için temel sınıf.
 */

#include <string>
#include <cassert>
#include "../ECS/registry.h"

struct EngineContext;

/**
 * @class scriptable_entity
 * @brief Varlıklara bağlanabilen ve oyun döngüsü olaylarını (create, update, destroy) dinleyen temel betik sınıfı.
 */
class scriptable_entity
{
public:
    virtual ~scriptable_entity() = default;

    /**
     * @brief Betiğin bağlı olduğu varlık kimliğini döner.
     */
    [[nodiscard]] entity get_entity() const { return m_entity; }

    /**
     * @brief Sahnenin ECS kayıt defterine (registry) erişim sağlar.
     */
    [[nodiscard]] registry* get_registry() const { return m_registry; }

    void set_entity(entity e) { m_entity = e; }
    void set_registry(registry* reg) { m_registry = reg; }

    /**
     * @brief Motor bağlamına (EngineContext) erişim sağlar.
     */
    [[nodiscard]] EngineContext& getContext() const { return *ctx; }
    void setContext(EngineContext* context) { ctx = context; }

    // Yaşam döngüsü (Lifecycle) fonksiyonları
    virtual void on_create() {}
    virtual void on_destroy() {}
    virtual void on_update(float delta_time) {}

    // Yardımcı fonksiyonlar

    /**
     * @brief Bağlı varlığın belirtilen bileşenine referans döner.
     * @tparam T İstenen bileşen türü.
     * @return Bileşenin referansı.
     */
    template<typename T>
    T& get_component()
    {
        T* ptr = m_registry->get_component<T>(m_entity);
        assert(ptr && "get_component: Bilesen bulunamadi!");
        return *ptr;
    }

    /**
     * @brief Sahnede belirtilen isme sahip ilk varlığı bulur.
     * @param name Aranacak varlığın adı (name_component değeri).
     * @return Varlık kimliği (Bulunamazsa 0 döner).
     */
    entity find_by_name(const std::string& name);

private:
    entity m_entity = 0;             ///< Betiğin bağlı olduğu varlık kimliği.
    registry* m_registry = nullptr;  ///< Sahne kayıt defteri referansı.
    EngineContext* ctx = nullptr;    ///< Motor bağlamı referansı.
};