#pragma once

// std
#include <concepts>
// lib
#include <vulkan/vulkan.h>

namespace hnll {

namespace utils {
  class graphics_frame_info;
  class compute_frame_info;
}

namespace graphics {

class device;

template <typename T, typename... Args>
concept GraphicsModel =
requires(T a, VkCommandBuffer cb, Args... args) { a.bind(cb, args...); a.draw(cb, args...); } &&
requires(const T a) { T::get_shading_type(); a.is_textured(); };

}

namespace game {

struct physics_frame_info;

template <typename T>
concept Updatable = requires (T a, const float& dt) { a.update(dt); };

template <typename T>
concept Actor = requires (const T t) { requires
  requires { t.get_actor_id(); };
} &&
requires (T t){
  requires Updatable<decltype(t)> &&
  requires { t.get_transform(); };
};

template <typename T>
concept Component = requires (T t) { t.get_component_id(); };

template <typename T>
concept UpdatableComponent = requires (T t) {
  requires Updatable<decltype(t)> &&
           Component<decltype(t)>;
};

template <typename T, typename... Args>
concept RenderableComponent =
requires(T a, VkCommandBuffer cb, Args... args) { a.bind(cb, args...); a.draw(cb, args...); } &&
requires(const T a) { a.get_rc_id(); a.get_shading_type(); };

template <typename T>
concept ShadingSystem =
requires(T a, const utils::graphics_frame_info& frame_info) { a.render(frame_info); } &&
requires(T, graphics::device& device) { T::create(device); };

template <typename T>
concept ComputeShader = requires (T t, const utils::compute_frame_info& info) {
  t.render(info);
};

using component_id = unsigned int;
using actor_id     = unsigned int;
using rc_id        = unsigned int;

}} // namespace hnll::game