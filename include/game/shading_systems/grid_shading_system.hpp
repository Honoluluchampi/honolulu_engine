#pragma once

// hnll
#include <game/shading_system.hpp>

namespace hnll::game {

using grid_shading_system = shading_system<dummy_renderable_comp<utils::shading_type::GRID>>;

}