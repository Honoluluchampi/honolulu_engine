#pragma once

// hnll
#include <utils/common_alias.hpp>

namespace hnll {

// wave solver on the heterogeneous dimensional grids
// currently max dimension is 2
class fdtd_horn
{
  public:
    using grid_id = uint32_t;

    static u_ptr<fdtd_horn> create(
      float dt,
      float dx,
      float rho,
      float c,
      int pml_count,
      std::vector<int> dimensions,
      std::vector<vec2> sizes);

    explicit fdtd_horn(
      float dt,
      float dx,
      float rho,
      float c,
      int pml_count,
      std::vector<int> dimensions,
      std::vector<vec2> sizes);

    // getter
    float get_dt()  const { return dt_; }
    float get_dx()  const { return dx_; }
    float get_rho() const { return rho_; }
    float get_c()   const { return c_; }
    int   get_pml_count() const { return pml_count_; }

    std::vector<int>     get_dimensions() const { return dimensions_; }
    std::vector<grid_id> get_start_grid_ids() const { return start_grid_ids_; }
    std::vector<ivec2>   get_grid_counts() const { return grid_counts_; }
    std::vector<vec4>    get_sizes() const { return sizes_; }
    int get_whole_grid_count() const { return whole_grid_count_; }

  private :
    float dt_;
    float dx_;
    float rho_;
    float c_; // sound speed
    int pml_count_;

    // dimension of each grid
    std::vector<int> dimensions_;
    // size of segment in meter
    std::vector<vec4> sizes_;
    std::vector<grid_id> start_grid_ids_;
    std::vector<ivec2> grid_counts_;
    int whole_grid_count_;
    // all of grid's value is packed into 1D vector
    // field.x : vx, y : vy, z : pressure, w : empty for now
    std::vector<vec4> field_;
    std::vector<vec4> grid_conditions_;
};

} // namespace hnll