#pragma once

// hnll
#include <utils/common_alias.hpp>

// std
#include <variant>

namespace hnll {

namespace utils { struct frame_info; }

namespace game {

// concepts
template <typename T>
concept GraphicalModel =
  requires(T a) { a.bind(); a.draw(); } &&
  requires(const T a) { a.get_shading_type(); };

template <typename T>
concept RenderableComponent =
  requires(T a) { a.add_to_shading_system(); a.remove_from_shading_system(); };

template <typename T>
concept ShadingSystem =
  requires(T a, const utils::frame_info& frame_info) { a.render(frame_info); };

class static_model_shading_system;
class frame_anim_shading_system;

// shading system variant
template<class ...Args>
  using shading_system_variant_base = std::variant<u_ptr<Args>...>;

using shading_system_variant = shading_system_variant_base<
  static_model_shading_system,
  frame_anim_shading_system
  >;

}} // namespace hnll::game