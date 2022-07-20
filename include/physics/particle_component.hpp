#pragma once

// hnll
#include <utils/utils.hpp>
#include <game/component.hpp>

// std
#include <functional>

// lib
#include <eigen3/Eigen/Dense>

namespace hnll {

// forward declaration
namespace game { class actor; }

namespace physics {

class particle_component :public hnll::game::component
{
  public:
    static s_ptr<particle_component> create(game::actor& actor);
    particle_component(s_ptr<hnll::utils::transform>&& transform_sp);
    ~particle_component() {}

    // disable copy-assignment
    particle_component(const particle_component&) = delete;
    particle_component& operator=(const particle_component&) = delete;

    // getter
    bool is_gravity_enabled() const { return is_gravity_enabled_; }

    // setter
    void set_gravity_state(bool state, double value = 9.8f)
    { if (state) gravity_value_ = value; is_gravity_enabled_ = state; }

  private:
    // solve ode
    void update_component(float dt) override;
    // TODO : consider to delete s_ptr<transform>
    s_ptr<hnll::utils::transform> transform_sp_;
    Eigen::Vector3d& position_;
    Eigen::Vector3d velocity_;
    double mass_;
    std::function<void(float dt)> dx_dt_;
    float radius_ = 1.f;
    bool is_gravity_enabled_ = true;
    double gravity_value_ = 9.8f;
};

} // namespace physics
} // namespace hnll