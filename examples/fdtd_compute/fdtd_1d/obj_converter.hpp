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

  int get_vert_id(int offset_id, int per_circle_id, bool outer)
  {
    return (VERTEX_PER_CIRCLE * 2 * offset_id) + per_circle_id + VERTEX_PER_CIRCLE * int(outer);
  }

  int get_body_norm_id(int per_circle_id, bool outer)
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

// TODO adjust a mouthpiece
// takes the list of the offsets of each bore segment.
void convert_to_obj(std::string name, float dx, float thickness, const std::vector<float>& offsets)
{
  auto segment_count = offsets.size();

  obj_model model;
  model.name = name;
  // register the vertices
  for (int i = 0; i < segment_count; i++) {
    auto radius = offsets[i];
    // add inner vertices
    for (int j = 0; j < model.VERTEX_PER_CIRCLE; j++) {
      auto phi = 2.f * M_PI * j / float(model.VERTEX_PER_CIRCLE);
      model.vertices.emplace_back(model.scale * vec3(
        dx * float(i),
        radius * float(std::sin(phi)),
        radius * float(std::cos(phi)))
      );
    }
    // add outer vertices
    for (int j = 0; j < model.VERTEX_PER_CIRCLE; j++) {
      auto phi = 2.f * M_PI * j / float(model.VERTEX_PER_CIRCLE);
      model.vertices.emplace_back(model.scale * vec3(
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
      // inner and outer planes
      for (int in_out = 0; in_out <= 1; in_out++) {
        model.planes.emplace_back(
          ivec3(
            model.get_vert_id(i, j, bool(in_out)),
            model.get_vert_id(i + 1, j, bool(in_out)),
            model.get_vert_id(i + 1, next_j, bool(in_out))),
          ivec3(
            model.get_body_norm_id(j, bool(in_out)),
            model.get_body_norm_id(j, bool(in_out)),
            model.get_body_norm_id(next_j, bool(in_out)))
        );
        model.planes.emplace_back(
          ivec3(
            model.get_vert_id(i, j, bool(in_out)),
            model.get_vert_id(i + 1, next_j, bool(in_out)),
            model.get_vert_id(i, next_j, bool(in_out))),
          ivec3(
            model.get_body_norm_id(j, bool(in_out)),
            model.get_body_norm_id(next_j, bool(in_out)),
            model.get_body_norm_id(next_j, bool(in_out)))
        );
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