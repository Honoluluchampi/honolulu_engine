#pragma once

// hnll
#include <game/component.hpp>
#include <graphics/renderer.hpp>
#include <utils/utils.hpp>

// lib
#include <eigen3/Eigen/Dense>

// implementation based on this tutorial https://www.youtube.com/watch?v=U0_ONQQ5ZNM&list=PL8327DO66nu9qYVKLDmdLW_84-yE4auCR&index=15

namespace hnll {
namespace game {

class viewer_component : public component
{
  public:
    viewer_component(hnll::utils::transform& transform, hnll::graphics::renderer& renderer);
    ~viewer_component() {}

    void update_component(float dt) override;

    // getter
    static float get_near_distance() { return near_distance_; }
    static float get_fovy() { return fovy_; }
    const Eigen::Matrix4f& get_projection() const { return projection_matrix_; }
    const Eigen::Matrix4f& get_view() const { return veiw_matrix_; }
    Eigen::Matrix4f get_inverse_perspective_projection() const;
    Eigen::Matrix4f get_inverse_view_yxz() const;

    // setter
    void set_orthographics_projection(float left, float right, float top, float bottom, float near, float far);
    void set_perspective_projection(float fovy, float aspect, float near, float far);
    // one of the three way to initialize view matrix
    // camera position, which direction to point, which direction is up
    void set_veiw_direction(const Eigen::Vector3f& position, const Eigen::Vector3f& direction, const Eigen::Vector3f& up = Eigen::Vector3f(0.f, -1.f, 0.f));
    void set_view_target(const Eigen::Vector3f& position, const Eigen::Vector3f& target, const Eigen::Vector3f& up = Eigen::Vector3f(0.f, -1.f, 0.f));
    void set_veiw_yxz();

  private:
    // ref of owner transform
    hnll::utils::transform& transform_;
    Eigen::Matrix4f projection_matrix_ = Eigen::Matrix4f::Identity();
    Eigen::Matrix4f veiw_matrix_ = Eigen::Matrix4f::Identity();
    hnll::graphics::renderer& renderer_;

    // distance to the screen
    static float near_distance_;
    static float fovy_;
};

} // namespace game
} // namespace hnll