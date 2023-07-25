// hnll
#include <graphics/window.hpp>
#include <graphics/device.hpp>
#include <graphics/desc_set.hpp>
#include <utils/rendering_utils.hpp>
#include <utils/singleton.hpp>

// lib
#include <gtest/gtest.h>

namespace hnll {

auto& window = utils::singleton<graphics::window>::get_instance(50, 30, "test");
auto& device = utils::singleton<graphics::device>::get_instance(utils::rendering_type::VERTEX_SHADING);

TEST(desc_set, ctor) {
  auto desc_pool = graphics::desc_pool::builder(device)
    .add_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100)
    .build();

  graphics::binding_info bind1 = { VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, "bind1" };
  graphics::binding_info bind2 = { VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, "bind2" };

  graphics::desc_set_info set1 {{ bind1, bind2 }, "set1" };
  graphics::desc_set_info set2 {{ bind1 }, "set2" };

  auto desc_set = graphics::desc_sets::create(
    device,
    desc_pool,
    { set1, set2 },
    2
  );

  auto buffer_debug_names = desc_set->get_buffer_debug_names();
  EXPECT_EQ(buffer_debug_names[0], "frame0 set1 bind1");
  EXPECT_EQ(buffer_debug_names[1], "frame0 set1 bind2");
  EXPECT_EQ(buffer_debug_names[2], "frame0 set2 bind1");
  EXPECT_EQ(buffer_debug_names[3], "frame1 set1 bind1");
  EXPECT_EQ(buffer_debug_names[4], "frame1 set1 bind2");
  EXPECT_EQ(buffer_debug_names[5], "frame1 set2 bind1");

  auto vk_desc_sets_debug_names = desc_set->get_vk_desc_sets_debug_names();
  EXPECT_EQ(vk_desc_sets_debug_names[0], "frame0 set1");
  EXPECT_EQ(vk_desc_sets_debug_names[1], "frame0 set2");
  EXPECT_EQ(vk_desc_sets_debug_names[2], "frame1 set1");
  EXPECT_EQ(vk_desc_sets_debug_names[3], "frame1 set2");
}

} // namespace hnll