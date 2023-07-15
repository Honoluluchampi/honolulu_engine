// hnll
#include "fdtd12_horn.hpp"
#include <graphics/buffer.hpp>
#include <utils/rendering_utils.hpp>

#define ID2(x, y) x + y * whole_x_

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
  float pml_max,
  std::vector<int> dimensions,
  std::vector<vec2> sizes)
{ return std::make_unique<fdtd_horn>(dt, dx, rho, c, pml_count, pml_max, dimensions, sizes); }

fdtd_horn::fdtd_horn(
  float dt,
  float dx,
  float rho,
  float c,
  int pml_count,
  float pml_max,
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
  pml_max_ = pml_max;
  dimensions_ = dimensions;
  segment_count_ = dimensions_.size();

  assert(dimensions.size() == sizes.size()
    && "fdtd_horn::ctor : element counts of 'dimensions' and 'sizes' should be same.");

  whole_grid_count_ = 0;
  size_infos_.resize(dimensions_.size());
  edge_infos_.resize(dimensions_.size());
  grid_counts_.resize(dimensions_.size());

  float current_x_edge = 0.f;

  whole_x_ = 0;
  whole_y_ = 0;

  // calc grid count of whole region
  for (int i = 0; i < segment_count_; i++) {
    grid_counts_[i].x() = std::floor(sizes[i].x() / dx_);
    grid_counts_[i].y() = std::floor(sizes[i].y() / dx_);
    if (grid_counts_[i].y() % 2 == 0)
      grid_counts_[i].y() -= 1;

    // fix size
    size_infos_[i].x() = grid_counts_[i].x() * dx_;
    size_infos_[i].y() = grid_counts_[i].y() * dx_;
    size_infos_[i].w() = dimensions_[i];

    // pml
    if (i == segment_count_ - 1) {
      if (dimensions_[i] == 2) {
        grid_counts_[i].x() += pml_count_ * 2;
        grid_counts_[i].y() += pml_count_ * 2;
        whole_x_ -= pml_count_;
      }
      else {
        throw std::runtime_error("fdtd_horn::unsupported combination.");
      }
    }

    whole_x_ += grid_counts_[i].x();
    whole_y_ = std::max(grid_counts_[i].y(), whole_y_);
  }

  x_max_ = whole_x_ * dx_;
  y_max_ = whole_y_ * dx_;
  whole_grid_count_ = whole_x_ * whole_y_;

  field_.resize(whole_grid_count_, { 0.f, 0.f, 0.f, 0.f });
  grid_conditions_.resize(whole_grid_count_, { 0.f, 0.f, 0.f, 0.f });

  // label grids
  int current_x_idx = 0;
  for (int i = 0; i < segment_count_; i++) {
    for (int x = 0; x < grid_counts_[i].x(); x++) {
      int x_idx = current_x_idx + x;
      for (int y = 0; y < grid_counts_[i].y(); y++) {
        int y_idx = whole_y_ / 2 + (y - grid_counts_[i].y() / 2);
        int idx = ID2(x_idx, y_idx);

        if (dimensions_[i] == 1) {
          assert(i != 0 && "1D tube should not be the first segment.");
          // normal
          grid_conditions_[idx].x() = NORMAL1;
          // junction
          if (x == 0) {
            grid_conditions_[idx].x() = JUNCTION_1to2_LEFT;
            grid_conditions_[idx - 1].x() = JUNCTION_2to1_RIGHT;
          }
          if (x == grid_counts_[i].x() - 1) {
            grid_conditions_[idx].x() = JUNCTION_1to2_RIGHT;
            grid_conditions_[idx + 1].x() = JUNCTION_2to1_LEFT;
          }
        }
        if (dimensions_[i] == 2) {
          // pml off
          if (i != segment_count_ - 1) {
            if (grid_conditions_[idx].x() != EMPTY)
              continue;
            // normal
            grid_conditions_[idx].x() = NORMAL2;
            // exciter
            if (x_idx == 0)
              grid_conditions_[idx].x() = EXCITER;
            // WALL
            if (y == 0 || y == grid_counts_[i].y() - 1)
              grid_conditions_[idx].x() = WALL;
          }
          // pml on
          else {
            idx -= pml_count_;
            if (grid_conditions_[idx].x() != EMPTY)
              continue;
            grid_conditions_[idx].x() = NORMAL2;
            if (x < pml_count_ || x_idx > whole_x_ - 1 ||
                y < pml_count_ || y > grid_counts_[i].y() - 1 - pml_count_)
              grid_conditions_[idx].x() = PML;
          }
        }
      }
      // set 1D tube wall
      if (dimensions_[i] == 1) {
        int y_lower_wall = whole_y_ / 2 + (-1 - grid_counts_[i].y() / 2);
        int y_upper_wall = whole_y_ / 2 + (grid_counts_[i].y() / 2 + 1);
        grid_conditions_[ID2(x_idx, y_lower_wall)].x() = WALL;
        grid_conditions_[ID2(x_idx, y_upper_wall)].x() = WALL;
      }
    }
    current_x_idx += grid_counts_[i].x();
  }

  // gather grids
  for (int i = 0; i < whole_grid_count_; i++) {
    auto state = static_cast<int>(grid_conditions_[i].x());
    switch (state) {
      case NORMAL1 :
        ids_1d_.push_back(i); break;
      case NORMAL2 :
        ids_2d_.push_back(i); break;
      case PML :
        ids_pml_.push_back(i); break;
      case EXCITER :
        ids_exc_.push_back(i); break;
      case JUNCTION_1to2_LEFT :
        ids_j12l_.push_back(i); break;
      case JUNCTION_1to2_RIGHT :
        ids_j12r_.push_back(i); break;
      case JUNCTION_2to1_LEFT :
        ids_j21l_.push_back(i); break;
      case JUNCTION_2to1_RIGHT :
        ids_j21r_.push_back(i); break;
      default :
        break;
    }
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
  frame_count_++;

  update_velocity();
  update_pressure();

  // update field buffer
  desc_sets_->write_to_buffer(0, 0, frame_index, field_.data());
  desc_sets_->flush_buffer(0, 0, frame_index);
}

void fdtd_horn::update_velocity()
{
  for (const auto& id : ids_1d_) {
    // retrieve x y coord

  }
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
        if (grid_conditions_[i + 1].x() != grid_type::WALL)
          field_[i].y() -= v_fac_ * (field_[i + 1].z() - field_[i].z());
        break;
      }

      case WALL : {
        break;
      }

      case EXCITER : {
        float freq = 1000; // Hz
        float amp = 1.f;
        float val = amp * std::sin(2.f * M_PI * freq * frame_count_ * dt_);
        field_[i].x() = val;// * (frame_count_ <= 10);
        break;
      }

      case PML : {
        auto pml = grid_conditions_[i].y();
        auto seg_id = int(grid_conditions_[i].w());
        auto y_grid_count = grid_counts_[seg_id].y();
        auto local_idx = i - int(edge_infos_[seg_id].y());
        auto not_upper = (local_idx % y_grid_count) != y_grid_count - 1;
        auto not_right = (local_idx / y_grid_count) != grid_counts_[seg_id].x() - 1;

        field_[i].x() = (field_[i].x()
                         - v_fac_ * (field_[i + y_grid_count].z() * not_right - field_[i].z())) / (1 + pml);
        field_[i].y() = (field_[i].y()
                         - v_fac_ * (field_[i + 1].z() * not_upper - field_[i].z())) / (1 + pml);

        break;
      }

      case JUNCTION_1to2_RIGHT : {
        // calc mean pressure
        auto seg_id = int(grid_conditions_[i].w());
        float mean_p = 0.f;
        float count = 0.f;
        int idx = -1;
        if (grid_conditions_[i + 1].x() == grid_type::PML) {
          int next_seg = grid_conditions_[i + 1].w();
          int next_y_grid_count = grid_counts_[next_seg].y();
          idx = i + 1 + pml_count_ * (next_y_grid_count + 1);
          // TODO : fix linear search
          for (int g = 0; g < next_y_grid_count - pml_count_ * 2; g++) {
            if (grid_conditions_[idx + g].x() == grid_type::JUNCTION_2to1_LEFT) {
              mean_p += field_[idx].z();
              count++;
            }
          }
        }
        else {
          idx = i + 2;
          while (grid_conditions_[idx].x() == grid_type::JUNCTION_2to1_LEFT) {
            mean_p += field_[idx].z();
            count++;
            idx++;
          }
        }

        mean_p /= std::max(1.f, count);
        field_[i].x() -= v_fac_ * (mean_p - field_[i].z());
        break;
      }

      case JUNCTION_2to1_RIGHT : {
        auto next_seg = grid_conditions_[i].w() + 1;
        auto next_idx = int(edge_infos_[next_seg].y());
        auto next_p = field_[next_idx].z();
        field_[i].x() -= v_fac_ * (next_p - field_[i].z());
        if (grid_conditions_[i + 1].x() != grid_type::WALL)
          field_[i].y() -= v_fac_ * (field_[i + 1].z() - field_[i].z());
        break;
      }

      default :
        throw std::runtime_error("unsupported grid state.");
    }
  }
}

void fdtd_horn::update_pressure()
{
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
        auto y_grid_count = int(grid_counts_[seg_id].y());
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
        field_[i].z() -= p_fac_ * field_[i].x();
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
                                          + field_[i].y() - field_[i - 1].y() * not_lower)
                        ) / (1 + pml);

        break;
      }

      case JUNCTION_1to2_LEFT : {
        // calc mean vx
        float mean_vx = 0.f;
        float count = 0.f;
        // TODO : pml
        if (grid_conditions_[i - 1].x() == PML) {

        }
        else {
          int idx = i - 2;
          while (grid_conditions_[idx].x() == JUNCTION_2to1_RIGHT) {
            mean_vx += field_[idx].x();
            count++;
            idx--;
          }
          mean_vx /= std::max(count, 1.f);
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
}

} // namespace hnll