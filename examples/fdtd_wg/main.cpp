#include <utils/common_alias.hpp>
#include <iostream>
#include <fstream>

namespace hnll {

constexpr float dx_fdtd = 3.83e-3; // dx for fdtd grid : 3.83mm
constexpr float dt = 7.81e-6; // in seconds
const float dx_wg = dx_fdtd / std::sqrt(2); // dx for wave guide grid

constexpr float c = 340; // sound speed : 340 m/s
constexpr float rho = 1.17f;

const float v_fac = dt / (rho * dx_fdtd);
const float p_fac = dt * rho * c * c / dx_fdtd;

const float segment_length = 0.2; // 20 cm

struct fdtd_section
{
  fdtd_section(float length)
  {
    // plus one grid for additional
    grid_count_ = static_cast<int>(length / dx_fdtd);
    p_.resize(grid_count_, 0);
    v_.resize(grid_count_, 0);
  }

  std::vector<float> p_;
  std::vector<float> v_;
  float ghost_v_ = 0;
  int grid_count_;
};

struct wg_section
{
  wg_section(float length)
  {
    grid_count_ = static_cast<int>(length / dx_wg);
    forward_.resize(grid_count_, 0);
    backward_.resize(grid_count_, 0);
  }

  void update(float f_input, float b_input)
  {
    f_cursor_ = f_cursor_ == 0 ? grid_count_ - 1 : f_cursor_ - 1;
    b_cursor_ = b_cursor_ + 1 == grid_count_ ? 0 : b_cursor_ + 1;

    forward_[f_cursor_] = f_input;
    backward_[b_cursor_] = b_input;
  }

  float get_first() const
  {
    if (b_cursor_ != grid_count_ - 1) {
      return backward_[b_cursor_ + 1];
    }
    else {
      return backward_[0];
    }
  }

  float get_last() const
  {
    if (f_cursor_ != 0) {
      return forward_[f_cursor_ - 1];
    }
    else {
      return forward_[grid_count_ - 1];
    }
  }

  std::vector<float> forward_;
  std::vector<float> backward_;
  int grid_count_;
  int f_cursor_ = 0; // indicates first element of forward
  int b_cursor_ = 0; //
};

} // namespace hnll

int main()
{
  std::cout << "dx fdtd : " << hnll::dx_fdtd << std::endl;
  std::cout << "dx wave guid : " << hnll::dx_wg << std::endl;

  int iter = 1000;

  // only fdtd
  auto only = hnll::fdtd_section(hnll::segment_length * 3.f);
  // combined
  auto left  = hnll::fdtd_section(hnll::segment_length);
  auto wg    = hnll::wg_section(hnll::segment_length);
  auto right = hnll::fdtd_section(hnll::segment_length);

  // impulse at 10 cm
  only.p_[only.grid_count_ / 6] = 500.f;
  left.p_[left.grid_count_ / 2] = 500.f;

  // file system
  std::ofstream ofs("data.txt");

  for (int t = 0; t < iter; t++) {
    // update fdtd_only's velocity ---------------------------------------------------
    for (int i = 1; i < only.grid_count_ - 1; i++) {
      only.v_[i] -= hnll::v_fac * (only.p_[i] - only.p_[i - 1]);
    }
    // update fdtd_only's pressure ---------------------------------------------------
    for (int i = 0; i < only.grid_count_ - 1; i++) {
      only.p_[i] -= hnll::p_fac * (only.v_[i + 1] - only.v_[i]);
    }

    // update combined model velocity ------------------------------------------------
    // left part
    for (int i = 1; i < left.grid_count_; i++) {
      left.v_[i] -= hnll::v_fac * (left.p_[i] - left.p_[i - 1]);
    }
    // junction part
    left.ghost_v_ -= hnll::v_fac * (wg.get_first() - left.p_[left.grid_count_ - 1]);

    // right part
    for (int i = 1; i < right.grid_count_; i++) {
      right.v_[i] -= hnll::v_fac * (right.p_[i] - right.p_[i - 1]);
    }
    // junction
    right.v_[0] -= hnll::v_fac * (right.p_[0] - wg.get_last());

    // update combined model pressure -----------------------------------------------
    // left
    for (int i = 0; i < left.grid_count_ - 1; i++) {
      left.p_[i] -= hnll::p_fac * (left.v_[i + 1] - left.v_[i]);
    }
    // junction
    left.p_[left.grid_count_ - 1] -= hnll::p_fac * (left.ghost_v_ - left.v_[left.grid_count_ - 1]);

    // right (include junction)
    for (int i = 0; i < right.grid_count_ - 1; i++) {
      right.p_[i] -= hnll::p_fac * (right.v_[i + 1] - right.v_[i]);
    }

    // wave guide
    wg.update(left.p_[left.grid_count_ - 1], right.p_[0]);

    std::cout << "only at 50 cm : " << only.p_[only.grid_count_ * 5 / 6] << std::endl;
    std::cout << "comb at 50 cm : " << right.p_[right.grid_count_ / 2] << std::endl;

    ofs << only.p_[only.grid_count_ * 5 / 6] << std::endl;
    ofs << right.p_[right.grid_count_ / 2] << std::endl;
  }
}