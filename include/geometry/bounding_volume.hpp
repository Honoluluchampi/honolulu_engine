#pragma once

// hnll
#include <utils/utils.hpp>
#include <utils/common_alias.hpp>

namespace hnll {

namespace geometry {

// bounding volume type
enum class bv_type
{
  SPHERE,
  AABB
};

enum class bv_ctor_type
{
  RITTER,
};

class bounding_volume
{
  public:
    // ctor for aabb
    explicit bounding_volume(const vec3d &center_point = {0.f, 0.f, 0.f}, const vec3d& radius = {0.f, 0.f, 0.f});
    // ctor for sphere
    explicit bounding_volume(const vec3d &center_point = {0.f, 0.f, 0.f}, const double radius = 1.f);

    // for aabb
    // currently bounding_volumes are owned only by rigid_component
    static u_ptr<bounding_volume> create_aabb(const std::vector<vec3d> &vertices);
    static u_ptr<bounding_volume> create_aabb(const vec3d& center, const vec3d& radius);
    // for sphere
    static u_ptr<bounding_volume> create_sphere(const std::vector<vec3d>& vertices);
    static u_ptr<bounding_volume> create_sphere(const vec3d& center, const double radius);
    // for mesh separation
    static u_ptr<bounding_volume> create_empty_bv(bv_type type, const vec3d& initial_point = {0.f, 0.f, 0.f});

    // getter
    inline bv_type get_bv_type() const { return type_; }
    inline bool is_sphere()      const { return type_ == bv_type::SPHERE; }
    inline bool is_aabb()        const { return type_ == bv_type::AABB; }

    // TODO : auto update for every member function
    inline vec3d get_world_center_point() const { return transform_->rotate_mat3().cast<double>() * center_point_ + transform_->translation.cast<double>(); }
    inline vec3d get_local_center_point() const { return center_point_; }
    inline vec3d get_aabb_radius()        const { return radius_; }
    inline double get_sphere_radius()     const { return radius_.x(); }

    // aabb getter
    inline double get_max_x() const { return center_point_.x() + radius_.x(); }
    inline double get_min_x() const { return center_point_.x() - radius_.x(); }
    inline double get_max_y() const { return center_point_.y() + radius_.y(); }
    inline double get_min_y() const { return center_point_.y() - radius_.y(); }
    inline double get_max_z() const { return center_point_.z() + radius_.z(); }
    inline double get_min_z() const { return center_point_.z() - radius_.z(); }
    // setter
    void set_center_point(const vec3d &center_point)             { center_point_ = center_point; }
    void set_aabb_radius(const vec3d& radius)                    { radius_ = radius; }
    void set_sphere_radius(const double radius)                  { radius_.x() = radius; }
    void set_transform(const s_ptr<utils::transform>& transform) { transform_ = transform; }
  private:
    vec3d center_point_;
    // if bv_type == SPHERE, only radius_.x() is valid.
    vec3d radius_;
    s_ptr<utils::transform> transform_;
    bv_type type_;
};

// support functions
std::pair<int, int> most_separated_points_on_aabb(const std::vector<vec3d> &vertices);

} // namespace geometry
} // namespace hnll