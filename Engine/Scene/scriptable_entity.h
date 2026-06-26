#pragma once

#include "../ECS/registry.h"

struct EngineContext;

class scriptable_entity
{
public:
	virtual ~scriptable_entity() = default;

	entity get_entity() const { return m_entity; }
	registry* get_registry() const { return m_registry; }
	void set_entity(entity e) { m_entity = e;}
	void set_registry(registry* reg) { m_registry = reg; }

	EngineContext& getContext() { return *ctx; }
	void setContext(EngineContext* context) { ctx = context; }

	virtual void on_create() {}
	virtual void on_destroy() {}
	virtual void on_update(float delta_time) {}


	//Yardımcı fonksiyonlar
	template<typename T>
	T& get_component()
	{
		T* ptr = m_registry->get_component<T>(m_entity);
		assert(ptr && "get_component: component bulunamadı!");
		return *ptr;
	}

	entity find_by_name(const std::string& name);

	
private:
	entity m_entity = 0;
	registry* m_registry = nullptr;
	EngineContext* ctx = nullptr;

};