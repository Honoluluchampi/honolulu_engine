#pragma once

// hnll
#include <game/concepts.hpp>
#include <utils/common_alias.hpp>

// macro for crtp
#define DEFINE_COMPONENT(new_comp) class new_comp : public game::component_base<new_comp>
#define DEFINE_TEMPLATE_COMPONENT(new_comp, ...) class new_comp : public game::component_base<new_comp<__VA_ARGS__>>

namespace hnll::game {

template <typename Derived>
class component_base
{
  public:
    template <typename... Args>
    static u_ptr<Derived> create(Args&& ...args)
    {
      auto ret = std::make_unique<Derived>(std::forward<Args>(args)...);
      // assign unique id
      static component_id id = 0;
      ret->set_component_id(id);
      return ret;
    }

    // getter
    component_id get_component_id() const { return id_; }

    // setter
    void set_component_id(component_id id) { id_ = id; }

  private:
    component_id id_;
};

} // namespace hnll::game