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
    explicit bounding_volume(const vec3d &center_point = {0.f, 0.f, 0.f}, const vec3d& radius = {0.f, 0.f, 0.f})
      : center_point_(center_point), radius_(radius), bv_type_(bv_type::AABB), transform_(std::make_shared<utils::transform>()) {}

    // ctor for sphere
    explicit bounding_volume(const vec3d &center_point = {0.f, 0.f, 0.f}, const double radius = 1.f)
      : center_point_(center_point), radius_(radius, 0.f, 0.f), bv_type_(bv_type::SPHERE), transform_(std::make_shared<utils::transform>()) {}

    // bounding_volumes are owned only by rigid_component
    static u_ptr<bounding_volume> create_aabb(const std::vector<vec3d> &vertices);
    static u_ptr<bounding_volume> create_blank_aabb(const vec3d& initial_point = {0.f, 0.f, 0.f}); // for mesh separation
    static u_ptr<bounding_volume> create_bounding_sphere(bv_ctor_type type, const std::vector<vec3d> &vertices);
    static u_ptr<bounding_volume> ritter_ctor(const std::vector<vec3d> &vertices);

    // getter
    inline bv_type get_bv_type() const      { return bv_type_; }
    // TODO : auto update for every member function
    inline vec3d get_world_center_point() const { return transform_->rotate_mat3().cast<double>() * center_point_ + transform_->translation.cast<double>(); }
    inline vec3d get_local_center_point() const { return center_point_; }
    inline vec3d get_aabb_radius()        const { return radius_; }
    inline double get_sphere_radius()    const { return radius_.x(); }
    inline bool is_sphere()              const { return bv_type_ == bv_type::SPHERE; }
    inline bool is_aabb()                const { return bv_type_ == bv_type::AABB; }
    // aabb getter
    inline double get_max_x() const { return center_point_.x() + radius_.x(); }
    inline double get_min_x() const { return center_point_.x() - radius_.x(); }
    inline double get_max_y() const { return center_point_.y() + radius_.y(); }
    inline double get_min_y() const { return center_point_.y() - radius_.y(); }
    inline double get_max_z() const { return center_point_.z() + radius_.z(); }
    inline double get_min_z() const { return center_point_.z() - radius_.z(); }
    // setter
    void set_bv_type(bv_type type)                               { bv_type_ = type; }
    void set_center_point(const vec3d &center_point)             { center_point_ = center_point; }
    void set_aabb_radius(const vec3d& radius)                    { radius_ = radius; }
    void set_sphere_radius(const double radius)                  { radius_.x() = radius; }
    void set_transform(const s_ptr<utils::transform>& transform) { transform_ = transform; }
  private:
    bv_type bv_type_;
    vec3d center_point_;
    // if bv_type == SPHERE, only radius_.x() is valid.
    vec3d radius_;
    s_ptr<utils::transform> transform_;
};

// support functions
std::pair<int, int> most_separated_points_on_aabb(const std::vector<vec3d> &vertices);

u_ptr<bounding_volume> sphere_from_distant_points(const std::vector<vec3d> &vertices);

void extend_sphere_to_point(bounding_volume &sphere, const vec3d &point);

} // namespace geometry
} // namespace hnll