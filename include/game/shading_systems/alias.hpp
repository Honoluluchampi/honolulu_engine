#pragma once

// hnll
#include <game/shading_system.hpp>
#include <game/renderable_component.hpp>

namespace hnll::game {

using static_mesh_comp = renderable_comp<utils::shading_type::MESH>;
using static_meshlet_comp = renderable_comp<utils::shading_type::MESHLET>;

}