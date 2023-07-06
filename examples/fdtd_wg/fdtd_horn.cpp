#include "fdtd_horn.hpp"

namespace hnll {

u_ptr<fdtd_horn> fdtd_horn::create(
  float dt,
  float dx,
  float rho,
  float c,
  int pml_count,
  std::vector<int> dimensions,
  std::vector<vec2> sizes)
{ return std::make_unique<fdtd_horn>(dt, dx, rho, c, pml_count, dimensions, sizes); }

fdtd_horn::fdtd_horn(
  float dt,
  float dx,
  float rho,
  float c,
  int pml_count,
  std::vector<int> dimensions,
  std::vector<vec2> sizes)
{
  dt_ = dt;
  dx_ = dx;
  rho_ = rho;
  c_ = c;
  pml_count_ = pml_count;
  dimensions_ = dimensions;
  sizes_ = sizes;

  whole_grid_count_ = 0;
  start_grid_ids_.resize(dimensions_.size());
  grid_counts_.resize(dimensions_.size());

  auto segment_count = dimensions.size();

  for (int i = 0; i < dimensions_.size(); i++) {
    start_grid_ids_[i] = whole_grid_count_;
    // no pml
    if (i != segment_count - 1) {
      if (dimensions_[i] == 1)
        grid_counts_[i] = {sizes_[i].x() / dx_, 1};
      if (dimensions_[i] == 2)
        grid_counts_[i] = {sizes_[i].x() / dx_, sizes_[i].y() / dx_};
    }
    // set pml
    else if (i == segment_count - 1 && dimensions_[i] == 2) {
      grid_counts_[i] = {sizes_[i].x() / dx_ + pml_count_ * 2, sizes_[i].y() / dx_ + pml_count * 2};
    }
    else {
      throw std::runtime_error("fdtd_horn::unsupported combination.");
    }
    whole_grid_count_ += grid_counts_[i].x() * grid_counts_[i].y();
  }

  field_.resize(whole_grid_count_, { 0.f, 0.f, 0.f, 0.f });
  grid_conditions_.resize(whole_grid_count_, { 0.f, 0.f, 0.f, 0.f });
}

}