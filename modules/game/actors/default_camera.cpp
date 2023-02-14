// hnll
#include <game/actors/default_camera.hpp>
#include <game/modules/graphics_engine.hpp>
//#include <geometry/perspective_frustum.hpp>

namespace hnll::game {

default_camera::default_camera(graphics_engine_core& g_core)
{
  viewer_comp_ = viewer_comp::create(transform_, g_core.get_renderer_r());
  // set initial position
  transform_.translation.z() = -7.f;
  transform_.translation.y() = -2.f;

  key_comp_ = key_move_comp::create(g_core.get_glfw_window(), transform_);
}

//utils::frustum_info default_camera::get_frustum_info() const
//{
//  auto& frustum = viewer_comp_sp_->get_perspective_frustum_ref();
//  return {
//    { transform_.translation.x, transform_.translation.y, transform_.translation.z},
//    frustum.get_near_p().cast<float>(),
//    frustum.get_far_p().cast<float>(),
//    frustum.get_top_n().cast<float>(),
//    frustum.get_bottom_n().cast<float>(),
//    frustum.get_right_n().cast<float>(),
//    frustum.get_left_n().cast<float>(),
//    frustum.get_near_n().cast<float>(),
//    frustum.get_far_n().cast<float>(),
//  };
//}

} // namespace hnll::game