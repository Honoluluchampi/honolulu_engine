// hnll
#include <game/engine.hpp>
#include <game/shading_systems/grid_shading_system.hpp>
#include <game/shading_systems/static_mesh_shading_system.hpp>
#include <game/actor.hpp>
#include <game/actors/default_camera.hpp>
#include <physics/particle.hpp>

namespace hnll {

const vec3d physics::particle::const_acc_ = { 0.f, 9.f, 0.f };

#define RADIUS 0.3f

DEFINE_PURE_ACTOR(mesh_particle)
{
  public:
    mesh_particle()
    {
      sphere_mesh_ = game::static_mesh_comp::create(*this, "smooth_sphere.obj");

      physics::particle_init init {
        .damping = 0.99f,
        .radius = RADIUS,
        .pos = { 0.f, -2.f, 0.f },
        .vel = { 1.f, 0.f, -1.f },
      };
      set_scale({RADIUS, RADIUS, RADIUS});

      particle_ = std::make_unique<physics::particle>(init);
    }

    void update_this(const float& dt)
    {
      particle_->update(dt);
      set_translation(particle_->get_pos().cast<float>());
    }

    game::static_mesh_comp& get_mesh_comp() { return *sphere_mesh_; }

    void add_force(const vec3d& force) { particle_->add_force(force); }

  private:
    u_ptr<game::static_mesh_comp> sphere_mesh_;
    u_ptr<physics::particle> particle_;
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
      if (ImGui::Button("remove particle")) {
        particle_.reset();
      }
      if (ImGui::Button("add x force")) {
        particle_->add_force(vec3d{ -100.f, 0.f, 0.f });
      }
      if (ImGui::Button("add y force")) {
        particle_->add_force(vec3d{ 0.f, -1000.f, 0.f });
      }
      ImGui::Text("fps : %d", int(1.f / dt));
      ImGui::End();

      particle_->update(dt);
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