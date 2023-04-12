// hnll
#include <physics/resource_manager.hpp>
#include <physics/mass_spring_cloth.hpp>
#include <physics/shading_system/cloth_compute_shading_system.hpp>
#include <physics/compute_shader/cloth_compute_shader.hpp>

namespace hnll::physics {

void resource_manager::add_cloth(const s_ptr<mass_spring_cloth>& cloth)
{
  cloth_compute_shader::add_cloth(cloth);
  cloth_compute_shading_system::add_cloth(cloth);
}

void resource_manager::remove_cloth(uint32_t cloth_id)
{
  cloth_compute_shader::remove_cloth(cloth_id);
  cloth_compute_shading_system::remove_cloth(cloth_id);
}


} // namespace hnll::physics