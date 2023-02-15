#pragma once

#include <concepts>

namespace hnll {

namespace utils { class frame_info; }
namespace graphics { class device; }

namespace game {

template <typename T>
concept Updatable = requires (T a, const float& dt) { a.update(dt); };

template <typename T>
concept Actor = requires (const T t) {
  requires Updatable<decltype(t)> &&
  requires { t.get_actor_id(); } &&
  requires { t.get_transform(); };
};

template <typename T>
concept Component = requires (T t) { t.get_component_id(); };

template <typename T>
concept UpdatableComponent = requires (T t) {
  requires Updatable<decltype(t)> &&
           Component<decltype(t)>;
};

template <typename T>
concept GraphicsModel =
requires(T a) { a.bind(); a.draw(); } &&
requires(const T a) { a.get_shading_type(); };

template <typename T>
concept RenderableComponent =
requires(T a) { a.bind(); a.draw(); } &&
requires(const T a) { a.get_rc_id(); a.get_shading_type(); };

template <typename T>
concept ShadingSystem =
requires(T a, const utils::frame_info& frame_info) { a.render(frame_info); } &&
requires(T, graphics::device& device) { T::create(device); };

using component_id = unsigned int;
using actor_id     = unsigned int;

}} // namespace hnll::game