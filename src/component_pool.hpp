#ifndef ENTT_COMPONENT_POOL_HPP
#define ENTT_COMPONENT_POOL_HPP


#include <utility>
#include <vector>
#include <cassert>


namespace entt {


template<typename, typename, typename...>
class ComponentPool;

// Single-component pool
template<typename Entity, typename Component>
class ComponentPool<Entity, Component> {
public:
    using component_type = Component;
    using entity_type = Entity;
    using pos_type = entity_type;
    using size_type = typename std::vector<component_type>::size_type;

private:
    inline bool valid(entity_type entity) const noexcept {
        return entity < reverse.size() && reverse[entity] < direct.size() && direct[reverse[entity]] == entity;
    }

public:
    explicit ComponentPool(size_type dim = 4098) noexcept {
        assert(!(dim < 0));
        data.reserve(dim);
    }

    ComponentPool(ComponentPool &&) = default;

    ~ComponentPool() noexcept {
        assert(empty());
    }

    ComponentPool & operator=(ComponentPool &&) = default;

    // Returns whether the pool is empty
    bool empty() const noexcept {
        return data.empty();
    }

    // Returns the maximum number of components created until now
    size_type capacity() const noexcept {
        return data.capacity();
    }

    // Returns the current number of components
    size_type size() const noexcept {
        return data.size();
    }

    // Returns a direct pointer to the internal array of the Component Index -> Entity vector
    const entity_type * entities() const noexcept {
        return direct.data();
    }

    // Returns whether the pool contains a component associated with the given entity
    bool has(entity_type entity) const noexcept {
        return valid(entity);
    }

    // Returns a const reference to the component associated with the given entity
    const component_type & get(entity_type entity) const noexcept {
        assert(valid(entity));
        return data[reverse[entity]];
    }

    // Returns a reference to the component associated with the given entity
    component_type & get(entity_type entity) noexcept {
        return const_cast<component_type &>(const_cast<const ComponentPool *>(this)->get(entity));
    }

    // Constructs a component and associates it with the given entity
    template<typename... Args>
    component_type & construct(entity_type entity, Args... args) {
        assert(!valid(entity));

        if(!(entity < reverse.size())) {
            reverse.resize(entity+1);
        }

        reverse[entity] = pos_type(direct.size());
        direct.emplace_back(entity);
        data.push_back({ args... });

        return data.back();
    }

    // Associates a preconstruted component with the given entity (using the component's move constructor)
    component_type & construct(entity_type entity, component_type&& component)
    {
        assert(!valid(entity));

        if (!(entity < reverse.size()))
        {
            reverse.resize(entity + 1);
        }

        reverse[entity] = pos_type(direct.size());
        direct.emplace_back(entity);
        data.emplace_back(component);

        return data.back();
    }

    // Removes the component associated with the given entity
    void destroy(entity_type entity) {
        assert(valid(entity));

        auto last = direct.size() - 1;

        reverse[direct[last]] = reverse[entity];
        direct[reverse[entity]] = direct[last];
        data[reverse[entity]] = std::move(data[last]);

        direct.pop_back();
        data.pop_back();
    }

    // Removes all components and entity-component associations
    void reset() {
        data.clear();
        reverse.resize(0);
        direct.clear();
    }

private:
    std::vector<component_type> data;   // Component storage
    std::vector<pos_type> reverse;      // Entity -> Component Index map
    std::vector<entity_type> direct;    // Component Index -> Entity map
};

// Multi-component pool
template<typename Entity, typename Component, typename... Components>
class ComponentPool
        : ComponentPool<Entity, Component>, ComponentPool<Entity, Components>...
{
    template<typename Comp>
    using Pool = ComponentPool<Entity, Comp>;

public:
    using entity_type = typename Pool<Component>::entity_type;
    using pos_type = typename Pool<Component>::pos_type;
    using size_type = typename Pool<Component>::size_type;

    explicit ComponentPool(size_type dim = 4098) noexcept
#ifdef _MSC_VER
        : ComponentPool<Entity, Component>{dim}, ComponentPool<Entity, Components>{dim}...
#else
        : Pool<Component>{dim}, Pool<Components>{dim}...
#endif
    {
        assert(!(dim < 0));
    }

    ComponentPool(const ComponentPool &) = delete;
    ComponentPool(ComponentPool &&) = delete;

    ComponentPool & operator=(const ComponentPool &) = delete;
    ComponentPool & operator=(ComponentPool &&) = delete;

    // Returns whether the pool (of the templated component type) is empty
    template<typename Comp>
    bool empty() const noexcept {
        return Pool<Comp>::empty();
    }

    // Returns the maximum number of components (of the templated component type) created until now
    template<typename Comp>
    size_type capacity() const noexcept {
        return Pool<Comp>::capacity();
    }

    // Returns the current number of components (of the templated component type)
    template<typename Comp>
    size_type size() const noexcept {
        return Pool<Comp>::size();
    }

    // Returns a direct pointer to the internal array of the Component Index (of the templated component type) -> Entity vector
    template<typename Comp>
    const entity_type * entities() const noexcept {
        return Pool<Comp>::entities();
    }

    // Returns whether the pool contains a component (of the templated component type) associated with the given entity
    template<typename Comp>
    bool has(entity_type entity) const noexcept {
        return Pool<Comp>::has(entity);
    }

    // Returns a const reference to the component (of the templated component type) associated with the given entity
    template<typename Comp>
    const Comp & get(entity_type entity) const noexcept {
        return Pool<Comp>::get(entity);
    }

    // Returns a const reference to the component (of the templated component type) associated with the given entity
    template<typename Comp>
    Comp & get(entity_type entity) noexcept {
        return const_cast<Comp &>(const_cast<const ComponentPool *>(this)->get<Comp>(entity));
    }

    // Constructs a component (of the templated component type) and associates it with the given entity
    template<typename Comp, typename... Args>
    Comp & construct(entity_type entity, Args... args) {
        return Pool<Comp>::construct(entity, args...);
    }

    // Associates a preconstruted component with the given entity (using the component's move constructor)
    template<typename Comp>
    Comp & construct(entity_type entity, Comp&& component)
    {
        return Pool<Comp>::construct(entity, component);
    }

    // Removes the component (of the templated component type) associated with the given entity
    template<typename Comp>
    void destroy(entity_type entity) {
        Pool<Comp>::destroy(entity);
    }

    // Removes all components and entity-component associations (of the templated component type)
    template<typename Comp>
    void reset() {
        Pool<Comp>::reset();
    }

    // Removes all components and entity-component associations (for all component types stored in pool)
    void reset() {
        using accumulator_type = int[];
        Pool<Component>::reset();
        accumulator_type accumulator = { (Pool<Components>::reset(), 0)... };
        (void)accumulator;
    }
};


}


#endif // ENTT_COMPONENT_POOL_HPP
