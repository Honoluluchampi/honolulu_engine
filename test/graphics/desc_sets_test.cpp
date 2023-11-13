// hnll
#include <graphics/window.hpp>
#include <graphics/device.hpp>
#include <graphics/desc_set.hpp>
#include <utils/vulkan_config.hpp>
#include <utils/singleton.hpp>

// lib
#include <gtest/gtest.h>

namespace hnll {

auto config = utils::singleton<utils::vulkan_config>::build_instance();
auto window = utils::singleton<graphics::window>::build_instance(50, 30, "test");
auto device = utils::singleton<graphics::device>::build_instance();

TEST(desc_set, ctor) {
  auto desc_pool = graphics::desc_pool::builder(*device)
    .add_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100)
    .build();

  graphics::binding_info bind1 = { VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, "bind0" };
  graphics::binding_info bind2 = { VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, "bind1" };

  graphics::desc_set_info set1 {{ bind1, bind2 }, "set0" };
  graphics::desc_set_info set2 {{ bind1 }, "set1" };

  auto desc_set = graphics::desc_sets::create(
    *device,
    desc_pool,
    { set1, set2 },
    2
  );

  auto buffer_debug_names = desc_set->get_buffer_debug_names();
  EXPECT_EQ(buffer_debug_names[0], "frame0 set0 bind0");
  EXPECT_EQ(buffer_debug_names[1], "frame0 set0 bind1");
  EXPECT_EQ(buffer_debug_names[2], "frame0 set1 bind0");
  EXPECT_EQ(buffer_debug_names[3], "frame1 set0 bind0");
  EXPECT_EQ(buffer_debug_names[4], "frame1 set0 bind1");
  EXPECT_EQ(buffer_debug_names[5], "frame1 set1 bind0");

  auto vk_desc_sets_debug_names = desc_set->get_vk_desc_sets_debug_names();
  EXPECT_EQ(vk_desc_sets_debug_names[0], "frame0 set0");
  EXPECT_EQ(vk_desc_sets_debug_names[1], "frame0 set1");
  EXPECT_EQ(vk_desc_sets_debug_names[2], "frame1 set0");
  EXPECT_EQ(vk_desc_sets_debug_names[3], "frame1 set1");

  EXPECT_EQ(0, desc_set->get_buffer_id(0, 0, 0));
  EXPECT_EQ(1, desc_set->get_buffer_id(0, 0, 1));
  EXPECT_EQ(2, desc_set->get_buffer_id(0, 1, 0));
  EXPECT_EQ(3, desc_set->get_buffer_id(1, 0, 0));
  EXPECT_EQ(4, desc_set->get_buffer_id(1, 0, 1));
  EXPECT_EQ(5, desc_set->get_buffer_id(1, 1, 0));

  EXPECT_EQ(0, desc_set->get_vk_desc_set_id(0, 0));
  EXPECT_EQ(1, desc_set->get_vk_desc_set_id(0, 1));
  EXPECT_EQ(2, desc_set->get_vk_desc_set_id(1, 0));
  EXPECT_EQ(3, desc_set->get_vk_desc_set_id(1, 1));
}

} // namespace hnll