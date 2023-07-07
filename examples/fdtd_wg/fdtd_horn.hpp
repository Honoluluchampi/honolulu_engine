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
      NORMAL = 0,
      PML = 1,
      PSEUDO_PML = 2,
      WALL = 3,
      EXCITER = 4,
    };

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
    int   get_pml_count() const { return pml_count_; }

    std::vector<int>     get_dimensions() const { return dimensions_; }
    std::vector<grid_id> get_start_grid_ids() const { return start_grid_ids_; }
    std::vector<ivec2>   get_grid_counts() const { return grid_counts_; }
    std::vector<vec4>    get_size_infos() const { return size_infos_; }
    int get_whole_grid_count() const { return whole_grid_count_; }
    float get_x_max() const { return size_infos_[segment_count_ - 1].z(); }

  private :
    float dt_;
    float dx_;
    float rho_;
    float c_; // sound speed
    int pml_count_;
    int segment_count_;

    // dimension of each grid
    std::vector<int> dimensions_;
    // contains whole grid count at the end of the vector
    std::vector<grid_id> start_grid_ids_;
    std::vector<ivec2> grid_counts_;
    int whole_grid_count_;

    // ******************** data for desc sets ***************************
    // all of grid's value is packed into 1D vector
    // field.x : vx, y : vy, z : pressure, w : empty for now
    std::vector<vec4> field_;
    // x : grid_type, y : pml coefficient, z : dimension
    std::vector<vec4> grid_conditions_;
    // x, y : size of segment, z : x edge, w : dimension
    std::vector<vec4> size_infos_;

    // ******************** desc sets **********************************
    s_ptr<graphics::desc_pool> desc_pool_;
    u_ptr<graphics::desc_sets> desc_sets_;
    u_ptr<graphics::desc_layout> desc_layout_;
};

} // namespace hnll