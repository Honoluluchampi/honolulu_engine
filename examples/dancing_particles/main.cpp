// hnll
#include <game/engine.hpp>
#include <game/shading_systems/grid_shading_system.hpp>
#include <game/shading_systems/static_mesh_shading_system.hpp>
#include <game/actor.hpp>
#include <game/actors/default_camera.hpp>

namespace hnll {

DEFINE_PURE_ACTOR(mesh_particle)
{
  public:
    mesh_particle()
    {
      sphere_mesh_ = game::static_mesh_comp::create(*this, "smooth_sphere.obj");
      set_translation({0.f, -1.f, 0.f});
    }

    game::static_mesh_comp& get_mesh_comp() { return *sphere_mesh_; }

  private:
    u_ptr<game::static_mesh_comp> sphere_mesh_;
};

// grid and mesh shader
SELECT_SHADING_SYSTEM(game::grid_shading_system, game::static_mesh_shading_system);
// particle and camera
SELECT_ACTOR(mesh_particle, game::default_camera);
// no compute task
SELECT_COMPUTE_SHADER();

DEFINE_ENGINE(dancing_particles)
{
  public:
    explicit ENGINE_CTOR(dancing_particles)
    {
      add_update_target_directly<game::default_camera>();
      particle_ = std::make_unique<mesh_particle>();
      add_render_target<game::static_mesh_shading_system>(particle_->get_mesh_comp());
    }

    void update_this(const float& dt)
    {
      ImGui::Begin("stats");
      ImGui::Text("fps : %d", int(1.f / dt));
      ImGui::End();
    }

  private:
    u_ptr<mesh_particle> particle_;
};

} // namespace hnll

int main()
{
  auto app = hnll::dancing_particles("dancing particles");

  try { app.run(); }
  catch (const std::exception& e) { std::cerr << e.what() << std::endl; }
}