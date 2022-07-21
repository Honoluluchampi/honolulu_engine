#pragma once 

// hnll
#include <graphics/camera.hpp>

// libs
#include <vulkan/vulkan.h>
#include <eigen3/Eigen/Dense>

namespace hnll {
namespace graphics {

// TODO : decrese this
#define MAX_LIGHTS 20

struct point_light
{
  Eigen::Vector4f position = Eigen::Vector4f::Zero(); // ignore w
  Eigen::Vector4f color = Eigen::Vector4f::Zero(); // w is intensity
};

// global uniform buffer object
struct global_ubo
{
  // check alignment rules
  Eigen::Matrix4f projection = Eigen::Matrix4f::Identity();
  Eigen::Matrix4f view = Eigen::Matrix4f::Identity();
  // point light
  Eigen::Vector4f ambient_light_color{1.f, 1.f, 1.f, .02f}; // w is light intensity
  point_light point_lights[MAX_LIGHTS];
  int lights_count;
};

struct frame_info
{
  int frame_index;
  VkCommandBuffer command_buffer;
  VkDescriptorSet global_discriptor_set;
};

} // namespace graphics
} // namesapce hnll