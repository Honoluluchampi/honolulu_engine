#pragma once

// hnll
#include <utils/common_alias.hpp>

namespace hnll::physics {

class mass_spring_cloth;

namespace resource_manager {

void add_cloth(const s_ptr<mass_spring_cloth> &cloth);
void remove_cloth(uint32_t cloth_id);

}
} // namespace hnll::physics