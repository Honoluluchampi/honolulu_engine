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
//  for (int i = 0; i < field_.size(); i++) {
//    field_[i].z() = static_cast<float>(i);
//  }
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
  frame_count_++;
  // update velocity
  for (int i = 0; i < whole_grid_count_; i++) {
    switch(int(grid_conditions_[i].x())) {
      case NORMAL1 :
      case JUNCTION_1to2_LEFT : {
        field_[i].x() -= v_fac_ * (field_[i + 1].z() - field_[i].z());
        break;
      }
      case NORMAL2 :
      case JUNCTION_2to1_LEFT : {
        auto y_grid_count = grid_counts_[int(grid_conditions_[i].w())].y();
        field_[i].x() -= v_fac_ * (field_[i + y_grid_count].z() - field_[i].z());
        if (grid_conditions_[i + 1].w() != grid_type::WALL)
          field_[i].y() -= v_fac_ * (field_[i + 1].z() - field_[i].z());
        break;
      }

      case WALL : {
        break;
      }

      case EXCITER : {
        float freq = 1000; // Hz
        float amp = 500.f;
        float val = amp * std::sin(2.f * M_PI * freq * frame_count_ * dt_);
        field_[i].x() = val;
        break;
      }

      case PML : {
        auto pml = grid_conditions_[i].y();
        auto seg_id = int(grid_conditions_[i].w());
        auto y_grid_count = grid_counts_[seg_id].y();
        auto local_idx = i - int(edge_infos_[seg_id].y());
        auto not_upper = (local_idx % y_grid_count) != y_grid_count - 1;
        auto not_right = (local_idx / y_grid_count) != grid_counts_[seg_id].x() - 1;

//        field_[i].x() = (field_[i].x()
//          - v_fac_ * (field_[i + y_grid_count].z() * not_right - field_[i].z())) / (1 + pml);
//        field_[i].y() = (field_[i].y()
//          - v_fac_ * (field_[i + 1].z() * not_upper - field_[i].z())) / (1 + pml);

        break;
      }

      case JUNCTION_1to2_RIGHT : {
        // calc mean pressure
        auto seg_id = int(grid_conditions_[i].w());
        // TODO : pml
        if (grid_conditions_[i + 1].x() == grid_type::PML) {

        }
        else {
          float mean_p = 0.f;
          float count = 0.f;
          int idx = i + 2;
          while(grid_conditions_[idx].x() == grid_type::JUNCTION_2to1_LEFT) {
            mean_p += field_[idx].z();
            count++;
            idx++;
          }
          mean_p /= count;
          field_[i].x() -= v_fac_ * (mean_p - field_[i].z());
        }
        break;
      }

      case JUNCTION_2to1_RIGHT : {
        auto next_seg = grid_conditions_[i].w() + 1;
        auto next_idx = int(edge_infos_[next_seg].y());
        auto next_p = field_[next_idx].z();
        field_[i].x() -= v_fac_ * (next_p - field_[i].z());
        if (grid_conditions_[i + 1].w() != grid_type::WALL)
          field_[i].y() -= v_fac_ * (field_[i + 1].z() - field_[i].z());
        break;
      }

      default :
        throw std::runtime_error("unsupported grid state.");
    }
  }

  // update pressure
  for (int i = 0; i < whole_grid_count_; i++) {
    switch (int(grid_conditions_[i].x())) {
      case NORMAL1 :
      case JUNCTION_1to2_RIGHT : {
        field_[i].z() -= p_fac_ * (field_[i].x() - field_[i - 1].x());
        break;
      }

      case NORMAL2 :
      case JUNCTION_2to1_RIGHT : {
        auto seg_id = int(grid_conditions_[i].w());
        auto y_grid_count = grid_counts_[seg_id].y();
        field_[i].z() -= p_fac_ * (
          field_[i].x() - field_[i - y_grid_count].x() +
          field_[i].y() - field_[i - 1].y()
        );
        break;
      }

      case WALL : {
        break;
      }

      case EXCITER : {
        float freq = 1000; // Hz
        float amp = 1.f;
        float val = amp * std::sin(2.f * M_PI * freq * frame_count_ * dt_);
        field_[i].z() = val;
//        field_[i].z() -= p_fac_ * field_[i].x();
        break;
      }

      case PML : {
        auto pml = grid_conditions_[i].y();
        auto seg_id = int(grid_conditions_[i].w());
        auto y_grid_count = grid_counts_[seg_id].y();
        auto local_idx = i - int(edge_infos_[seg_id].y());
        auto not_lower = (local_idx % y_grid_count) != 0;
        auto not_left = (local_idx / y_grid_count) != 0;

        field_[i].z() = (field_[i].z() - p_fac_ *
          (field_[i].x() - field_[i - y_grid_count].x() * not_left
          - field_[i].y() - field_[i - 1].y() * not_lower)
          ) / (1 + pml);

        break;
      }

      case JUNCTION_1to2_LEFT : {
        // calc mean vx
        float mean_vx = 0.f;
        float count = 0.f;
        // TODO : pml
        if (grid_conditions_[i - 1].w() == PML) {

        }
        else {
          int idx = i - 2;
          while (grid_conditions_[idx].w() == JUNCTION_2to1_RIGHT) {
            mean_vx += field_[idx].z();
            count++;
            idx--;
          }
          mean_vx /= count;
          field_[i].z() -= p_fac_ * (field_[i].x() - mean_vx);
        }

        break;
      }

      case JUNCTION_2to1_LEFT : {
        auto this_seg = grid_conditions_[i].w();
        float last_vx = field_[edge_infos_[this_seg].y() - 1].x();
        field_[i].z() -= p_fac_ * (
          field_[i].x() - last_vx +
          field_[i].y() - field_[i - 1].y()
        );
      }
    }
  }
  // update field buffer
  desc_sets_->write_to_buffer(0, 0, frame_index, field_.data());
  desc_sets_->flush_buffer(0, 0, frame_index);
}

} // namespace hnll