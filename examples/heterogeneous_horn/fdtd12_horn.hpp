#pragma once

// hnll
#include <graphics/desc_set.hpp>
#include <utils/rendering_utils.hpp>
#include <utils/common_alias.hpp>

namespace hnll {

// wave solver on the heterogeneous dimensional grids
// currently max dimension is 2
class fdtd_horn
{
  public:
    using grid_id = uint32_t;

    // common desc binding configuration for all desc sets
    static graphics::binding_info common_binding_info;

    enum grid_type {
      EMPTY,
      NORMAL1,
      NORMAL2,
      WALL,
      EXCITER,
      PML,
      JUNCTION_1to2_LEFT,
      JUNCTION_1to2_RIGHT,
      JUNCTION_2to1_LEFT,
      JUNCTION_2to1_RIGHT,
    };

    static u_ptr<fdtd_horn> create(
      float dt,
      float dx,
      float rho,
      float c,
      int pml_count,
      float pml_max,
      std::vector<int> dimensions,
      std::vector<vec2> sizes);

    explicit fdtd_horn(
      float dt,
      float dx,
      float rho,
      float c,
      int pml_count,
      float pml_max,
      std::vector<int> dimensions,
      std::vector<vec2> sizes);

    void update(int frame_index);

    // graphics process
    void build_desc(graphics::device& device);
    inline void bind(VkCommandBuffer command) {}
    inline void draw(VkCommandBuffer command) { vkCmdDraw(command, 6, 1, 0, 0); }
    inline uint32_t get_rc_id() const { return 0; }
    inline utils::shading_type get_shading_type() const { return utils::shading_type::UNIQUE; }
    inline std::vector<VkDescriptorSet> get_vk_desc_sets(int frame_index) const
    { return desc_sets_->get_vk_desc_sets(frame_index); }

    // getter
    float get_dt()  const { return dt_; }
    float get_dx()  const { return dx_; }
    float get_rho() const { return rho_; }
    float get_c()   const { return c_; }
    int   get_segment_count() const { return segment_count_; };
    int   get_pml_count() const { return pml_count_; }

    std::vector<int>   get_dimensions() const { return dimensions_; }
    std::vector<ivec2> get_grid_counts() const { return grid_counts_; }
    std::vector<vec4>  get_size_infos() const { return size_infos_; }
    std::vector<vec4>  get_edge_infos() const { return edge_infos_; }

    const std::vector<vec4>& get_field() const { return field_; }
    const std::vector<vec4>& get_grid_conditions() const { return grid_conditions_; }

    int   get_whole_x() const { return whole_x_; }
    int   get_whole_y() const { return whole_y_; }
    float get_x_max() const { return x_max_; }
    float get_y_max() const { return y_max_; }

    // for test
    const std::vector<int>& get_ids_1d() const { return ids_1d_; }
    const std::vector<int>& get_ids_2d() const { return ids_2d_; }
    const std::vector<int>& get_ids_pml() const { return ids_pml_; }
    const std::vector<int>& get_ids_gal() const { return ids_gal_; } // gradually averaging layers
    const std::vector<int>& get_ids_exc() const { return ids_exc_; }
    const std::vector<int>& get_ids_j12l() const { return ids_j12l_; } // JUNCTION_1to2_LEFT
    const std::vector<int>& get_ids_j12r() const { return ids_j12r_; }
    const std::vector<int>& get_ids_j21l() const { return ids_j21l_; }
    const std::vector<int>& get_ids_j21r() const { return ids_j21r_; }

  private :
    void update_velocity();
    void update_pressure();

    float dt_;
    float dx_;
    float rho_;
    float c_; // sound speed
    float v_fac_;
    float p_fac_;
    int pml_count_;
    float pml_max_;
    int segment_count_;
    int frame_count_ = 0;

    // dimension of each grid
    std::vector<int> dimensions_;
    int whole_grid_count_;
    int whole_x_;
    int whole_y_;
    int active_grid_count_;

    float x_max_;
    float y_max_;

    std::vector<int> ids_1d_;
    std::vector<int> ids_2d_;
    std::vector<int> ids_pml_;
    std::vector<int> ids_gal_; // gradually averaging layers
    std::vector<int> ids_exc_;
    std::vector<int> ids_j12l_; // JUNCTION_1to2_LEFT
    std::vector<int> ids_j12r_;
    std::vector<int> ids_j21l_;
    std::vector<int> ids_j21r_;

    // ******************** data for desc sets ***************************
    // all of grid's value is packed into 1D vector
    // field.x : vx, y : vy, z : pressure, w : empty for now
    std::vector<vec4> field_;
    // x : grid_type, y : pml coefficient, z : dimension, w : segment_id
    std::vector<vec4> grid_conditions_;
    // x, y : size of segment, z : x edge, w : dimension
    std::vector<vec4> size_infos_;
    // x : x edge, y : starting_grid_id
    std::vector<vec4> edge_infos_;
    // y = 1 if 1D
    std::vector<ivec2> grid_counts_;

    // ******************** desc sets **********************************
    s_ptr<graphics::desc_pool> desc_pool_;
    u_ptr<graphics::desc_sets> desc_sets_;
    u_ptr<graphics::desc_layout> desc_layout_;
};

} // namespace hnll