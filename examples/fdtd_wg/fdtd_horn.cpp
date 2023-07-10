// hnll
#include "fdtd_horn.hpp"
#include <graphics/buffer.hpp>
#include <utils/rendering_utils.hpp>

namespace hnll {

graphics::binding_info fdtd_horn::common_binding_info = {
  VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
};

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
  segment_count_ = dimensions_.size();

  whole_grid_count_ = 0;
  size_infos_.resize(dimensions_.size());
  edge_infos_.resize(dimensions_.size());
  grid_counts_.resize(dimensions_.size());

  auto segment_count = dimensions.size();

  float current_x_edge = 0.f;

  for (int i = 0; i < dimensions_.size(); i++) {
    size_infos_[i].x() = sizes[i].x();
    size_infos_[i].y() = sizes[i].y();
    size_infos_[i].w() = dimensions_[i];

    // no pml
    if (i != segment_count - 1) {
      if (dimensions_[i] == 1) {
        grid_counts_[i] = {size_infos_[i].x() / dx_, 1};
        // fix size
        size_infos_[i].x() = grid_counts_[i].x() * dx_;
      }
      if (dimensions_[i] == 2) {
        grid_counts_[i] = {size_infos_[i].x() / dx_, size_infos_[i].y() / dx_};
        // fix size
        size_infos_[i].x() = grid_counts_[i].x() * dx_;
        size_infos_[i].y() = grid_counts_[i].y() * dx_;
      }
    }
    // set pml
    else if (i == segment_count - 1 && dimensions_[i] == 2) {
      grid_counts_[i] = {size_infos_[i].x() / dx_ + pml_count_ * 2, size_infos_[i].y() / dx_ + pml_count * 2};
      // fix size
      size_infos_[i].x() = (grid_counts_[i].x() - 2 * pml_count_) * dx_;
      size_infos_[i].y() = (grid_counts_[i].y() - 2 * pml_count_) * dx_;
    }
    else {
      throw std::runtime_error("fdtd_horn::unsupported combination.");
    }

    edge_infos_[i].x() = current_x_edge;
    edge_infos_[i].y() = whole_grid_count_;
    current_x_edge += size_infos_[i].x();
    whole_grid_count_ += grid_counts_[i].x() * grid_counts_[i].y();
  }

  edge_infos_.emplace_back(current_x_edge, static_cast<float>(whole_grid_count_), 0.f, 0.f);

  field_.resize(whole_grid_count_, { 0.f, 0.f, 0.f, 0.f });
  grid_conditions_.resize(whole_grid_count_, { 0.f, 0.f, 0.f, 0.f });

  // define grid_conditions
  for (int i = 0; i < edge_infos_.size() - 1; i++) {
    for (grid_id j = edge_infos_[i].y(); j < edge_infos_[i + 1].y(); j++) {
      grid_conditions_[j] = { float(grid_type::NORMAL), 0.f, float(dimensions_[i]), 0.f };
      // TODO : define EXCITER, PML
      // detect PML
      if (i == edge_infos_.size() - 2) {
        auto local_grid = j - int(edge_infos_[i].y());
        auto x_idx = local_grid % grid_counts_[i].x();
        auto y_idx = local_grid / grid_counts_[i].y();
        bool pml_x = (x_idx < pml_count) || (x_idx >= grid_counts_[i].x() - pml_count);
        bool pml_y = (y_idx < pml_count) || (y_idx >= grid_counts_[i].y() - pml_count);
        if (pml_x || pml_y) {
          grid_conditions_[j].x() = float(grid_type::PML);
        }
      }
    }
  }

  // temp : test pressure
  for (int i = 0; i < field_.size(); i++) {
    field_[i].z() = static_cast<float>(i);
  }
}

void fdtd_horn::build_desc(graphics::device &device)
{
  desc_pool_ = graphics::desc_pool::builder(device)
    // field, grid conditions, size_infos
    .add_pool_size(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, utils::FRAMES_IN_FLIGHT * 3)
    .build();

  graphics::desc_set_info common_set_info {{ common_binding_info }};
  common_set_info.is_frame_buffered_ = true;

  desc_sets_ = graphics::desc_sets::create(
    device,
    desc_pool_,
    // field, grid conditions, size_infos
    { common_set_info, common_set_info, common_set_info, common_set_info, common_set_info },
    utils::FRAMES_IN_FLIGHT
  );

  for (int i = 0; i < utils::FRAMES_IN_FLIGHT; i++) {
    auto field_buffer = graphics::buffer::create(
      device,
      sizeof(vec4) * field_.size(),
      1,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      field_.data()
    );

    auto grid_conditions_buffer = graphics::buffer::create(
      device,
      sizeof(vec4) * grid_conditions_.size(),
      1,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      grid_conditions_.data()
    );

    auto size_infos_buffer = graphics::buffer::create(
      device,
      sizeof(vec4) * size_infos_.size(),
      1,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      size_infos_.data()
    );

    auto edge_infos_buffer = graphics::buffer::create(
      device,
      sizeof(vec4) * edge_infos_.size(),
      1,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      edge_infos_.data()
    );

    auto grid_counts_buffer = graphics::buffer::create(
      device,
      sizeof(ivec2) * grid_counts_.size(),
      1,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      grid_counts_.data()
    );

    desc_sets_->set_buffer(0, 0, i, std::move(field_buffer));
    desc_sets_->set_buffer(1, 0, i, std::move(grid_conditions_buffer));
    desc_sets_->set_buffer(2, 0, i, std::move(size_infos_buffer));
    desc_sets_->set_buffer(3, 0, i, std::move(edge_infos_buffer));
    desc_sets_->set_buffer(4, 0, i, std::move(grid_counts_buffer));
  }
  desc_sets_->build();
}

void fdtd_horn::update(int frame_index)
{
  // update field buffer
  desc_sets_->write_to_buffer(0, 0, frame_index, field_.data());
  desc_sets_->flush_buffer(0, 0, frame_index);
}

} // namespace hnll