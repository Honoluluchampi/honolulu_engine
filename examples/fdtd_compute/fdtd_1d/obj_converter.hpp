#pragma once

#include <utils/common_alias.hpp>
#include <fstream>

namespace hnll {

struct obj_model
{
  struct plane
  {
    ivec3 vertices;
    ivec3 normals;
  };

  std::vector<vec3> vertices;
  std::vector<vec3> normals;
  std::vector<plane> planes; // three indices of the triangle vertices
  int VERTEX_PER_CIRCLE = 32;
  float scale = 100.f;
  std::string name;

  // returns added vertex's id
  size_t add_vert(const auto& vec3)
  {
    auto ret = vertices.size();
    vertices.emplace_back(vec3);
    return ret;
  }

  const vec3& get_vert(int offset_id, int per_circle_id, bool outer) const
  { return vertices[get_vert_id(offset_id, per_circle_id, outer)]; }

  int get_vert_id(int offset_id, int per_circle_id, bool outer) const
  { return (VERTEX_PER_CIRCLE * 2 * offset_id) + per_circle_id + VERTEX_PER_CIRCLE * int(outer); }

  int get_body_norm_id(int per_circle_id, bool outer) const
  {
    // 0 : -1, 0, 0, (for input edge)
    // 1 : 1, 0, 0, (for output edge)
    // 2 ~ VERTEX_PER_CIRCLE + 1 : inner body
    // VERTEX_PER_CIRCLE + 2 ~ : outer body
    return per_circle_id + 2 + VERTEX_PER_CIRCLE * int(outer);
  }

