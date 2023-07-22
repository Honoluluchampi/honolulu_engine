// hnll
#include <graphics/window.hpp>
#include <graphics/device.hpp>
#include <graphics/desc_set.hpp>
#include <utils/rendering_utils.hpp>

// lib
#include <gtest/gtest.h>

namespace hnll {

auto window = graphics::window::create(50, 30, "test");
auto device = graphics::device::create(*window, utils::rendering_type::VERTEX_SHADING);
auto desc_pool = graphics::desc_pool::builder(*device)
  .add_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100)
  .build();

TEST(desc_set, ctor) {
  std::vector<graphics::binding_info> bindings = {
    { VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER },
    { VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER }
  };

  graphics::desc_set_info set_info { bindings };
  set_info.is_frame_buffered_ = true;

  auto desc_set = graphics::desc_sets::create(
    *device,
    desc_pool,
    { set_info, set_info },
    2
  );

  EXPECT_EQ(1, 1);
}

} // namespace hnll