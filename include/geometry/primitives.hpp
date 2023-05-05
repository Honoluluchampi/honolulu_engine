#pragma once

#include <utils/common_alias.hpp>

namespace hnll::geometry {

// forward declaration
struct vertex;
struct face;
class  half_edge;

using vertex_id     = uint32_t;
using vertex_map    = std::unordered_map<vertex_id, vertex>;
using face_id       = uint32_t;
using face_map      = std::unordered_map<face_id, face>;
using half_edge_id  = uint32_t;
using half_edge_key = std::pair<vertex, vertex>; // consists of two vertex_ids
using half_edge_map = std::unordered_map<half_edge_key, half_edge>;

struct vertex
{
  vertex(const vec3& pos, vertex_id v_id_ = -1)
  {
    position = pos;
    v_id = v_id_;
  }

  void update_normal(const vec3& new_face_normal)
  {
    // take the mean between all adjoining faces
    auto tmp = normal * face_count + new_face_normal;
    normal = (tmp / ++face_count).normalized();
  }

  vertex_id v_id; // for half-edge hash table
  vec3 position{0.f, 0.f, 0.f};
  vec3 color   {1.f, 1.f, 1.f};
  vec3 normal  {0.f, 0.f, 0.f};
  vec2 uv;
  unsigned face_count = 0;
  half_edge_id he_id = -1;
};

struct face
{
  face(face_id f_id_ = -1, half_edge_id he_id_ = -1)
  {
    f_id = f_id_;
    he_id = he_id_;
  }
  face_id f_id;
  vec3 normal;
  vec3 color;
  half_edge_id he_id;
};

class half_edge
{
  public:
    half_edge(vertex_map& v_map, vertex_id v_id_)
    {
      auto& v = v_map.at(v_id_);
      if (v.he_id == -1)
        v.he_id = this_id;
      // set vertex to this he
      v_id = v.v_id;
    }

    half_edge_id this_id = -1, next = -1, prev = -1, pair = -1;
    vertex_id    v_id = -1; // start point of this half_edge
    face_id      f_id = -1; // half_edges run in a counterclockwise direction around this face
};

} // namespace hnll::geometry