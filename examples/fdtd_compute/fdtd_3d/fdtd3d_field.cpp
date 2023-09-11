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

  compute_constants();
}

fdtd3_field::~fdtd3_field() {}

void fdtd3_field::compute_constants()
{
  dx_ = 3.83e-3;
  dt_ = 6.50e-6; // dt = dx / c / root(3)

  grid_count_.x() = std::ceil(length_.x() / dx_);
  grid_count_.y() = std::ceil(length_.y() / dx_);
  grid_count_.z() = std::ceil(length_.z() / dx_);

  v_fac_ = 1 / (rho_ * dx_);
  p_fac_ = rho_ * c_ * c_ / dx_;
}

} // namespace hnll