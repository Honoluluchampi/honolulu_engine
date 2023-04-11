#pragma once

// hnll
#include <utils/common_alias.hpp>

namespace hnll::physics {

class mass_spring_cloth
{
  public:
    mass_spring_cloth() { static uint32_t id = 0; cloth_id_ = id++; }

    inline uint32_t get_id() const { return cloth_id_; }
  private:
    uint32_t cloth_id_;
};

} // namespace hnll::physics