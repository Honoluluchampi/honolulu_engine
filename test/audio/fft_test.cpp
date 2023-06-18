// hnll
#include <audio/utils.hpp>

// lib
#include <gtest/gtest.h>


namespace hnll::audio {

TEST(fft, fft) {
  std::vector<ALshort> input = { 1, 0, 2, 0, 0, 2, 5, 4 };
  // F_ans is obtained by numpy.fft.fft
  std::vector<double> F_real_ans = {
    14.f,
    2.313213562f,
    -6.f,
    -0.41421356f,
    2.f,
    -0.41421356f,
    -6.f,
    2.313213562f,
  };

  std::vector<double> F_imag_ans = {
    0.f,
    7.242640687f,
    2.f,
    1.242640687f,
    0.f,
    -1.242640687f,
    -2.f,
    -7.242640687f,
  };

  auto F = utils::fft(input);

  auto inverse = utils::ifft(F);

  // check fft answer
  for (int i = 0; i < F.size(); i++) {
    EXPECT_DOUBLE_EQ(F[i].real(), F_real_ans[i]);
    EXPECT_DOUBLE_EQ(F[i].imag(), F_imag_ans[i]);
  }
}

TEST(fft, ifft) {
  std::vector<ALshort> input = { 1, 0, 2, 0, 0, 2, 5, 4 };

  auto F = utils::fft(input);
  auto inverse = utils::ifft(F);

  // check ifft answer
  for (int i = 0; i < input.size(); i++) {
    EXPECT_EQ(input[i], inverse[i].real());
    EXPECT_DOUBLE_EQ(0.f, inverse[i].imag());
  }
}
} // namespace hnll::audio