// hnll
#include <game/engine.hpp>
#include <game/actors/default_camera.hpp>
#include <game/shading_systems/grid_shading_system.hpp>
#include <game/shading_systems/static_mesh_shading_system.hpp>
#include <game/actors/basic_actors.hpp>
#include <physics/mass_spring_cloth.hpp>
#include <physics/compute_shader/cloth_compute_shader.hpp>
#include <physics/shading_system/cloth_compute_shading_system.hpp>
#include <imgui/imgui.h>

namespace hnll {

SELECT_SHADING_SYSTEM(
  game::grid_shading_system,
  game::static_mesh_shading_system,
  physics::cloth_compute_shading_system);

SELECT_ACTOR(game::default_camera);

SELECT_COMPUTE_SHADER(physics::cloth_compute_shader);

DEFINE_ENGINE(cloth_simulation)
{
  public:
    cloth_simulation()
    {
      add_update_target_directly<game::default_camera>();
      // add cloth
      cloth_ = physics::mass_spring_cloth::create(32, 32, 5, 5);
      // add sphere
      sphere_ = game::static_mesh_actor::create("smooth_sphere.obj");
      sphere_->set_translation(vec3(0.f, -1.f, 0.f));
      sphere_->set_rotation(vec3(0.f, 1.6f, 0.f));
      add_render_target<game::static_mesh_shading_system>(sphere_->get_mesh_comp());
    }
    ~cloth_simulation() {}

    void update_this(const float& dt)
    {
      dt_ = dt;
      ImGui::Begin("fps");
      ImGui::Text("fps : %d", static_cast<int>(1.f / dt_));
      if (cloth_ != nullptr) {
        ImGui::Text("x grid : %d", cloth_->get_x_grid());
        ImGui::Text("y grid : %d", cloth_->get_y_grid());
        if (ImGui::Button("unbind")) {
          cloth_->unbind();
        }
      }
      if (ImGui::Button("restart")){
        cloth_.reset();
        cloth_ = physics::mass_spring_cloth::create(32, 32, 5, 5);
      }
      ImGui::End();
    }

    void cleanup() { cloth_.reset(); }
  private:
    u_ptr<game::static_mesh_actor> sphere_;
    s_ptr<physics::mass_spring_cloth> cloth_;
    float dt_;
};

}

int main()
{
  hnll::cloth_simulation engine;

  try { engine.run(); }
  catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
}