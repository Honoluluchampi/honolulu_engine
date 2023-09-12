#pragma once

#include <utils/common_alias.hpp>

namespace hnll::physics {

struct particle_init
{
  double inv_mass = 1.f;
  double damping  = 0.95f;
  double radius   = 1.f;
  vec3d pos   = { 0.f, 0.f, 0.f };
  vec3d vel   = { 0.f, 0.f, 0.f };
  vec3d force = { 0.f, 0.f, 0.f };
};

class particle
{
  public:
    explicit particle(const particle_init &init)
    {
      inv_mass_ = init.inv_mass;
      damping_  = init.damping;
      radius_   = init.radius;
      pos_      = init.pos;
      vel_      = init.vel;
      force_    = init.force;
    }

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

    // getter
    vec3d get_pos() const { return pos_; }

    // setter
    void add_force(const vec3d& force) { force_ += force; }

  private:
    double inv_mass_ = 1.f; // 1 / mass
    double damping_ = 0.95f;
    double radius_ = 1.f;
    vec3d pos_ = { 0.f, 0.f, 0.f };
    vec3d vel_ = { 0.f, 0.f, 0.f };
    vec3d force_ = { 0.f, 0.f, 0.f }; // zeroed in each frame update
    static const vec3d const_acc_; // mainly for gravity
};

} // namespace hnll::physics