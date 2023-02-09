// hnll
#include <utils/utils.hpp>

// std
#include <filesystem>
#include <sys/stat.h>

namespace hnll::utils {

// model loading ----------------------------------------------------------------
std::string get_full_path(const std::string& _filename)
{
  std::string filepath = "";
  for (const auto& directory : utils::loading_directories) {
    if (std::filesystem::exists(directory + "/" + _filename)) {
      filepath = directory + "/" + _filename;
      break;
    }
  }
  if (filepath == "")
    std::runtime_error(_filename + " doesn't exist!");

  return filepath;
}

void mkdir_p(const std::string& _dir_name)
{
  struct stat buffer;
  // if the directory exists
  if (stat(_dir_name.c_str(), &buffer) == 0)
    return;
  else {
    if (mkdir(_dir_name.c_str(), 0777) != 0)
      throw std::runtime_error("failed to make directory : " + _dir_name);
  }
}

std::string create_cache_directory()
{
  auto cache_directory = std::string(getenv("HNLL_ENGN")) + "/cache";
  mkdir_p(cache_directory);

  return cache_directory;
}

std::string create_sub_cache_directory(const std::string& _dir_name)
{
  auto cache_directory = create_cache_directory();
  cache_directory += "/" + _dir_name;
  mkdir_p(cache_directory);

  return cache_directory;
}

// 3d transformation ---------------------------------------------------------------
mat4 transform::transform_mat4() const
{
  const float c3 = std::cos(rotation.z()), s3 = std::sin(rotation.z()), c2 = std::cos(rotation.x()),
    s2 = std::sin(rotation.x()), c1 = std::cos(rotation.y()), s1 = std::sin(rotation.y());

  mat4 ret;
  ret <<
    scale.x() * (c1 * c3 + s1 * s2 * s3), scale.y() * (c3 * s1 * s2 - c1 * s3), scale.z() * (c2 * s1), translation.x(),
    scale.x() * (c2 * s3),                scale.y() * (c2 * c3),                scale.z() * (-s2),     translation.y(),
    scale.x() * (c1 * s2 * s3 - c3 * s1), scale.y() * (c1 * c3 * s2 + s1 * s3), scale.z() * (c1 * c2), translation.z(),
    0.f,                                0.f,                                0.f,                 1.f;
  return ret;
}

mat4 transform::rotate_mat4() const
{
  const float c3 = std::cos(rotation.z()), s3 = std::sin(rotation.z()), c2 = std::cos(rotation.x()),
    s2 = std::sin(rotation.x()), c1 = std::cos(rotation.y()), s1 = std::sin(rotation.y());

  mat4 ret;
  ret <<
    scale.x() * (c1 * c3 + s1 * s2 * s3), scale.y() * (c3 * s1 * s2 - c1 * s3), scale.z() * (c2 * s1), 0.f,
    scale.x() * (c2 * s3),                scale.y() * (c2 * c3),                scale.z() * (-s2),     0.f,
    scale.x() * (c1 * s2 * s3 - c3 * s1), scale.y() * (c1 * c3 * s2 + s1 * s3), scale.z() * (c1 * c2), 0.f,
    0.f,                                0.f,                                0.f,                 1.f;
  return ret;
}

mat3 transform::rotate_mat3() const
{
  const float c3 = std::cos(rotation.z()), s3 = std::sin(rotation.z()), c2 = std::cos(rotation.x()),
    s2 = std::sin(rotation.x()), c1 = std::cos(rotation.y()), s1 = std::sin(rotation.y());

  mat3 ret;
  ret <<
    scale.x() * (c1 * c3 + s1 * s2 * s3), scale.y() * (c3 * s1 * s2 - c1 * s3), scale.z() * (c2 * s1),
    scale.x() * (c2 * s3),                scale.y() * (c2 * c3),                scale.z() * (-s2),
    scale.x() * (c1 * s2 * s3 - c3 * s1), scale.y() * (c1 * c3 * s2 + s1 * s3), scale.z() * (c1 * c2);
  return ret;
}

// normal = R * S(-1)
mat4 transform::normal_matrix() const
{
  const float c3 = std::cos(rotation.z()), s3 = std::sin(rotation.z()), c2 = std::cos(rotation.x()),
    s2 = std::sin(rotation.x()), c1 = std::cos(rotation.y()), s1 = std::sin(rotation.y());
  const vec3 inv_scale = {1.f / scale.x(), 1.f / scale.y(), 1.f / scale.z()};

  mat4 ret;
  ret <<
    inv_scale.x() * (c1 * c3 + s1 * s2 * s3), inv_scale.y() * (c3 * s1 * s2 - c1 * s3), inv_scale.z() * (c2 * s1), 0.f,
    inv_scale.x() * (c2 * s3),                inv_scale.y() * (c2 * c3),                inv_scale.z() * (-s2),     0.f,
    inv_scale.x() * (c1 * s2 * s3 - c3 * s1), inv_scale.y() * (c1 * c3 * s2 + s1 * s3), inv_scale.z() * (c1 * c2), 0.f,
    0.f,                                      0.f,                                      0.f,                       1.f;
  return ret;
}

} // namespace hnll::utils