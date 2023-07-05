#include "fdtd_horn.hpp"

namespace hnll {

u_ptr<fdtd_horn> fdtd_horn::create(
  float dt,
  float dx,
  float rho,
  float c,
  std::vector<int> dimensions,
  std::vector<vec2> sizes)
{ return std::make_unique<fdtd_horn>(dt, dx, rho, c, dimensions, sizes); }

fdtd_horn::fdtd_horn(
  float dt,
  float dx,
  float rho,
  float c,
  std::vector<int> dimensions,
  std::vector<vec2> sizes)
{
  dt_ = dt;
  dx_ = dx;
  rho_ = rho;
  c_ = c;
  dimensions_ = dimensions;
  sizes_ = sizes;

  whole_grid_count_ = 0;
  start_grid_ids_.resize(dimensions_.size());
  grid_counts_.resize(dimensions_.size());

  for (int i = 0; i < dimensions_.size(); i++) {
    start_grid_ids_[i] = whole_grid_count_;
    if (dimensions_[i] == 1)
      grid_counts_[i] = { sizes_[i].x() / dx_, 1 };
    if (dimensions_[i] == 2)
      grid_counts_[i] = { sizes_[i].x() / dx_, sizes_[i].y() / dx_ };
    whole_grid_count_ += grid_counts_[i].x() * grid_counts_[i].y();
  }

  field_.resize(whole_grid_count_, { 0.f, 0.f, 0.f, 0.f });
}

}