#pragma once

#include <utils/common_alias.hpp>
#include <fstream>

namespace hnll {

struct obj_model
{
  std::vector<vec3> vertices;
  std::vector<ivec3> planes; // three indices of the triangle vertices
  int VERTEX_PER_CIRCLE = 32;
  float scale = 100.f;
  std::string name;

  int get_id(int offset_id, int per_circle_id, bool outer)
  {
    return (VERTEX_PER_CIRCLE * 2 * offset_id) + per_circle_id + VERTEX_PER_CIRCLE * int(outer);
  }

  void write()
  {
    std::ofstream writing_file;
    writing_file.open(name, std::ios::out);
    writing_file << "mtllib " << name << std::endl;

    if (!writing_file.is_open())
      std::cerr << "failed to open file : " << name << std::endl;

    for (const auto& vert : vertices)
      writing_file << "v " << vert.x() << " " << vert.y() << " " << vert.z() << std::endl;

    // temporal
    writing_file << "vn -1, 0, 0" << std::endl;

    for (const auto& plane : planes)
      writing_file << "f " << plane.x() + 1 << "//1 " << plane.y() + 1 << "//1 " << plane.z() + 1 << "//1 " << std::endl;

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
        dx * i,
        radius * std::sin(phi),
        radius * std::cos(phi))
      );
    }
    // add outer vertices
    for (int j = 0; j < model.VERTEX_PER_CIRCLE; j++) {
      auto phi = 2.f * M_PI * j / float(model.VERTEX_PER_CIRCLE);
      model.vertices.emplace_back(model.scale * vec3(
        dx * i,
        (radius + thickness) * std::sin(phi),
        (radius + thickness) * std::cos(phi))
      );
    }
  }

  // register the planes
  // input edge
  for (int i = 0; i < model.VERTEX_PER_CIRCLE; i++) {
    auto next_i = (i + 1) % model.VERTEX_PER_CIRCLE;
    model.planes.emplace_back(
      model.get_id(0, i, false),
      model.get_id(0, i, true),
      model.get_id(0, next_i, true)
    );
    model.planes.emplace_back(
      model.get_id(0, i, false),
      model.get_id(0, next_i, true),
      model.get_id(0, next_i, false)
    );
  }

  model.write();
}

} // namespace hnll