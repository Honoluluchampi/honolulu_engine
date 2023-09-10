#pragma once

#include <utils/common_alias.hpp>

namespace hnll::physics {

class particle
{
  public:
    void update(double dt)
    {
      // update linear position
      pos_ += dt * vel_;
      // update velocity
      vel_ += dt * (inv_mass_ * force_ + const_acc_);
      // full form of the dragging force
      vel_ *= std::pow(damping_, dt);
      // fast but not precise form
      // vel_ *= damping_;

      force_ = { 0.f, 0.f, 0.f };
    }

    // setter
    void add_force(const vec3d& force) { force_ += force; }

  private:
    double inv_mass_; // 1 / mass
    double damping_;
    vec3d pos_;
    vec3d vel_;
    vec3d force_; // zeroed in each frame update
    static const vec3d const_acc_; // mainly for gravity
};

} // namespace hnll::physics