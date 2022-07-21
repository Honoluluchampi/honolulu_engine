// hnll
#include <graphics/camera.hpp>

// std
#include <cassert>
#include <limits>

namespace hnll {
namespace graphics {

void camera::set_orthographics_projection(
    float left, float right, float top, float bottom, float near, float far) {
  projection_matrix_ = Eigen::Matrix4f::Identity();
  projection_matrix_(0, 0) = 2.f / (right - left);
  projection_matrix_(1, 1) = 2.f / (bottom - top);
  projection_matrix_(2, 2) = 1.f / (far - near);
  projection_matrix_(3, 0) = -(right + left) / (right - left);
  projection_matrix_(3, 1) = -(bottom + top) / (bottom - top);
  projection_matrix_(3, 2) = -near / (far - near);
}
 
void camera::set_perspective_projection(float fovy, float aspect, float near, float far) {
  assert(std::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
  const float tah_hal_foxy = tan(fovy / 2.f);
  projection_matrix_ = Eigen::Matrix4f::Identity();
  projection_matrix_(0, 0) = 1.f / (aspect * tah_hal_foxy);
  projection_matrix_(1, 1) = 1.f / (tah_hal_foxy);
  projection_matrix_(2, 2) = far / (far - near);
  projection_matrix_(2, 3) = 1.f;
  projection_matrix_(3, 2) = -(far * near) / (far - near);
}

void camera::set_veiw_direction(Eigen::Vector3f position, Eigen::Vector3f direction, Eigen::Vector3f up) {
  const Eigen::Vector3f w = direction.normalized();
  const Eigen::Vector3f u = w.cross(up).normalized();
  const Eigen::Vector3f v = w.cross(u);

  view_matrix_ = Eigen::Matrix4f::Identity();
  view_matrix_(0, 0) = u.x();
  view_matrix_(1, 0) = u.y();
  view_matrix_(2, 0) = u.z();
  view_matrix_(0, 1) = v.x();
  view_matrix_(1, 1) = v.y();
  view_matrix_(2, 1) = v.z();
  view_matrix_(0, 2) = w.x();
  view_matrix_(1, 2) = w.y();
  view_matrix_(2, 2) = w.z();
  view_matrix_(3, 0) = -u.dot(position);
  view_matrix_(3, 1) = -v.dot(position);
  view_matrix_(3, 2) = -w.dot(position);
}

void camera::set_view_target(Eigen::Vector3f position, Eigen::Vector3f target, Eigen::Vector3f up) {
  set_veiw_direction(position, target - position, up);
}

void camera::set_veiw_yxz(Eigen::Vector3f position, Eigen::Vector3f rotation) {
  const float c3 = std::cos(rotation.z());
  const float s3 = std::sin(rotation.z());
  const float c2 = std::cos(rotation.x());
  const float s2 = std::sin(rotation.x());
  const float c1 = std::cos(rotation.y());
  const float s1 = std::sin(rotation.y());
  const Eigen::Vector3f u{(c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1)};
  const Eigen::Vector3f v{(c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3)};
  const Eigen::Vector3f w{(c2 * s1), (-s2), (c1 * c2)};
  view_matrix_ = Eigen::Matrix4f::Identity();
  view_matrix_(0, 0) = u.x();
  view_matrix_(1, 0) = u.y();
  view_matrix_(2, 0) = u.z();
  view_matrix_(0, 1) = v.x();
  view_matrix_(1, 1) = v.y();
  view_matrix_(2, 1) = v.z();
  view_matrix_(0, 2) = w.x();
  view_matrix_(1, 2) = w.y();
  view_matrix_(2, 2) = w.z();
  view_matrix_(3, 0) = -u.dot(position);
  view_matrix_(3, 1) = -v.dot(position);
  view_matrix_(3, 2) = -w.dot(position);
}

} // namespace graphics
} // namesapce hnll