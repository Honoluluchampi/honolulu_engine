// hnll
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
};

} // namespace hnll

int main()
{

}