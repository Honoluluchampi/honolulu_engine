#pragma once

// hnll
#include <graphics/mesh_model.hpp>
#include <game/component.hpp>
#include <game/components/renderable_component.hpp>

// std
#include <unordered_map>

namespace hnll {
namespace game {

template<class U> using u_ptr = std::unique_ptr<U>;
template<class S> using s_ptr = std::shared_ptr<S>;

class mesh_component : public renderable_component
{
  public:
    // copy a passed shared_ptr
    mesh_component(const s_ptr<hnll::graphics::mesh_model>& mesh_model_sp) : renderable_component(render_type::SIMPLE), model_sp_(mesh_model_sp) {}
    mesh_component(s_ptr<hnll::graphics::mesh_model>&& mesh_model_sp) : renderable_component(render_type::SIMPLE), model_sp_(std::move(mesh_model_sp)) {}
    ~mesh_component(){}

    s_ptr<hnll::graphics::mesh_model>& get_model_sp() { return model_sp_; }
    template<class S>
    void set_mesh_model(S&& model) { model_sp_ = std::forward<S>(model); }
        
  private:
    void update_component(float dt) override { std::cout << this->transform_sp_->translation.x() << std::endl; }
    // hnll::graphics::mesh_model can be shared all over a game
    s_ptr<hnll::graphics::mesh_model> model_sp_ = nullptr;
};

using mesh_component_map = std::unordered_map<game::component_id, s_ptr<game::mesh_component>>;

} // namespace game
} // namespace hnll