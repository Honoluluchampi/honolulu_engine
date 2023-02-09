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
  requires(T a) { a.bind(); a.draw(); } &&
  requires(const T a) { a.get_rc_id(); a.get_shading_type(); };

template <typename T>
concept ShadingSystem =
  requires(T a, const utils::frame_info& frame_info) { a.render(frame_info); };

}} // namespace hnll::game