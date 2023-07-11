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
  v_fac_ = dt / (rho * dx);
  p_fac_ = dt * rho * c * c / dx;
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
    grid_counts_[i].x() = std::floor(sizes[i].x() / dx_);
    grid_counts_[i].y() = std::floor(sizes[i].y() / dx_);
    if (grid_counts_[i].y() % 2 == 0)
      grid_counts_[i].y() -= 1;

    size_infos_[i].x() = grid_counts_[i].x() * dx_;
    size_infos_[i].y() = grid_counts_[i].y() * dx_;
    size_infos_[i].w() = dimensions_[i];

    // if 1D
    if (dimensions_[i] == 1) {
      grid_counts_[i].y() = 1;
    }
    // if pml
    if (i == segment_count - 1) {
      if (dimensions_[i] == 2) {
        grid_counts_[i].x() += pml_count_ * 2;
        grid_counts_[i].y() += pml_count_ * 2;
      }
      else {
        throw std::runtime_error("fdtd_horn::unsupported combination.");
      }
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
      if (dimensions_[i] == 1)
        grid_conditions_[j] = { float(grid_type::NORMAL1), 0.f, float(dimensions_[i]), float(i) };
      else if (dimensions_[i] == 2)
        grid_conditions_[j] = { float(grid_type::NORMAL2), 0.f, float(dimensions_[i]), float(i) };
      else
        throw std::runtime_error("fdtd_horn : unsupported dimension.");

      // junction 1D -> 2D
      if (dimensions_[i] == 1) {
        // beginning point or ending point
        if (j == edge_infos_[i].y())
          grid_conditions_[j].x() = grid_type::JUNCTION_1to2_LEFT;
        else if (j == edge_infos_[i + 1].y() - 1)
          grid_conditions_[j].x() = grid_type::JUNCTION_1to2_RIGHT;
      }

      // junction 2D -> 1D (other than PML segment)
      if (dimensions_[i] == 2 && i != dimensions_.size() - 1) {
        auto local_grid = j - int(edge_infos_[i].y());
        int x_idx = local_grid / grid_counts_[i].y();
        int y_idx = local_grid % grid_counts_[i].y();
        // inside a 1D tube's radius
        float y_coord = float(y_idx - int(grid_counts_[i].y() / 2)) * dx_;
        // beginning point
        if (x_idx == 0 &&
          i != 0 &&
          std::abs(y_coord) <= size_infos_[i - 1].y() / 2) {
          grid_conditions_[j].x() = grid_type::JUNCTION_2to1_LEFT;
        }
        // ending point
        if (x_idx == grid_counts_[i].x() - 1 &&
          i != segment_count_ - 1 &&
          std::abs(y_coord) <= size_infos_[i + 1].y() / 2) {
          grid_conditions_[j].x() = grid_type::JUNCTION_2to1_RIGHT;
        }

        // detect EXCITER
        // temp : set on the top of the first segment
        if (i == 0) {
          if (j < grid_counts_[i].y())
            grid_conditions_[j].x() = grid_type::EXCITER;
        }

        // wall
        if (y_idx == 0 || y_idx == grid_counts_[i].y() - 1)
          grid_conditions_[j].x() = grid_type::WALL;
      }

      // detect PML
      if (i == segment_count_ - 1) {
        auto local_grid = j - int(edge_infos_[i].y());
        auto x_idx = local_grid / grid_counts_[i].y();
        int y_idx = local_grid % grid_counts_[i].y();
        bool pml_x = (x_idx < pml_count) || (x_idx >= grid_counts_[i].x() - pml_count);
        bool pml_y = (y_idx < pml_count) || (y_idx >= grid_counts_[i].y() - pml_count);
        if (pml_x || pml_y) {
          grid_conditions_[j].x() = grid_type::PML;
        }

        float y_coord = float(y_idx - int(grid_counts_[i].y() / 2)) * dx_;
        if (x_idx == pml_count_ &&
          std::abs(y_coord) <= size_infos_[i - 1].y() / 2.f &&
          i != 0)
          grid_conditions_[j].x() = grid_type::JUNCTION_2to1_LEFT;
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
  // update velocity
  for (int i = 0; i < whole_grid_count_; i++) {
    switch(int(grid_conditions_[i].x())) {
      case NORMAL1 :
        field_[i].x() = field_[i].x() - v_fac_ * (field_[i + 1].z() - field_[i].z());

      case NORMAL2 : {
        auto y_grid_count = grid_counts_[int(grid_conditions_[i].w())].y();
        field_[i].x() = field_[i].x()
          - v_fac_ * (field_[i + y_grid_count].z() - field_[i].z());
        field_[i].y() = field_[i].y()
          - v_fac_ * (field_[i + 1].z() * float(int(grid_conditions_[i + 1].w()) != grid_type::WALL) - field_[i].z());
      }
      default :
        throw std::runtime_error("unsupported grid state.");
    }
  }

  // update field buffer
  desc_sets_->write_to_buffer(0, 0, frame_index, field_.data());
  desc_sets_->flush_buffer(0, 0, frame_index);
}

} // namespace hnll