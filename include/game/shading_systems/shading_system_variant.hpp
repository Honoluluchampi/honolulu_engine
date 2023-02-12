#pragma once

// hnll
#include <game/shading_systems/grid_shading_system.hpp>

// std
#include <variant>

namespace hnll::game {

// variant
template<ShadingSystem ...Args>
using shading_system_variant_base = std::variant<u_ptr<Args>...>;

using shading_system_variant = shading_system_variant_base<
  grid_shading_system
>;

} // namespace hnll::game