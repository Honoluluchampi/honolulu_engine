#pragma once

// lib
#include <eigen3/Eigen/Dense>

namespace hnll {
namespace graphics {

class camera 
{
  public:
    camera();
    void set_orthographics_projection(float left, float right, float top, float bottom, float near, float far);

    void set_perspective_projection(float fovy, float aspect, float near, float far);

    // one of the three ways to initialize view matrix
    // camera position, which direction to point, which direction is up
    void set_veiw_direction(Eigen::Vector3d position, Eigen::Vector3d direction, Eigen::Vector3d up = Eigen::Vector3d(0.f, -1.f, 0.f));
    void set_view_target(Eigen::Vector3d position, Eigen::Vector3d target, Eigen::Vector3d up = Eigen::Vector3d(0.f, -1.f, 0.f));
    void set_veiw_yxz(Eigen::Vector3d position, Eigen::Vector3d rotation);


    const Eigen::Matrix4d& get_projection() const { return projection_matrix_; }
    const Eigen::Matrix4d& get_view() const { return view_matrix_; }
    
  private:
    Eigen::Matrix4d projection_matrix_;
    Eigen::Matrix4d view_matrix_;
};

} // namespace graphics
} // namespace hnll