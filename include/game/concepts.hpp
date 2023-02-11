#pragma once

#include <concepts>

namespace hnll::game {

template <typename T>
concept Updatable = requires (T a, const float& dt) { a.update(dt); };

template <typename T>
concept Actor = requires (T t) {
  requires Updatable<decltype(t)> &&
  requires { t.get_actor_id(); };
};

template <typename T>
concept Component = requires (T t) { t.get_component_id(); };

template <typename T>
concept UpdatableComponent = requires (T t) {
  requires Updatable<decltype(t)> &&
           Component<decltype(t)>;
};

using component_id = unsigned int;
using actor_id     = unsigned int;

}