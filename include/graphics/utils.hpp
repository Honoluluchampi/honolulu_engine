#pragma once

// std
#include <vector>
#include <utils/common_alias.hpp>

// lib
#include <vulkan/vulkan.h>

namespace hnll::graphics {

struct vertex
{
  alignas(16) vec3 position{};
  alignas(16) vec3 color{};
  alignas(16) vec3 normal{};
  // texture coordinates
  vec2 uv{};
  // return a description compatible with the shader
  static std::vector<VkVertexInputBindingDescription> get_binding_descriptions();
  static std::vector<VkVertexInputAttributeDescription> get_attribute_descriptions();

  bool operator==(const vertex& other) const
  { return position == other.position && color == other.color && normal == other.normal && uv == other.uv; }
};

struct obj_loader
{
  // copied to the vertex buffer and index buffer
  std::vector<vertex> vertices{};
  std::vector<uint32_t> indices{};

  void load_model(const std::string& filename);
};

// for ray tracing
VkDeviceAddress get_device_address(VkDevice device, VkBuffer buffer);

} // namespace hnll::graphics

namespace std {

template<typename Scalar, int Rows, int Cols>
struct hash<Eigen::Matrix<Scalar, Rows, Cols>> {
  // https://wjngkoh.wordpress.com/2015/03/04/c-hash-function-for-eigen-matrix-and-vector/
  size_t operator()(const Eigen::Matrix<Scalar, Rows, Cols> &matrix) const {
    size_t seed = 0;
    for (size_t i = 0; i < matrix.size(); ++i) {
      Scalar elem = *(matrix.data() + i);
      seed ^=
        std::hash<Scalar>()(elem) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
  }
};
} // namespace std

namespace hnll::graphics {
// https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x/57595105#57595105
template<typename T, typename... Rest>
void hash_combine(std::size_t &seed, const T &v, const Rest &... rest) {
  seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  (hash_combine(seed, rest), ...);
}
} // namespace hnll::graphics

namespace std {

template <>
struct hash<hnll::graphics::vertex>
{
  size_t operator() (hnll::graphics::vertex const &vertex) const
  {
    // stores final hash value
    size_t seed = 0;
    hnll::graphics::hash_combine(seed, vertex.position, vertex.color, vertex.normal, vertex.uv);
    return seed;
  }
};

} // namespace std