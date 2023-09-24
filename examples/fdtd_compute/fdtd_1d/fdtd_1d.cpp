// hnll
#include <game/engine.hpp>
#include <game/actor.hpp>
#include <game/shading_system.hpp>

namespace hnll {

constexpr float dx  = 0.01;
constexpr float dt  = 0.01;
constexpr float rho = 0.01;
constexpr float sound_speed = 340.f;

struct fdtd_info {
  float x_len;
  int pml_count;
  int update_per_frame;
};

class fdtd_1d_field
{
  public:
    fdtd_1d_field() = default;

  private:

};

DEFINE_SHADING_SYSTEM(fdtd_1d_shader, game::dummy_renderable_comp<utils::shading_type::UNIQUE>)
{
  public:
    DEFAULT_SHADING_SYSTEM_CTOR(fdtd_1d_shader, game::dummy_renderable_comp<utils::shading_type::UNIQUE>);

    void setup()
    {

    }

    void render(const utils::graphics_frame_info&)
    {}
};

SELECT_ACTOR(game::no_actor);
SELECT_SHADING_SYSTEM(fdtd_1d_shader);
SELECT_COMPUTE_SHADER();

DEFINE_ENGINE(curved_fdtd_1d)
{
  public:
    ENGINE_CTOR(curved_fdtd_1d)
    {}
};

} // namespace hnll

int main()
{
  hnll::curved_fdtd_1d app;

  try { app.run(); }
  catch(const std::exception& e) { std::cerr << e.what() << std::endl; }
}