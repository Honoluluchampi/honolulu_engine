#pragma once

// hnll
#include <game/component.hpp>
#include <utils/utils.hpp>

namespace hnll {

// forward declaration
namespace graphics { class renderer; }

namespace game {

DEFINE_COMPONENT(viewer_comp) {
  public:
    viewer_comp(const utils::transform &transform, graphics::renderer &renderer);
    ~viewer_comp();

    void update(const float& dt);
    void update_frustum();

    // getter
    static float get_near_distance() { return near_distance_; }
    static float get_fov_y() { return fov_y_; }

    const mat4 &get_projection()   const { return projection_matrix_; }
    const mat4 &get_view()         const { return view_matrix_; }
    const mat4 &get_inverse_view() const { return inverse_view_matrix_; }

    mat4 get_inverse_perspective_projection() const;
    mat4 get_inverse_view_yxz() const;

    // setter
    void set_orthogonal_projection(float left, float right, float top, float bottom, float near, float far);
    void set_perspective_projection(float fov_y, float aspect, float near, float far);

    // one of the three ways to initialize view matrix
    // camera position, which direction to point, which direction is up
    void set_view_direction(const vec3 &position, const vec3 &direction, const vec3 &up = vec3(0.f, -1.f, 0.f));
    void set_view_target(const vec3 &position, const vec3 &target, const vec3 &up = vec3(0.f, -1.f, 0.f));
    void set_view_yxz();

  private:
    // ref of owner's transform
    const utils::transform &transform_;
    mat4 projection_matrix_   = mat4::Identity();
    mat4 view_matrix_         = mat4::Identity();
    mat4 inverse_view_matrix_ = mat4::Identity();
    graphics::renderer &renderer_;

    // distance to the screen
    static float near_distance_, far_distance_;
    static float fov_y_;
};

}} // namespace hnll::game