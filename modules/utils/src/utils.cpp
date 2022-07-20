// hnll
#include <utils/utils.hpp>

namespace hnll {
namespace utils {

Eigen::Matrix4d transform::mat4()
{
  const float c3 = std::cos(rotation.z()), s3 = std::sin(rotation.z()), c2 = std::cos(rotation.x()),
    s2 = std::sin(rotation.x()), c1 = std::cos(rotation.y()), s1 = std::sin(rotation.y());
  Eigen::Matrix4d result;
  result <<
          scale.x() * (c1 * c3 + s1 * s2 * s3),
          scale.x() * (c2 * s3),
          scale.x() * (c1 * s2 * s3 - c3 * s1),
          0.0f,

          scale.y() * (c3 * s1 * s2 - c1 * s3),
          scale.y() * (c2 * c3),
          scale.y() * (c1 * c3 * s2 + s1 * s3),
          0.0f,

          scale.z() * (c2 * s1),
          scale.z() * (-s2),
          scale.z() * (c1 * c2),
          0.0f,

          translation.x(),
          translation.y(),
          translation.z(),
          1.0f;
  return result;
}

Eigen::Matrix4d transform::rotate_mat4()
{
  const float c3 = std::cos(rotation.z()), s3 = std::sin(rotation.z()), c2 = std::cos(rotation.x()),
  s2 = std::sin(rotation.x()), c1 = std::cos(rotation.y()), s1 = std::sin(rotation.y());
  Eigen::Matrix4d result;
  result <<
          scale.x() * (c1 * c3 + s1 * s2 * s3),
          scale.x() * (c2 * s3),
          scale.x() * (c1 * s2 * s3 - c3 * s1),
          0.0f,

          scale.y() * (c3 * s1 * s2 - c1 * s3),
          scale.y() * (c2 * c3),
          scale.y() * (c1 * c3 * s2 + s1 * s3),
          0.0f,

          scale.z() * (c2 * s1),
          scale.z() * (-s2),
          scale.z() * (c1 * c2),
          0.0f,

          0.f,
          0.f,
          0.f,
          1.0f;
  return result;
}

// normal = R * S(-1)
Eigen::Matrix4d transform::normal_matrix()
{
  const float c3 = std::cos(rotation.z()), s3 = std::sin(rotation.z()), c2 = std::cos(rotation.x()),
    s2 = std::sin(rotation.x()), c1 = std::cos(rotation.y()), s1 = std::sin(rotation.y());

  const Eigen::Vector3d inv_scale = scale.cwiseInverse();
  Eigen::Matrix4d result;
  result <<
          inv_scale.x() * (c1 * c3 + s1 * s2 * s3),
          inv_scale.x() * (c2 * s3),
          inv_scale.x() * (c1 * s2 * s3 - c3 * s1),
          0,

          inv_scale.y() * (c3 * s1 * s2 - c1 * s3),
          inv_scale.y() * (c2 * c3),
          inv_scale.y() * (c1 * c3 * s2 + s1 * s3),
          0,

          inv_scale.z() * (c2 * s1),
          inv_scale.z() * (-s2),
          inv_scale.z() * (c1 * c2),
          0,

          0,
          0,
          0,
          1;
  return result;
}

} // namespace utils
} // namespace hnll