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
  Eigen::Vector4d position{}; // ignore w
  Eigen::Vector4d color{}; // w is intensity
};

// global uniform buffer object
struct global_ubo
{
  // check alignment rules
  Eigen::Matrix4d projection = Eigen::Matrix4d::Identity();
  Eigen::Matrix4d view = Eigen::Matrix4d::Identity();
  // point light
  Eigen::Vector4d ambient_light_color{1.f, 1.f, 1.f, .02f}; // w is light intensity
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