#pragma once

// std
#include <variant>

// hnll
#include <utils/common_alias.hpp>

namespace hnll::physics {

struct bvh;

class collision_detector
{
  public:

  private:
    u_ptr<bvh> bvh_;
};

} // namespace hnll::physics