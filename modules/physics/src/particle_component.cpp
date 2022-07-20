// hnll
#include <physics/particle_component.hpp>
#include <game/actor.hpp>

// lib
#include <eigen3/Eigen/Dense>

using hnll::utils::transform;
using Eigen::VectorXd;
using Eigen::MatrixXd;

namespace hnll {
namespace physics {

particle_component::particle_component(s_ptr<hnll::utils::transform>&& transform_sp)
 : transform_sp_(std::move(transform_sp)), position_(transform_sp->translation)
{
  dx_dt_ = [this](float dt) {
    this->position_ += velocity_ * dt;
    this->velocity_.array() += gravity_value_ * dt; // m * g * dt / m
  };
}

s_ptr<particle_component> particle_component::create(game::actor &actor)
{
  auto particle = std::make_shared<particle_component>(actor.get_transform_sp());
  return particle;
}

void particle_component::update_component(float dt)
{
  dx_dt_(dt);
}

} // namespace physics
} // namespace hnll