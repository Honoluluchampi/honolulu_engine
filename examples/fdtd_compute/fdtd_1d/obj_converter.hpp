#pragma once

#include <utils/common_alias.hpp>
#include <fstream>
#include <mcut/mcut.h>

namespace hnll {

constexpr double MODEL_SCALE = 100.f;
constexpr int    VERTEX_PER_CIRCLE = 32;

// compatible with mcut
struct obj_model
{
  std::vector<double> vertex_coords;
  std::vector<uint32_t> face_indices;
  std::vector<uint32_t> face_sizes;

  uint32_t vertex_count = 0;
  uint32_t face_count = 0;

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
        << vertex_coords[i * 3 + 0] << " "
        << vertex_coords[i * 3 + 1] << " "
        << vertex_coords[i * 3 + 2] << " "
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

obj_model create_instrument(
  float dx,
  float thickness,
  float start_x,
  const std::vector<float>& offsets)
{
  auto segment_count = offsets.size();

  obj_model model;

  // register the vertices
  for (int i = 0; i < segment_count; i++) {
    auto radius = offsets[i];
    // add inner vertices
    for (int j = 0; j < VERTEX_PER_CIRCLE; j++) {
      auto phi = 2.f * M_PI * j / float(VERTEX_PER_CIRCLE);
      model.vertex_coords.emplace_back(MODEL_SCALE * dx * float(i));
      model.vertex_coords.emplace_back(MODEL_SCALE * radius * float(std::sin(phi))),
      model.vertex_coords.emplace_back(MODEL_SCALE * radius * float(std::cos(phi)));
    }
    // add outer vertices
    for (int j = 0; j < VERTEX_PER_CIRCLE; j++) {
      auto phi = 2.f * M_PI * j / float(VERTEX_PER_CIRCLE);
      model.vertex_coords.emplace_back(MODEL_SCALE * dx * float(i));
      model.vertex_coords.emplace_back(MODEL_SCALE * (radius + thickness) * float(std::sin(phi))),
      model.vertex_coords.emplace_back(MODEL_SCALE * (radius + thickness) * float(std::cos(phi)));
    }
  }
  model.vertex_count += VERTEX_PER_CIRCLE * 2 * segment_count;

  // register the planes
  for (int i = 0; i < VERTEX_PER_CIRCLE; i++) {
    // input edge
    auto next_i = (i + 1) % VERTEX_PER_CIRCLE;
    auto input_offset = int(start_x / dx) + 1;
    model.face_indices.emplace_back(model.get_vert_id(segment_count - 1, i, false));
    model.face_indices.emplace_back(model.get_vert_id(segment_count - 1, next_i, true));
    model.face_indices.emplace_back(model.get_vert_id(segment_count - 1, i, true));
    model.face_indices.emplace_back(model.get_vert_id(segment_count - 1, i, false));
    model.face_indices.emplace_back(model.get_vert_id(segment_count - 1, next_i, false));
    model.face_indices.emplace_back(model.get_vert_id(segment_count - 1, next_i, true));

    // output edge
    model.face_indices.emplace_back(model.get_vert_id(input_offset, i, false));
    model.face_indices.emplace_back(model.get_vert_id(input_offset, i, true));
    model.face_indices.emplace_back(model.get_vert_id(input_offset, next_i, true));
    model.face_indices.emplace_back(model.get_vert_id(input_offset, i, false));
    model.face_indices.emplace_back(model.get_vert_id(input_offset, next_i, true));
    model.face_indices.emplace_back(model.get_vert_id(input_offset, next_i, false));
  }
  model.face_count += VERTEX_PER_CIRCLE * 4;

  // body
  for (int i = 0; i < segment_count; i++) {
    if (dx * float(i) < start_x)
      continue;
    for (int j = 0; j < VERTEX_PER_CIRCLE; j++) {
      auto next_j = (j + 1) % VERTEX_PER_CIRCLE;

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
  model.face_count += (segment_count - int(start_x / dx + 1)) * VERTEX_PER_CIRCLE * 4;

  model.face_sizes.resize(model.face_count, 3);

  return model;
}

obj_model create_cylinder(float hole_radius, float x_offset)
{
  obj_model model;

  // register vertices
  float z_offset[2] = { 0.f, 10.f };
  // bottom and top circle
  for (int i = 0; i < 2; i++) {
    model.vertex_coords.emplace_back(x_offset * MODEL_SCALE);
    model.vertex_coords.emplace_back(z_offset[i]);
    model.vertex_coords.emplace_back(0.f);
    for (int j = 0; j < VERTEX_PER_CIRCLE; j++) {
      auto phi = 2.f * M_PI * float(j) / float(VERTEX_PER_CIRCLE);
      model.vertex_coords.emplace_back(MODEL_SCALE * (x_offset + hole_radius * std::cos(phi)));
      model.vertex_coords.emplace_back(z_offset[i]);
      model.vertex_coords.emplace_back(MODEL_SCALE * (hole_radius * std::sin(phi)));
    }
  }

  // register circle faces
  uint32_t id_offset[2] = {0, VERTEX_PER_CIRCLE + 1};
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < VERTEX_PER_CIRCLE; j++) {
      model.face_indices.emplace_back(id_offset[i]);
      model.face_indices.emplace_back(id_offset[i] + 1 + (j % VERTEX_PER_CIRCLE));
      model.face_indices.emplace_back(id_offset[i] + 1 + ((j + 1) % VERTEX_PER_CIRCLE));
    }
  }

  // register wall faces
  for (int i = 0; i < VERTEX_PER_CIRCLE; i++) {
    model.face_indices.emplace_back(id_offset[0] + 1 + (i % VERTEX_PER_CIRCLE));
    model.face_indices.emplace_back(id_offset[0] + 1 + ((i + 1) % VERTEX_PER_CIRCLE));
    model.face_indices.emplace_back(id_offset[1] + 1 + ((i + 1) % VERTEX_PER_CIRCLE));
    model.face_indices.emplace_back(id_offset[0] + 1 + (i % VERTEX_PER_CIRCLE));
    model.face_indices.emplace_back(id_offset[1] + 1 + ((i + 1) % VERTEX_PER_CIRCLE));
    model.face_indices.emplace_back(id_offset[1] + 1 + (i % VERTEX_PER_CIRCLE));
  }

  model.vertex_count = VERTEX_PER_CIRCLE * 2 + 2;
  model.face_count = VERTEX_PER_CIRCLE * 4;
  model.face_sizes.resize(model.face_count, 3);

  return model;
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
  auto cylinder = create_instrument(dx, thickness, start_x, offsets);
  auto bore = create_cylinder(hole_radius, hole_ids[1] * dx);

  McContext context = MC_NULL_HANDLE;
  McResult err = mcCreateContext(&context, MC_DEBUG);
  assert(err == MC_NO_ERROR);

  // dispatch task
  McFlags bool_op = MC_DISPATCH_FILTER_FRAGMENT_SEALING_INSIDE | MC_DISPATCH_FILTER_FRAGMENT_LOCATION_ABOVE;
  err = mcDispatch(
    context,
    MC_DISPATCH_VERTEX_ARRAY_DOUBLE | MC_DISPATCH_ENFORCE_GENERAL_POSITION | bool_op,
    // source
    reinterpret_cast<const void*>(bore.vertex_coords.data()),
    reinterpret_cast<const uint32_t*>(bore.face_indices.data()),
    bore.face_sizes.data(),
    static_cast<uint32_t>(bore.vertex_coords.size() / 3),
    static_cast<uint32_t>(bore.face_sizes.size()),
    // cut mesh
    reinterpret_cast<const void*>(cylinder.vertex_coords.data()),
    reinterpret_cast<const uint32_t*>(cylinder.face_indices.data()),
    cylinder.face_sizes.data(),
    static_cast<uint32_t>(cylinder.vertex_count),
    static_cast<uint32_t>(cylinder.face_sizes.size())
  );
  assert(err == MC_NO_ERROR);

  // the number of available connected component should be 1
  uint32_t numCC; // connected component
  err = mcGetConnectedComponents(context, MC_CONNECTED_COMPONENT_TYPE_FRAGMENT, 0, NULL, &numCC);
  assert(err == MC_NO_ERROR);
  assert(numCC == 1);

  std::vector<McConnectedComponent> CCs(numCC, MC_NULL_HANDLE);
  CCs.resize(numCC);
  err = mcGetConnectedComponents(context, MC_CONNECTED_COMPONENT_TYPE_FRAGMENT, (uint32_t)CCs.size(), CCs.data(), NULL);
  assert(err == MC_NO_ERROR);

  // query the data of each connected component from MCUT
  McConnectedComponent connComp = CCs[0];

  // query the vertices
  McSize numBytes = 0;
  err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_VERTEX_DOUBLE, 0, NULL, &numBytes);
  assert(err == MC_NO_ERROR);
  uint32_t ccVertexCount = (uint32_t)(numBytes / (sizeof(double) * 3));
  std::vector<double> ccVertices((McSize)ccVertexCount * 3u, 0);
  err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_VERTEX_DOUBLE, numBytes, (void*)ccVertices.data(), NULL);
  assert(err == MC_NO_ERROR);

