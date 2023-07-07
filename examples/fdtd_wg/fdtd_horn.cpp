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

  whole_grid_count_ = 0;
  size_infos_.resize(dimensions_.size());
  start_grid_ids_.resize(dimensions_.size());
  grid_counts_.resize(dimensions_.size());

  auto segment_count = dimensions.size();

  float current_x_edge = 0.f;

  for (int i = 0; i < dimensions_.size(); i++) {
    start_grid_ids_[i] = whole_grid_count_;
    size_infos_[i].x() = sizes[i].x();
    size_infos_[i].y() = sizes[i].y();
    current_x_edge += sizes[i].x();
    size_infos_[i].z() = current_x_edge;
    size_infos_[i].w() = dimensions_[i];

    // no pml
    if (i != segment_count - 1) {
      if (dimensions_[i] == 1)
        grid_counts_[i] = {size_infos_[i].x() / dx_, 1};
      if (dimensions_[i] == 2)
        grid_counts_[i] = {size_infos_[i].x() / dx_, size_infos_[i].y() / dx_};
    }
    // set pml
    else if (i == segment_count - 1 && dimensions_[i] == 2) {
      grid_counts_[i] = {size_infos_[i].x() / dx_ + pml_count_ * 2, size_infos_[i].y() / dx_ + pml_count * 2};
    }
    else {
      throw std::runtime_error("fdtd_horn::unsupported combination.");
    }
    whole_grid_count_ += grid_counts_[i].x() * grid_counts_[i].y();
  }

  start_grid_ids_.push_back(whole_grid_count_);

  field_.resize(whole_grid_count_, { 0.f, 0.f, 0.f, 0.f });
  grid_conditions_.resize(whole_grid_count_, { 0.f, 0.f, 0.f, 0.f });

  // define grid_conditions
  for (int i = 0; i < start_grid_ids_.size() - 1; i++) {
    for (grid_id j = start_grid_ids_[i]; j < start_grid_ids_[i + 1]; j++) {
      grid_conditions_[j] = { grid_type::NORMAL, 0.f, dimensions_[i], 0.f };
      // TODO : define EXCITER, PML
    }
  }
}

}