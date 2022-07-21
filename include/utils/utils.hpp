#pragma once

// TODO : put this file on a appropriate position
// TODO : create createOneShotCommandPool();

// std
#include <memory>
#include <iostream>

// lib
#include <eigen3/Eigen/Dense>

template<class U> using u_ptr = std::unique_ptr<U>;
template<class S> using s_ptr = std::shared_ptr<S>;

namespace hnll {
namespace utils {

// 3d transformation
struct transform
{
  Eigen::Vector3f translation{}; // position offset
  Eigen::Vector3f scale{1.f, 1.f, 1.f};
  // y-z-x tait-brian rotation
  Eigen::Vector3f rotation{};

  // Matrix corrsponds to Translate * Ry * Rx * Rz * Scale
  // Rotations correspond to Tait-bryan angles of Y(1), X(2), Z(3)
  // https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
  Eigen::Matrix4f mat4();
  Eigen::Matrix4f rotate_mat4();
  // normal = R * S(-1)
  Eigen::Matrix4f normal_matrix();
};

static inline Eigen::Vector3f sclXvec(const float scalar, const Eigen::Vector3f& vec)
{ return {vec.x() * scalar, vec.y() * scalar, vec.z() * scalar}; }

} // namespace utils
} // namespace hnll