  void write()
  {
    std::ofstream writing_file;
    writing_file.open(name, std::ios::out);
    writing_file << "mtllib " << name << std::endl;

    if (!writing_file.is_open())
      std::cerr << "failed to open file : " << name << std::endl;

    // write vertex value
    for (const auto& vert : vertices)
      writing_file << "v " << vert.x() << " " << vert.y() << " " << vert.z() << std::endl;

    // write normal value
    for (const auto& norm : normals)
      writing_file << "vn " << norm.x() << " " << norm.y() << " " << norm.z() << std::endl;

    // write plane with 1-index indices
    for (const auto& plane : planes)
      writing_file << "f " <<
        plane.vertices.x() + 1 << "//" << plane.normals.x() + 1<< " " <<
        plane.vertices.y() + 1 << "//" << plane.normals.y() + 1<< " " <<
        plane.vertices.z() + 1 << "//" << plane.normals.z() + 1<< " " <<
        std::endl;

    writing_file.close();
  }
};

// returns true if an intersection is detected
bool test_point_hole_intersection(const vec3& point, float hole_x, float hole_radius)
{ return (vec2(point.x(), point.z()) - vec2(hole_x, 0.f)).norm() <= hole_radius; }

// start should be out of hole cylinder
// end should be in the hole cylinder
// therefore, the output is guaranteed to be the closer intersecting point
vec3 test_line_hole_intersection(const vec3& start, const vec3& end, float hole_x, float hole_radius)
{
  float a = std::pow(end.x() - start.x(), 2.f) + std::pow(end.z() - start.z(), 2.f);
  float b = 2.f * ((end.x() - start.x()) * (start.x() - hole_x) + (end.z() - start.z()) * start.z());
  float c = std::pow(start.x() - hole_x, 2.f) + std::pow(start.z(), 2.f) - std::pow(hole_radius, 2.f);

  float D = b * b - 4.f * a * c;
  assert(D >= 0 && "test_line_hole_intersection");

  float t = (-b - std::sqrt(D)) / (2.f * a);
  assert(t <= 1.f && t >= 0.f);

  return t * (end - start) + start;
}

// TODO adjust a mouthpiece
// takes the list of the offsets of each bore segment.
void convert_to_obj(
  std::string name,
  float dx,
  float thickness,
  float start_x,
  const std::vector<float>& offsets,
  const std::vector<int>& hole_ids,
  float hole_radius)
{
  auto segment_count = offsets.size();

  obj_model model;
  model.name = name;
  // register the vertices
  for (int i = 0; i < segment_count; i++) {
    if (dx * float(i) < start_x)
      continue;

    auto radius = offsets[i];
    // add inner vertices
    for (int j = 0; j < model.VERTEX_PER_CIRCLE; j++) {
      auto phi = 2.f * M_PI * j / float(model.VERTEX_PER_CIRCLE);
      model.add_vert(model.scale * vec3(
        dx * float(i),
        radius * float(std::sin(phi)),
        radius * float(std::cos(phi)))
      );
    }
    // add outer vertices
    for (int j = 0; j < model.VERTEX_PER_CIRCLE; j++) {
      auto phi = 2.f * M_PI * j / float(model.VERTEX_PER_CIRCLE);
      model.add_vert(model.scale * vec3(
        dx * float(i),
        (radius + thickness) * float(std::sin(phi)),
        (radius + thickness) * float(std::cos(phi)))
      );
    }
  }

  // register normals
  {
    // input edge
    model.normals.emplace_back(-1, 0, 0);
    // output edge
    model.normals.emplace_back(1, 0, 0);
    // body
    for (int inout = 0; inout <=1; inout++) {
      for (int i = 0; i < model.VERTEX_PER_CIRCLE; i++) {
        auto sign = 2.f * float(inout) - 1.f;
        auto phi = 2.f * M_PI * i / float(model.VERTEX_PER_CIRCLE);
        model.normals.emplace_back(0, sign * std::sin(phi), sign * std::cos(phi));
      }
    }
  }

  // register the planes
  // input edge
  for (int i = 0; i < model.VERTEX_PER_CIRCLE; i++) {
    auto next_i = (i + 1) % model.VERTEX_PER_CIRCLE;
    model.planes.emplace_back(
      ivec3(
        model.get_vert_id(0, i, false),
        model.get_vert_id(0, i, true),
        model.get_vert_id(0, next_i, true)),
      ivec3(0, 0, 0)
    );
    model.planes.emplace_back(
      ivec3(
        model.get_vert_id(0, i, false),
        model.get_vert_id(0, next_i, true),
        model.get_vert_id(0, next_i, false)),
      ivec3(0, 0, 0)
    );
  }

  // body
  for (int i = 0; i < segment_count; i++) {
    for (int j = 0; j < model.VERTEX_PER_CIRCLE; j++) {
      auto next_j = (j + 1) % model.VERTEX_PER_CIRCLE;

      int id_set[2][6] = {
        { i, j, i + 1, j, i + 1, next_j },
        { i, j, i + 1, next_j, i, next_j }
      };

      // loop on the 2 triangles which makes the body's square
      for (int k = 0; k < 2; k++) {
        // count the number of the intersecting points
        int count = 0;
        int intersecting_vert_id = -1;
        int detect_remaining_vert = 0;
        float hole_x;
        std::vector<vec3> vertices = {
          model.get_vert(id_set[k][0], id_set[k][1], true),
          model.get_vert(id_set[k][2], id_set[k][3], true),
          model.get_vert(id_set[k][4], id_set[k][5], true)
        };

        for (const auto& hole : hole_ids) {
          for (int l = 0; l < 3; l++) {
            if (test_point_hole_intersection(vertices[l], dx * float(hole) * model.scale, hole_radius * model.scale)) {
              count++;
              intersecting_vert_id = l;
              detect_remaining_vert += l;
              hole_x = dx * float(hole) * model.scale;
            }
          }
        }

        // inner and outer planes
        for (int in_out = 0; in_out <= 1; in_out++) {
          // do not make hole on the bottom side of the bore
          if (count == 0 || vertices[0].y() < 0) {
            model.planes.emplace_back(
              ivec3(
                model.get_vert_id(id_set[k][0], id_set[k][1], bool(in_out)),
                model.get_vert_id(id_set[k][2], id_set[k][3], bool(in_out)),
                model.get_vert_id(id_set[k][4], id_set[k][5], bool(in_out))),
              ivec3(
                model.get_body_norm_id(id_set[k][1], bool(in_out)),
                model.get_body_norm_id(id_set[k][3], bool(in_out)),
                model.get_body_norm_id(id_set[k][5], bool(in_out)))
            );
          }
          // close the hole side
          if (count == 1 && vertices[0].y() > 0) {
            // id of the remaining two vertices
            int id1 = ((intersecting_vert_id + 1) * 2 ) % 6;
            int id2 = ((intersecting_vert_id + 2) * 2 ) % 6;
            model.planes.emplace_back(
              ivec3(
                model.get_vert_id(id_set[k][id1], id_set[k][id1 + 1], true),
                model.get_vert_id(id_set[k][id2], id_set[k][id2 + 1], true),
                model.get_vert_id(id_set[k][id2], id_set[k][id2 + 1], false)),
              ivec3(
                model.get_body_norm_id(id_set[k][id1 + 1], true),
                model.get_body_norm_id(id_set[k][id2 + 1], true),
                model.get_body_norm_id(id_set[k][id2 + 1], false))
              );
            model.planes.emplace_back(
              ivec3(
                model.get_vert_id(id_set[k][id1], id_set[k][id1 + 1], true),
                model.get_vert_id(id_set[k][id2], id_set[k][id2 + 1], false),
                model.get_vert_id(id_set[k][id1], id_set[k][id1 + 1], false)),
              ivec3(
                model.get_body_norm_id(id_set[k][id1 + 1], true),
                model.get_body_norm_id(id_set[k][id2 + 1], false),
                model.get_body_norm_id(id_set[k][id1 + 1], false))
            );
          }

          if (count == 2 && vertices[0].y() > 0) {
            int out_id_raw = 3 - detect_remaining_vert;
            int out_id = out_id_raw * 2;
            int id1 = ((out_id_raw + 1) % 3) * 2;
            int id2 = ((out_id_raw + 2) % 3) * 2;
            // add a small triangle
            const auto& out_v = model.get_vert(id_set[k][out_id], id_set[k][out_id + 1], true);
            // intersecting 2 vertices
            const auto& v1 = model.get_vert(id_set[k][id1], id_set[k][id1 + 1], true);
            const auto& v2 = model.get_vert(id_set[k][id2], id_set[k][id2 + 1], true);

            auto new_v1 = test_line_hole_intersection(out_v, v1, hole_x, hole_radius * model.scale);
            auto new_v2 = test_line_hole_intersection(out_v, v2, hole_x, hole_radius * model.scale);

            auto new_v1_id = model.add_vert(new_v1);
            auto new_v2_id = model.add_vert(new_v2);

            model.planes.emplace_back(
              ivec3(
                model.get_vert_id(id_set[k][out_id], id_set[k][out_id + 1], true),
                new_v1_id,
                new_v2_id),
              ivec3(
                model.get_body_norm_id(id_set[k][out_id + 1], true),
                model.get_body_norm_id(id_set[k][out_id + 1], true),
                model.get_body_norm_id(id_set[k][out_id + 1], true))
            );
          }
        }
      }
    }
  }

  // output edge
  for (int i = 0; i < model.VERTEX_PER_CIRCLE; i++) {
    auto next_i = (i + 1) % model.VERTEX_PER_CIRCLE;
    model.planes.emplace_back(
      ivec3(
        model.get_vert_id(segment_count - 1, i, false),
        model.get_vert_id(segment_count - 1, i, true),
        model.get_vert_id(segment_count - 1, next_i, true)),
      ivec3(1, 1, 1)
    );
    model.planes.emplace_back(
      ivec3(
        model.get_vert_id(segment_count - 1, i, false),
        model.get_vert_id(segment_count - 1, next_i, true),
        model.get_vert_id(segment_count - 1, next_i, false)),
      ivec3(1, 1, 1)
    );
  }

  model.write();
}

} // namespace hnll