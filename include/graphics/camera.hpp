#pragma once

// lib
#include <eigen3/Eigen/Dense>

namespace hnll {
namespace graphics {

class camera 
{
  public:
    void set_orthographics_projection(float left, float right, float top, float bottom, float near, float far);

    void set_perspective_projection(float fovy, float aspect, float near, float far);

    // one of the three ways to initialize view matrix
    // camera position, which direction to point, which direction is up
    void set_veiw_direction(Eigen::Vector3f position, Eigen::Vector3f direction, Eigen::Vector3f up = Eigen::Vector3f(0.f, -1.f, 0.f));
    void set_view_target(Eigen::Vector3f position, Eigen::Vector3f target, Eigen::Vector3f up = Eigen::Vector3f(0.f, -1.f, 0.f));
    void set_veiw_yxz(Eigen::Vector3f position, Eigen::Vector3f rotation);


    const Eigen::Matrix4f& get_projection() const { return projection_matrix_; }
    const Eigen::Matrix4f& get_view() const { return view_matrix_; }
    
  private:
    Eigen::Matrix4f projection_matrix_ = Eigen::Matrix4f::Identity();
    Eigen::Matrix4f view_matrix_ = Eigen::Matrix4f::Identity();
};

} // namespace graphics
} // namespace hnll