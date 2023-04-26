// hnll
#include <physics/shading_system/fdtd2_shading_system.hpp>
#include <physics/fdtd2_field.hpp>

namespace hnll::physics {

fdtd2_field* fdtd2_shading_system::target_ = nullptr;

void fdtd2_shading_system::set_target(fdtd2_field* target)
{ target_ = target; }

void fdtd2_shading_system::remove_target(uint32_t field_id)
{ if (field_id == target_->get_field_id()) target_ = nullptr; }

} // namespace hnll::physics