#pragma once

#include <utils/common_alias.hpp>
#include <fstream>

namespace hnll {

// compatible with mcut
struct obj_model
{
  std::vector<double> vertex_coords;
  std::vector<uint32_t> face_indices;
  std::vector<uint32_t> face_sizes;

  uint32_t vertex_count = 0;
  uint32_t face_count = 0;

  int VERTEX_PER_CIRCLE = 32;
  float scale = 100.f;
  std::string name;

  int get_vert_id(int offset_id, int per_circle_id, bool outer) const
  { return (VERTEX_PER_CIRCLE * 2 * offset_id) + per_circle_id + VERTEX_PER_CIRCLE * int(outer); }

  void write()
  {
    std::ofstream writing_file;
    writing_file.open(name, std::ios::out);
    writing_file << "mtllib " << name << std::endl;

    if (!writing_file.is_open())
      std::cerr << "failed to open file : " << name << std::endl;

    // write vertex value
    assert(vertex_coords.size() / 3 == vertex_count);
    assert(vertex_coords.size() % 3 == 0);
    for (int i = 0; i < vertex_coords.size() / 3; i++)
      writing_file << "v "
        << vertex_coords[i * 3 + 0] * scale << " "
        << vertex_coords[i * 3 + 1] * scale << " "
        << vertex_coords[i * 3 + 2] * scale << " "
        << std::endl;

    // temp : single norm
    writing_file << "vn " << 0 << " " << 1 << " " << 0 << std::endl;

    // write plane with 1-index indices
    assert(face_indices.size() / 3 == face_count);
    assert(face_indices.size() % 3 == 0);
    for (int i = 0; i < face_indices.size() / 3; i++)
      writing_file << "f "
        << face_indices[i * 3 + 0] + 1 << "//" << 1 << " "
        << face_indices[i * 3 + 1] + 1 << "//" << 1 << " "
        << face_indices[i * 3 + 2] + 1 << "//" << 1 << " "
        << std::endl;

    writing_file.close();
  }
};

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
    auto radius = offsets[i];
    // add inner vertices
    for (int j = 0; j < model.VERTEX_PER_CIRCLE; j++) {
      auto phi = 2.f * M_PI * j / float(model.VERTEX_PER_CIRCLE);
      model.vertex_coords.emplace_back(dx * float(i));
      model.vertex_coords.emplace_back(radius * float(std::sin(phi))),
      model.vertex_coords.emplace_back(radius * float(std::cos(phi)));
    }
    // add outer vertices
    for (int j = 0; j < model.VERTEX_PER_CIRCLE; j++) {
      auto phi = 2.f * M_PI * j / float(model.VERTEX_PER_CIRCLE);
      model.vertex_coords.emplace_back(dx * float(i));
      model.vertex_coords.emplace_back((radius + thickness) * float(std::sin(phi))),
      model.vertex_coords.emplace_back((radius + thickness) * float(std::cos(phi)));
    }
  }
  model.vertex_count += model.VERTEX_PER_CIRCLE * 2 * segment_count;

  // register normals
  {
//    // input edge
//    model.normals.emplace_back(-1, 0, 0);
//    // output edge
//    model.normals.emplace_back(1, 0, 0);
//    // body
//    for (int inout = 0; inout <= 1; inout++) {
//      for (int i = 0; i < model.VERTEX_PER_CIRCLE; i++) {
//        auto sign = 2.f * float(inout) - 1.f;
//        auto phi = 2.f * M_PI * i / float(model.VERTEX_PER_CIRCLE);
//        model.normals.emplace_back(0, sign * std::sin(phi), sign * std::cos(phi));
//      }
//    }
  }

  // register the planes
  for (int i = 0; i < model.VERTEX_PER_CIRCLE; i++) {
    // input edge
    auto next_i = (i + 1) % model.VERTEX_PER_CIRCLE;
    auto input_offset = int(start_x / dx) + 1;
    model.face_indices.emplace_back(model.get_vert_id(input_offset, i, false));
    model.face_indices.emplace_back(model.get_vert_id(input_offset, i, true));
    model.face_indices.emplace_back(model.get_vert_id(input_offset, next_i, true));
    model.face_indices.emplace_back(model.get_vert_id(input_offset, i, false));
    model.face_indices.emplace_back(model.get_vert_id(input_offset, next_i, true));
    model.face_indices.emplace_back(model.get_vert_id(input_offset, next_i, false));
    // output edge
    model.face_indices.emplace_back(model.get_vert_id(segment_count - 1, i, false));
    model.face_indices.emplace_back(model.get_vert_id(segment_count - 1, i, true));
    model.face_indices.emplace_back(model.get_vert_id(segment_count - 1, next_i, true));
    model.face_indices.emplace_back(model.get_vert_id(segment_count - 1, i, false));
    model.face_indices.emplace_back(model.get_vert_id(segment_count - 1, next_i, true));
    model.face_indices.emplace_back(model.get_vert_id(segment_count - 1, next_i, false));
  }
  model.face_count += model.VERTEX_PER_CIRCLE * 4;

  // body
  for (int i = 0; i < segment_count; i++) {
    if (dx * float(i) < start_x)
      continue;
    for (int j = 0; j < model.VERTEX_PER_CIRCLE; j++) {
      auto next_j = (j + 1) % model.VERTEX_PER_CIRCLE;

      int id_set[2][6] = {
        { i, j, i + 1, j, i + 1, next_j },
        { i, j, i + 1, next_j, i, next_j }
      };

      // loop on the 2 triangles which makes the body's square
      for (int k = 0; k < 2; k++) {
        // inner and outer planes
        for (int in_out = 0; in_out <= 1; in_out++) {
          model.face_indices.emplace_back(model.get_vert_id(id_set[k][0], id_set[k][1], bool(in_out)));
          model.face_indices.emplace_back(model.get_vert_id(id_set[k][2], id_set[k][3], bool(in_out)));
          model.face_indices.emplace_back(model.get_vert_id(id_set[k][4], id_set[k][5], bool(in_out)));
        }
      }
    }
  }
  model.face_count += (segment_count - int(start_x / dx + 1)) * model.VERTEX_PER_CIRCLE * 4;

  model.face_sizes.resize(model.face_count, 3);

  model.write();
}

} // namespace hnll