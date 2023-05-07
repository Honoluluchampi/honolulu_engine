#pragma once

// hnll
#include <utils/common_alias.hpp>

// std
#include <set>

namespace hnll {

namespace utils { struct transform; }

namespace geometry {

// forward declaration
struct vertex;
struct face;
class  half_edge;

using vertex_id     = uint32_t;
using vertex_id_set = std::set<vertex_id>;
using vertex_map    = std::unordered_map<vertex_id, vertex>;
using face_id       = uint32_t;
using face_id_set   = std::set<face_id>;
using face_map      = std::unordered_map<face_id, face>;
using half_edge_id  = uint32_t;
using half_edge_set = std::set<half_edge_id>;
using half_edge_key = std::pair<vertex, vertex>; // consists of two vertex_ids
using half_edge_map = std::unordered_map<half_edge_key, half_edge&>;
using half_edge_id_map = std::unordered_map<half_edge_id, half_edge>;

// all ids should be set manually

struct vertex
{
  vertex(const vec3d& pos, vertex_id v_id_)
  {
    position = pos;
    v_id = v_id_;
  }

  void update_normal(const vec3d& new_face_normal)
  {
    // take the mean between all adjoining faces
    auto tmp = normal * face_count + new_face_normal;
    normal = (tmp / ++face_count).normalized();
  }

  vertex_id v_id; // for half-edge hash table
  vec3d position{0.f, 0.f, 0.f};
  vec3d color   {1.f, 1.f, 1.f};
  vec3d normal  {0.f, 0.f, 0.f};
  vec2d uv;
  unsigned face_count = 0;
  half_edge_id he_id = -1;
};

struct face
{
  face(face_id f_id_, half_edge_id he_id_ = -1) { f_id = f_id_; he_id = he_id_; }
  face_id f_id;
  vec3d normal;
  vec3d color;
  half_edge_id he_id;
};

class half_edge
{
  public:
    half_edge(vertex& v, half_edge_id he_id)
    {
      this_id = he_id;

      if (v.he_id == -1)
        v.he_id = this_id;
      // set vertex to this he
      v_id = v.v_id;
    }

    half_edge_id this_id, next = -1, prev = -1, pair = -1;
    vertex_id    v_id = -1; // start point of this half_edge
    face_id      f_id = -1; // half_edges run in a counterclockwise direction around this face
};


struct plane
{
  vec3d point;
  vec3d normal;
  // plane's normal is guaranteed to be normalized
  plane(const vec3d& point_, const vec3d& normal_) : point(point_), normal(normal_) { normal.normalize(); }
  plane() : normal({0.f, -1.f, 0.f}) {}
};

class perspective_frustum
{
  public:
    enum class update_fov_x { ON, OFF };

    static s_ptr<perspective_frustum> create(double fov_x, double fov_y, double near_z, double far_z);
    perspective_frustum(double fov_x, double fov_y, double near_z, double far_z);
    void update_planes(const utils::transform& tf);

    // getter
    vec3d get_near_n()   const { return near_.normal; }
    vec3d get_far_n()    const { return far_.normal; }
    vec3d get_left_n()   const { return left_.normal; }
    vec3d get_right_n()  const { return right_.normal; }
    vec3d get_top_n()    const { return top_.normal; }
    vec3d get_bottom_n() const { return bottom_.normal; }
    vec3d get_near_p()   const { return near_.point;}
    vec3d get_far_p()    const { return far_.point;}
    vec3d get_left_p()   const { return left_.point;}
    vec3d get_right_p()  const { return right_.point;}
    vec3d get_top_p()    const { return top_.point;}
    vec3d get_bottom_p() const { return bottom_.point;}
    const plane& get_near()   const { return near_; }
    const plane& get_far()    const { return far_; }
    const plane& get_left()   const { return left_; }
    const plane& get_right()  const { return right_; }
    const plane& get_top()    const { return top_; }
    const plane& get_bottom() const { return bottom_; }
    double get_near_z() const { return near_z_; }
    double get_far_z()  const { return far_z_; }
    const std::array<vec3d, 4>& get_default_points() const { return default_points_; }

    // setter
    void set_fov_x(double fx)  { fov_x_ = fx; }
    void set_fov_y(double fy)  { fov_y_ = fy; }
    void set_near_z(double nz) { near_z_ = nz; }
    void set_far_z(double fz)  { far_z_ = fz; }

  private:
    plane  near_, far_, left_, right_, top_, bottom_;
    double fov_x_ = M_PI / 4.f, fov_y_ = M_PI / 4.f, near_z_, far_z_;
    update_fov_x update_fov_x_ = update_fov_x::ON;
    // start from upper left point, and goes around near plane in counter-clockwise direction
    std::array<vec3d, 4> default_points_;
};


}} // namespace hnll::geometry