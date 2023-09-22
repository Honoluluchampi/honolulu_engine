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

  // compute constants
  double eps = 1e-5;
  EXPECT_NEAR(field->get_dx(), 3.83e-3, eps);
  EXPECT_NEAR(field->get_dt(), 6.50e-6, eps);
  EXPECT_EQ(field->get_grid_count(), ivec3(262, 209, 209 ));
  EXPECT_NEAR(field->get_v_fac(), 261096.578, eps * 1000);
  EXPECT_NEAR(field->get_p_fac(), 30182.7676, eps * 1000);
}

} // namespace hnll