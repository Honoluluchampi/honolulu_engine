#pragma once

// hnll
#include <game/actor.hpp>
#include <game/renderable_component.hpp>
#include <graphics/graphics_models/static_mesh.hpp>

namespace hnll::game {

// this actor has only static graphics component
using static_mesh_comp = renderable_comp<graphics::static_mesh>;
DEFINE_PURE_ACTOR(static_mesh_actor)
  {
    public:
    static_mesh_actor(const std::string& model_name)
    { mesh_model_ = game::static_mesh_comp ::create(*this, model_name); }

    game::static_mesh_comp& get_mesh_comp() { return *mesh_model_; }

    private:
    u_ptr<game::static_mesh_comp> mesh_model_;
  };

} // namespace hnll::game