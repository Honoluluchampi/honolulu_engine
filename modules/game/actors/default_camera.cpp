// hnll
#include <game/actors/default_camera.hpp>
#include <game/engine.hpp>
#include <game/modules/graphics_engine.hpp>
#include <utils/singleton.hpp>
//#include <geometry/perspective_frustum.hpp>

namespace hnll::game {

default_camera::default_camera()
{
  auto graphics_engine_core_ = utils::singleton<graphics_engine_core>::get_single_ptr();

  viewer_comp_ = viewer_comp::create(transform_, graphics_engine_core_->get_renderer_r());
  // set initial position
  transform_.translation.z() = -7.f;
  transform_.translation.y() = -2.f;

  key_comp_ = key_move_comp::create(graphics_engine_core_->get_glfw_window(), transform_);
}

void default_camera::update(const float &dt)
{
  key_comp_->update(dt);
  viewer_comp_->update(dt);
  engine_core::set_viewer_info({viewer_comp_->get_projection(), viewer_comp_->get_view(), viewer_comp_->get_inverse_view()});
}

} // namespace hnll::game