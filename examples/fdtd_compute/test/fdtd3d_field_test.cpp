// hnll
#include "../fdtd_3d/include/fdtd3_field.hpp"
#include <utils/test.hpp>

namespace hnll {

TEST(fdtd_3d, field_ctor) {
  setup_singleton();

  fdtd_info info {
    .length = { 1.f, 0.8f, 0.8f },
    .sound_speed = 340.f,
    .rho = 0.001f,
    .pml_count = 5,
    .update_per_frame = 3
  };

  auto field = fdtd3_field::create(info);

  EXPECT_EQ(field->get_length(), vec3(1.f, 0.8, 0.8f));
  EXPECT_EQ(field->get_sound_speed(), 340.f);
  EXPECT_EQ(field->get_rho(), 0.001f);
  EXPECT_EQ(field->get_pml_count(), 5);
  EXPECT_EQ(field->get_update_per_frame(), 3);
}

} // namespace hnll