  // query the faces
  numBytes = 0;

  err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_FACE_TRIANGULATION, 0, NULL, &numBytes);
  assert(err == MC_NO_ERROR);
  std::vector<uint32_t> ccFaceIndices(numBytes / sizeof(uint32_t), 0);
  err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_FACE_TRIANGULATION, numBytes, ccFaceIndices.data(), NULL);
  assert(err == MC_NO_ERROR);

  std::vector<uint32_t> faceSizes(ccFaceIndices.size() / 3, 3);
  const uint32_t ccFaceCount = static_cast<uint32_t>(faceSizes.size());

  /// ------------------------------------------------------------------------------------

  // Here we show, how to know when connected components, pertain particular boolean operations.

  McPatchLocation patchLocation = (McPatchLocation)0;

  err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_PATCH_LOCATION, sizeof(McPatchLocation), &patchLocation, NULL);
  assert(err == MC_NO_ERROR);

  McFragmentLocation fragmentLocation = (McFragmentLocation)0;
  err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_FRAGMENT_LOCATION, sizeof(McFragmentLocation), &fragmentLocation, NULL);
  assert(err == MC_NO_ERROR);

  // save cc mesh to .obj file
  // -------------------------

  auto extract_fname = [](const std::string& full_path) {
    // get filename
    std::string base_filename = full_path.substr(full_path.find_last_of("/\\") + 1);
    // remove extension from filename
    std::string::size_type const p(base_filename.find_last_of('.'));
    std::string file_without_extension = base_filename.substr(0, p);
    return file_without_extension;
  };

  std::string fpath(name);

  printf("write file: %s\n", fpath.c_str());

  std::ofstream file(fpath);

  // write vertices and normals
  for (uint32_t i = 0; i < ccVertexCount; ++i) {
    double x = ccVertices[(McSize)i * 3 + 0];
    double y = ccVertices[(McSize)i * 3 + 1];
    double z = ccVertices[(McSize)i * 3 + 2];
    file << "v " << std::setprecision(std::numeric_limits<long double>::digits10 + 1) << x << " " << y << " " << z << std::endl;
  }

  int faceVertexOffsetBase = 0;

  // for each face in CC
  for (uint32_t f = 0; f < ccFaceCount; ++f) {
    bool reverseWindingOrder = (fragmentLocation == MC_FRAGMENT_LOCATION_BELOW) && (patchLocation == MC_PATCH_LOCATION_OUTSIDE);
    int faceSize = faceSizes.at(f);
    file << "f ";
    // for each vertex in face
    for (int v = (reverseWindingOrder ? (faceSize - 1) : 0);
         (reverseWindingOrder ? (v >= 0) : (v < faceSize));
         v += (reverseWindingOrder ? -1 : 1)) {
      const int ccVertexIdx = ccFaceIndices[(McSize)faceVertexOffsetBase + v];
      file << (ccVertexIdx + 1) << " ";
    } // for (int v = 0; v < faceSize; ++v) {
    file << std::endl;

    faceVertexOffsetBase += faceSize;
  }

  // 6. free connected component data
  // --------------------------------
  err = mcReleaseConnectedComponents(context, (uint32_t)CCs.size(), CCs.data());
  assert(err == MC_NO_ERROR);

  // 7. destroy context
  // ------------------
  err = mcReleaseContext(context);

  assert(err == MC_NO_ERROR);
}

} // namespace hnll