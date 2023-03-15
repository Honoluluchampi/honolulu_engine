#pragma once

#include <utils/common_alias.hpp>

namespace hnll::geometry {

// forward declaration
struct vertex;
struct face;
class  half_edge;

using vertex_id     = uint32_t;
using vertex_map    = std::unordered_map<vertex_id, s_ptr<vertex>>;
using face_id       = uint32_t;
using face_map      = std::unordered_map<face_id, s_ptr<face>>;
using half_edge_key = std::pair<vertex, vertex>; // consists of two vertex_ids

struct vertex
{
  static s_ptr<vertex> create(const vec3& position, const vertex_id v_id = -1)
  {
    auto vertex_sp = std::make_shared<vertex>();
    vertex_sp->position_ = position;
    // identical id for each vertex object
    vertex_sp->id_ = v_id;
    return vertex_sp;
  }

  void update_normal(const vec3& new_face_normal)
  {
    auto tmp = normal_ * face_count_ + new_face_normal;
    normal_ = (tmp / ++face_count_).normalized();
  }

  vertex_id id_; // for half-edge hash table
  vec3 position_{0.f, 0.f, 0.f};
  vec3 color_   {1.f, 1.f, 1.f};
  vec3 normal_  {0.f, 0.f, 0.f};
  vec2 uv_;
  unsigned face_count_ = 0;
  s_ptr<half_edge> half_edge_ = nullptr;
};

struct face
{
  static s_ptr<face> create(const s_ptr<half_edge>& he)
  {
    auto face_sp = std::make_shared<face>();
    face_sp->half_edge_ = he;
    static face_id id = 0;
    face_sp->id_ = id++;
    return face_sp;
  }
  face_id id_;
  vec3 normal_;
  vec3 color_;
  s_ptr<half_edge> half_edge_ = nullptr;
  // for sdf separation
  double shape_diameter_ = -1;
};

class half_edge
{
  public:
    static s_ptr<half_edge> create(const s_ptr<vertex>& v)
    {
      auto half_edge_sp = std::make_shared<half_edge>();
      if (v->half_edge_ == nullptr) v->half_edge_ = half_edge_sp;
      half_edge_sp->set_vertex(v);
      return half_edge_sp;
    }

    // getter
    inline s_ptr<half_edge> get_next() const   { return next_; }
    inline s_ptr<half_edge> get_prev() const   { return prev_; }
    inline s_ptr<half_edge> get_pair() const   { return pair_; }
    inline s_ptr<face>      get_face() const   { return face_; }
    inline s_ptr<vertex>    get_vertex() const { return vertex_; }
    // setter
    inline void set_next(const s_ptr<half_edge>& next)  { next_ = next; }
    inline void set_prev(const s_ptr<half_edge>& prev)  { prev_ = prev; }
    inline void set_pair(const s_ptr<half_edge>& pair)  { pair_ = pair; }
    inline void set_face(const s_ptr<face>& face)       { face_ = face; }
    inline void set_vertex(const s_ptr<vertex>& vertex) { vertex_ = vertex; }

  private:
    s_ptr<half_edge> next_   = nullptr, prev_ = nullptr, pair_ = nullptr;
    s_ptr<vertex>    vertex_ = nullptr; // start point of this half_edge
    s_ptr<face>      face_   = nullptr; // half_edges run in a counterclockwise direction around this face
};

// for shape diameter function
struct ray
{
  vec3 origin;
  vec3 direction;
};

} // namespace hnll::geometry