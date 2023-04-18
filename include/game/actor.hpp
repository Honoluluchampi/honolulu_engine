#pragma once

// hnll
#include <utils/common_alias.hpp>
#include <utils/utils.hpp>
#include <game/concepts.hpp>

// std
#include <unordered_map>
#include <concepts>
#include <variant>

// macro for crtp
#define DEFINE_ACTOR(new_actor, ...) class new_actor : public game::actor_base<new_actor, __VA_ARGS__>
#define DEFINE_PURE_ACTOR(new_actor) class new_actor : public game::pure_actor_base<new_actor>

namespace hnll::game {

static actor_id id_pool = 0;

template <typename Derived, UpdatableComponent... UpdatableComponents>
class actor_base
{
    using updatable_components_map = std::unordered_map<component_id, std::variant<u_ptr<UpdatableComponents>...>>;
  public:
    template<typename... Args>
    static u_ptr<Derived> create(Args&& ...args)
    {
      auto ret = std::make_unique<Derived>(std::forward<Args>(args)...);
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
    inline const utils::transform& get_transform() const { return transform_; }
    void set_rotation(const vec3& rot) { transform_.rotation = rot; }
    void add_rotation(const vec3& rot) { transform_.rotation += rot; }
    void set_translation(const vec3& tra) { transform_.translation = tra; }

  protected:
    // specialize this function for each actor class
    void update_this(const float& dt) {}

    actor_id id_;

    updatable_components_map updatable_components_;

    utils::transform transform_ {};
};

template <typename Derived>
class pure_actor_base
{
  public:
    template<typename... Args>
    static u_ptr<Derived> create(Args&& ...args)
    {
      auto ret = std::make_unique<Derived>(std::forward<Args>(args)...);
      // assign unique id
      ret->id_ = id_pool++;
      // add to game engine's update list
      return ret;
    }

    inline void update(const float& dt)
    { static_cast<Derived*>(this)->update_this(dt); }

    inline actor_id get_actor_id() const { return id_; }
    inline const utils::transform& get_transform() const { return transform_; }

    void set_transform(const utils::transform& tf) { transform_ = tf; }
    void set_rotation(const vec3& rot) { transform_.rotation = rot; }
    void add_rotation(const vec3& rot) { transform_.rotation += rot; }
    void set_translation(const vec3& tra) { transform_.translation = tra; }

  protected:
    // specialize this function for each actor class
    void update_this(const float& dt) {}

    actor_id id_;

    utils::transform transform_ {};
};

DEFINE_PURE_ACTOR(dummy_actor)
{

};

} // namespace hnll::game