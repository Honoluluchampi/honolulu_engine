#pragma once

#include <game/renderable_component.hpp>

namespace hnll::game {

class mesh_model { public : void bind(){}; void draw(){}; utils::shading_type get_shading_type() const{}; };
using mesh_model_comp = renderable_comp<mesh_model>;

template <class RC>
class shading_system
{
    using target_map = std::unordered_map<uint32_t, RC&>;
  private:
    target_map target_map_;
};

using mesh_shading_system = shading_system<mesh_model_comp>;

} // namespace hnll::game