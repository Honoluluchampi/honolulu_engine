// hnll
#include <game/components/viewer_comp.hpp>
// #include <geometry/perspective_frustum.hpp>

// std
#include <limits>

namespace hnll::game {

float viewer_comp::near_distance_ = 0.1f;
float viewer_comp::far_distance_  = 50.f;
float viewer_comp::fov_y_ = 50.f * M_PI / 180.f;

s_ptr<viewer_comp> viewer_comp::create(utils::transform& transform, graphics::renderer &renderer,double fov_x, double fov_y, double near_z, double far_z)
{
  auto viewer_compo = std::make_shared<viewer_comp>(transform, renderer);
  auto frustum = geometry::perspective_frustum::create(fov_x, fov_y, near_z, far_z);
  viewer_compo->set_perspective_frustum(std::move(frustum));
  return viewer_compo;
}

viewer_comp::viewer_comp(const hnll::utils::transform& transform, hnll::graphics::renderer& renderer)
  : transform_(transform), renderer_(renderer) {}

viewer_comp::~viewer_comp() = default;

void viewer_comp::set_orthogonal_projection(float left, float right, float top, float bottom, float near, float far)
{
  projection_matrix_ <<
    2.f / (right - left), 0.f,                  0.f,                -(right + left) / (right - left),
    0.f,                  2.f / (bottom - top), 0.f,                -(bottom + top) / (bottom - top),
    0.f,                  0.f,                  1.f / (far - near), -near / (far - near),
    0.f,                  0.f,                  0.f,                1.f;
}

void viewer_comp::update_frustum() { perspective_frustum_->update_planes(transform_); }

void viewer_comp::set_perspective_projection(float fov_y, float aspect, float near, float far)
{
  assert(std::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
  const float tan_half_fov_y = tan(fov_y / 2.f);
  projection_matrix_ <<
    1.f / (aspect * tan_half_fov_y), 0.f,                  0.f,                0.f,
    0.f,                             1.f / tan_half_fov_y, 0.f,                0.f,
    0.f,                             0.f,                  far / (far - near), -(far * near) / (far - near),
    0.f,                             0.f,                  1.f,                0.f;
}

void viewer_comp::set_view_direction(const vec3& position, const vec3& direction, const vec3& up)
{
  const vec3 w = direction.normalized();
  const vec3 u = w.cross(up).normalized();
  const vec3 v = w.cross(u);

  view_matrix_ <<
    u.x(), u.y(), u.z(), -u.dot(position),
    v.x(), v.y(), v.z(), -u.dot(position), // TODO : check
    w.x(), w.y(), w.z(), -w.dot(position),
    0.f, 0.f, 0.f, 1.f;

  // also update inverse view matrix
  inverse_view_matrix_ <<
    u.x(), v.x(), w.x(), position.x(),
    u.y(), v.y(), w.y(), position.y(),
    u.z(), v.z(), w.z(), position.z(),
    0.f, 0.f, 0.f, 1.f;
}

void viewer_comp::set_view_target(const vec3& position, const vec3& target, const vec3& up)
{ set_view_direction(position, target - position, up); }

void viewer_comp::set_view_yxz()
{
  // TODO : eigenize and delete
  auto& position = transform_.translation;
  auto& rotation = transform_.rotation;
  const float c3 = std::cos(rotation.z());
  const float s3 = std::sin(rotation.z());
  const float c2 = std::cos(rotation.x());
  const float s2 = std::sin(rotation.x());
  const float c1 = std::cos(rotation.y());
  const float s1 = std::sin(rotation.y());
  const vec3 u{(c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1)};
  const vec3 v{(c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3)};
  const vec3 w{(c2 * s1), (-s2), (c1 * c2)};
  view_matrix_ <<
    u.x(), u.y(), u.z(), -u.dot(position),
    v.x(), v.y(), v.z(), -v.dot(position),
    w.x(), w.y(), w.z(), -w.dot(position),
    0.f,   0.f,   0.f,   1.f;

  // also update inverse view matrix
  inverse_view_matrix_ <<
    u.x(), v.x(), w.x(), position.x(),
    u.y(), v.y(), w.y(), position.y(),
    u.z(), v.z(), w.z(), position.z(),
    0.f, 0.f, 0.f, 1.f;
}

Eigen::Matrix4f viewer_comp::get_inverse_perspective_projection() const
{
  // TODO : calc this in a safe manner
  return projection_matrix_.inverse();
}

Eigen::Matrix4f viewer_comp::get_inverse_view_yxz() const
{
  auto& position = transform_.translation;
  auto rotation = -transform_.rotation;
  const float c3 = std::cos(rotation.z());
  const float s3 = std::sin(rotation.z());
  const float c2 = std::cos(rotation.x());
  const float s2 = std::sin(rotation.x());
  const float c1 = std::cos(rotation.y());
  const float s1 = std::sin(rotation.y());
  const vec3 u{(c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1)};
  const vec3 v{(c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3)};
  const vec3 w{(c2 * s1), (-s2), (c1 * c2)};

  Eigen::Matrix4f inv_view;
  inv_view <<
    u.x(), u.y(), u.z(), -u.dot(position),
    v.x(), v.y(), v.z(), -v.dot(position),
    w.x(), w.y(), w.z(), -w.dot(position),
    0.f,   0.f,   0.f,   1.f;
  return inv_view;
}

// owner's transform should be updated by keyMoveComp before this function
void viewer_comp::update(const float& dt)
{
  set_view_yxz();
  auto aspect = renderer_.get_aspect_ratio();
  set_perspective_projection(fov_y_, aspect, near_distance_, far_distance_);

  if (update_view_frustum_ == update_view_frustum::ON)
    perspective_frustum_->update_planes(transform_);
}

// getter setter
const geometry::perspective_frustum& viewer_comp::get_perspective_frustum() const
{ return *perspective_frustum_; }

void viewer_comp::set_perspective_frustum(s_ptr<geometry::perspective_frustum>&& frustum)
{ perspective_frustum_ = std::move(frustum); }


} // namespace hnll::game