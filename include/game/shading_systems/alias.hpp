#pragma once

// hnll
#include <game/shading_system.hpp>
#include <game/renderable_component.hpp>

namespace hnll::game {

using static_mesh_comp = renderable_comp<utils::shading_type::MESH>;

using grid_shading_system = shading_system<dummy_renderable_comp<utils::shading_type::GRID>>;
using static_mesh_shading_system = shading_system<renderable_comp<utils::shading_type::MESH>>;
}