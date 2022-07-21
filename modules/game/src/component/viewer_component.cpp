// hnll
#include <game/components/viewer_component.hpp>

// std
#include <cassert>
#include <limits>

// lib

namespace hnll {
namespace game {

float viewer_component::near_distance_ = 0.1f;
float viewer_component::fovy_ = 50.f * 2.f * M_PI / 360.f;

viewer_component::viewer_component(hnll::utils::transform& transform, hnll::graphics::renderer& renderer)
  : component(), transform_(transform), renderer_(renderer)
{
  
}

void viewer_component::set_orthographics_projection(
    float left, float right, float top, float bottom, float near, float far) 
{
  projection_matrix_ = Eigen::Matrix4f::Identity();
  projection_matrix_(0, 0) = 2.f / (right - left);
  projection_matrix_(1, 1) = 2.f / (bottom - top);
  projection_matrix_(2, 2) = 1.f / (far - near);
  projection_matrix_(3, 0) = -(right + left) / (right - left);
  projection_matrix_(3, 1) = -(bottom + top) / (bottom - top);
  projection_matrix_(3, 2) = -near / (far - near);
}
 
void viewer_component::set_perspective_projection(float fovy, float aspect, float near, float far) 
{
  assert(std::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
  const float tan_half_fovy = tan(fovy / 2.f);
  projection_matrix_ = Eigen::Matrix4f::Zero();
  projection_matrix_(0, 0) = 1.f / (aspect * tan_half_fovy);
  projection_matrix_(1, 1) = 1.f / (tan_half_fovy);
  projection_matrix_(2, 2) = far / (far - near);
  projection_matrix_(2, 3) = 1.f;
  projection_matrix_(3, 2) = -(far * near) / (far - near);
}

void viewer_component::set_veiw_direction(const Eigen::Vector3f& position, const Eigen::Vector3f& direction, const Eigen::Vector3f& up)
{
  const Eigen::Vector3f w = direction.normalized();
  const Eigen::Vector3f u = w.cross(up).normalized();
  const Eigen::Vector3f v = w.cross(u);

  veiw_matrix_ = Eigen::Matrix4f::Identity();
  veiw_matrix_(0, 0) = u.x();
  veiw_matrix_(1, 0) = u.y();
  veiw_matrix_(2, 0) = u.z();
  veiw_matrix_(0, 1) = v.x();
  veiw_matrix_(1, 1) = v.y();
  veiw_matrix_(2, 1) = v.z();
  veiw_matrix_(0, 2) = w.x();
  veiw_matrix_(1, 2) = w.y();
  veiw_matrix_(2, 2) = w.z();
  veiw_matrix_(3, 0) = -u.dot(position);
  veiw_matrix_(3, 1) = -v.dot(position);
  veiw_matrix_(3, 2) = -w.dot(position);
}

void viewer_component::set_view_target(const Eigen::Vector3f& position, const Eigen::Vector3f& target, const Eigen::Vector3f& up)
{ set_veiw_direction(position, target - position, up); }

void viewer_component::set_veiw_yxz() 
{
  auto& position = transform_.translation;
  auto& rotation = transform_.rotation;
  const float c3 = std::cos(rotation.z());
  const float s3 = std::sin(rotation.z());
  const float c2 = std::cos(rotation.x());
  const float s2 = std::sin(rotation.x());
  const float c1 = std::cos(rotation.y());
  const float s1 = std::sin(rotation.y());
  const Eigen::Vector3f u{(c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1)};
  const Eigen::Vector3f v{(c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3)};
  const Eigen::Vector3f w{(c2 * s1), (-s2), (c1 * c2)};
  veiw_matrix_ = Eigen::Matrix4f::Identity();
  veiw_matrix_(0, 0) = u.x();
  veiw_matrix_(1, 0) = u.y();
  veiw_matrix_(2, 0) = u.z();
  veiw_matrix_(0, 1) = v.x();
  veiw_matrix_(1, 1) = v.y();
  veiw_matrix_(2, 1) = v.z();
  veiw_matrix_(0, 2) = w.x();
  veiw_matrix_(1, 2) = w.y();
  veiw_matrix_(2, 2) = w.z();
  veiw_matrix_(3, 0) = -u.dot(position);
  veiw_matrix_(3, 1) = -v.dot(position);
  veiw_matrix_(3, 2) = -w.dot(position);
}

Eigen::Matrix4f viewer_component::get_inverse_perspective_projection() const
{
  // TODO : calc this in a safe manner
  return projection_matrix_.inverse();
}

Eigen::Matrix4f viewer_component::get_inverse_view_yxz() const
{
  auto position = -transform_.translation;
  auto rotation = -transform_.rotation;
  const float c3 = std::cos(rotation.z());
  const float s3 = std::sin(rotation.z());
  const float c2 = std::cos(rotation.x());
  const float s2 = std::sin(rotation.x());
  const float c1 = std::cos(rotation.y());
  const float s1 = std::sin(rotation.y());
  const Eigen::Vector3f u{(c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1)};
  const Eigen::Vector3f v{(c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3)};
  const Eigen::Vector3f w{(c2 * s1), (-s2), (c1 * c2)};
  Eigen::Matrix4f inv_view = Eigen::Matrix4f::Identity();
  inv_view(0, 0) = u.x();
  inv_view(1, 0) = u.y();
  inv_view(2, 0) = u.z();
  inv_view(0, 1) = v.x();
  inv_view(1, 1) = v.y();
  inv_view(2, 1) = v.z();
  inv_view(0, 2) = w.x();
  inv_view(1, 2) = w.y();
  inv_view(2, 2) = w.z();
  inv_view(3, 0) = -u.dot(position);
  inv_view(3, 1) = -v.dot(position);
  inv_view(3, 2) = -w.dot(position);
  return inv_view;
}

// owner's transform should be update by keyMoveComp before this function
void viewer_component::update_component(float dt)
{ 
  set_veiw_yxz();
  auto aspect = renderer_.get_aspect_ratio();
  set_perspective_projection(fovy_, aspect, near_distance_, 50.f); 
}

} // namespace game
} // namesapce hnll