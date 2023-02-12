#pragma once

// hnll
#include <utils/common_alias.hpp>
#include <game/concepts.hpp>

// std
#include <unordered_map>
#include <concepts>
#include <variant>

namespace hnll::game {

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
      static actor_id id = 0;
      ret->id_ = id++;
      // add to game engine's update list
      return ret;
    }

    void update(const float& dt)
    {
      update_this(dt);
      for (const auto& c : updatable_components_) {
        std::visit([&dt](auto& comp) { comp->update(dt); }, c.second);
      }
    }

    actor_id get_actor_id() const { return id_; }

  private:
    // specialize this function for each actor class
    void update_this(const float& dt) {}

    actor_id id_;

    updatable_components_map updatable_components_;
};

} // namespace hnll::game