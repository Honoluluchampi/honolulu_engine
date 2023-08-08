// hnll
#include "include/fdtd3_field.hpp"
#include <utils/singleton.hpp>
#include <graphics/device.hpp>
#include <utils/rendering_utils.hpp>

namespace hnll {

u_ptr<fdtd3_field> fdtd3_field::create(const hnll::fdtd_info &info)
{ return std::make_unique<fdtd3_field>(info); }

fdtd3_field::fdtd3_field(const hnll::fdtd_info &info)
  : device_(utils::singleton<graphics::device>::get_instance())
{
  length_ = info.length;
  c_ = info.sound_speed;
  rho_ = info.rho;
  pml_count_ = info.pml_count;
  update_per_frame_ = info.update_per_frame;
}

fdtd3_field::~fdtd3_field() {}

} // namespace hnll