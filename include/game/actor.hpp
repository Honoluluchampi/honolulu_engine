#pragma once

// hnll
#include <utils/common_alias.hpp>
#include <game/concepts.hpp>

// std
#include <unordered_map>
#include <concepts>
#include <variant>

// macro for crtp
#define DEFINE_ACTOR(new_actor, ...) class new_actor : public game::actor_base<new_actor, __VA_ARGS__>
#define DEFINE_PURE_ACTOR(new_actor, ...) class new_actor : public game::pure_actor_base<new_actor>

namespace hnll::game {

static actor_id id_pool = 0;

template <typename Derived, UpdatableComponent... UpdatableComponents>
class actor_base
{
    using updatable_components_map = std::unordered_map<component_id, std::variant<u_ptr<UpdatableComponents>...>>;
  public:
    template<typename... Args>
    static u_ptr<actor_base<UpdatableComponents...>> create(Args&& ...args)
    {
      auto ret = std::make_unique<actor_base<UpdatableComponents...>>(std::forward<Args>(args)...);
      // assign unique id
      ret->id_ = id_pool++;
      // add to game engine's update list
      return ret;
    }

    void update(const float& dt)
    {
      static_cast<Derived*>(this)->update_this(dt);
      for (const auto& c : updatable_components_) {
        std::visit([&dt](auto& comp) { comp->update(dt); }, c.second);
      }
    }

    inline actor_id get_actor_id() const { return id_; }

  private:
    // specialize this function for each actor class
    void update_this(const float& dt) {}

    actor_id id_;

    updatable_components_map updatable_components_;
};

template <typename Derived>
class pure_actor_base
{
  public:
    template<typename... Args>
    static u_ptr<actor_base<Derived>> create(Args&& ...args)
    {
      auto ret = std::make_unique<actor_base<Derived>>(std::forward<Args>(args)...);
      // assign unique id
      ret->id_ = id_pool++;
      // add to game engine's update list
      return ret;
    }

    inline void update(const float& dt)
    { static_cast<Derived*>(this)->update_this(dt); }

    inline actor_id get_actor_id() const { return id_; }

  private:
    // specialize this function for each actor class
    void update_this(const float& dt) {}

    actor_id id_;
};

} // namespace hnll::game