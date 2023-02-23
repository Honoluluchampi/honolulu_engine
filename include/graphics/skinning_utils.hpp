#pragma once

// hnll
#include <utils/common_alias.hpp>

// lib
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <vulkan/vulkan.h>

// forward declaration
namespace tinygltf {
class Node;
class Model;
struct Mesh;
}

namespace hnll {
namespace graphics {

class device;
class buffer;
class descriptor_set_layout;
class descriptor_pool;

namespace skinning_utils
{

#define MAX_JOINTS_NUM 128u

struct node;

struct vertex
{
  alignas(16) vec3 position;
  alignas(16) vec3 normal;
  vec2 tex_coord_0;
  vec2 tex_coord_1;
  vec4 color; // rgba
  vec4 joint_indices;
  vec4 joint_weights;
  static std::vector<VkVertexInputBindingDescription>   get_binding_descriptions();
  static std::vector<VkVertexInputAttributeDescription> get_attribute_descriptions();
};

struct mesh
{
  uint32_t first_index    = 0;
  uint32_t index_count    = 0;
  uint32_t vertex_count   = 0;
  uint32_t material_index = 0;
  bool has_indices;
};

struct mesh_group
{
  public:
    mesh_group(device& device);
    void build_desc();
    void update_desc_buffer();
    VkDescriptorSet get_desc_set() const { return desc_set_; }

    int node_index;
    std::vector<mesh> meshes;
    // TODO : add desc buffer
    struct uniform_block
    {
      mat4 matrix = mat4::Identity();
      mat4 joint_matrices[MAX_JOINTS_NUM]{};
      float joint_count = 0;
    } block;

  private:
    device&                      device_;
    u_ptr<buffer>                desc_buffer_;
    u_ptr<descriptor_pool>       desc_pool_;
    u_ptr<descriptor_set_layout> desc_layout_;
    VkDescriptorSet              desc_set_;
};

struct skin
{
  std::string name;
  s_ptr<node> root_node = nullptr;
  std::vector<s_ptr<node>>  joints;
  std::vector<mat4>         inv_bind_matrices;
  uint32_t skin_vertex_count;
};

struct node
{
  node()  = default;
  ~node() = default;

  std::string name = "";

  vec3 translation = { 0.f, 0.f, 0.f };
  quat rotation{};
  vec3 scale       = { 1.f, 1.f, 1.f };
  mat4 local_mat   = Eigen::Matrix4f::Identity();
  mat4 world_mat   = Eigen::Matrix4f::Identity();

  std::vector<s_ptr<node>> children;
  s_ptr<node>      parent = nullptr;

  uint32_t index;

  int mesh_index = -1;

  mat4 matrix;
  s_ptr<mesh_group> mesh_group_ = nullptr;
  s_ptr<skin>       skin_ = nullptr;
  int32_t skin_index = -1;

  mat4 get_local_matrix();
  mat4 get_matrix();
  void update();
};

struct material
{
  std::string name = "";
  int texture_index = -1;
  vec3 diffuse_color = { 1.f, 1.f, 1.f };
};

struct image_info
{
  std::vector<uint8_t> image_buffer;
  std::string filepath;
};

struct texture_info { int image_index; };

struct skinning_model_builder
{
  std::vector<uint32_t> index_buffer;
  std::vector<vec3>     position_buffer;
  std::vector<vec3>     normal_buffer;
  std::vector<vec2>     tex_coord_buffer;

  std::vector<uvec4> joint_buffer;
  std::vector<vec4>  weight_buffer;
};

struct builder
{
  std::vector<uint32_t>               index_buffer;
  std::vector<skinning_utils::vertex> vertex_buffer;
  size_t index_pos  = 0;
  size_t vertex_pos = 0;
};

enum class interpolation_type
{
  LINEAR,
  STEP,
  CUBICSPLINE
};

struct animation_sampler
{
  interpolation_type interpolation;
  std::vector<float> inputs;
  std::vector<vec4>  outputs;
};

struct animation_channel
{
  enum class path_type
  {
    TRANSLATION,
    ROTATION,
    SCALE
  };

  path_type   path; // translation, rotation, scale, or weights
  s_ptr<node> node_;
  uint32_t    sampler_index;
};

struct animation
{
  std::string name;
  std::vector<animation_sampler> samplers;
  std::vector<animation_channel> channels;
  float start = std::numeric_limits<float>::max();
  float end   = std::numeric_limits<float>::min();
};

}  // namespace skinning_utils
}} // namespace hnll::graphics#pragma once