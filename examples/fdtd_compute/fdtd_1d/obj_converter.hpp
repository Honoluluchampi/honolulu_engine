#pragma once

#include <utils/common_alias.hpp>
#include <utils/utils.hpp>
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

void write_obj(
  const std::string& name,
  McContext& context,
  McConnectedComponent& connComp,
  const std::vector<double>& ccVertices,
  const std::vector<uint32_t>& ccFaceIndices)
{
  McPatchLocation patchLocation = (McPatchLocation)0;

  auto err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_PATCH_LOCATION, sizeof(McPatchLocation), &patchLocation, NULL);
  assert(err == MC_NO_ERROR);

  McFragmentLocation fragmentLocation = (McFragmentLocation)0;
  err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_FRAGMENT_LOCATION, sizeof(McFragmentLocation), &fragmentLocation, NULL);
  assert(err == MC_NO_ERROR);

  // save cc mesh to .obj file
  std::string fpath(name);

  printf("write file: %s\n", fpath.c_str());

  std::ofstream file(fpath);

  // write vertices and normals
  for (uint32_t i = 0; i < ccVertices.size() / 3; ++i) {
    double x = ccVertices[(McSize)i * 3 + 0];
    double y = ccVertices[(McSize)i * 3 + 1];
    double z = ccVertices[(McSize)i * 3 + 2];
    file << "v " << std::setprecision(std::numeric_limits<long double>::digits10 + 1) << x << " " << y << " " << z << std::endl;
  }

  int faceVertexOffsetBase = 0;

  // for each face in CC
  for (uint32_t f = 0; f < ccFaceIndices.size() / 3; ++f) {
    bool reverseWindingOrder = (fragmentLocation == MC_FRAGMENT_LOCATION_BELOW) && (patchLocation == MC_PATCH_LOCATION_OUTSIDE);
    int faceSize = 3;
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
}

obj_model create_instrument(
  float dx,
  float thickness,
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

  // register the planes
  for (int i = 0; i < VERTEX_PER_CIRCLE; i++) {
    // input edge
    auto next_i = (i + 1) % VERTEX_PER_CIRCLE;
    auto input_offset = 0;
    model.face_indices.emplace_back(model.get_vert_id(input_offset, i, false));
    model.face_indices.emplace_back(model.get_vert_id(input_offset, i, true));
    model.face_indices.emplace_back(model.get_vert_id(input_offset, next_i, true));
    model.face_indices.emplace_back(model.get_vert_id(input_offset, i, false));
    model.face_indices.emplace_back(model.get_vert_id(input_offset, next_i, true));
    model.face_indices.emplace_back(model.get_vert_id(input_offset, next_i, false));

    // output edge
    model.face_indices.emplace_back(model.get_vert_id(segment_count - 1, i, false));
    model.face_indices.emplace_back(model.get_vert_id(segment_count - 1, next_i, true));
    model.face_indices.emplace_back(model.get_vert_id(segment_count - 1, i, true));
    model.face_indices.emplace_back(model.get_vert_id(segment_count - 1, i, false));
    model.face_indices.emplace_back(model.get_vert_id(segment_count - 1, next_i, false));
    model.face_indices.emplace_back(model.get_vert_id(segment_count - 1, next_i, true));
  }

  // body
  for (int i = 0; i < segment_count - 1; i++) {
    for (int j = 0; j < VERTEX_PER_CIRCLE; j++) {
      auto next_j = (j + 1) % VERTEX_PER_CIRCLE;

      int id_set[2][6] = {
        { i, j, i + 1, j, i + 1, next_j },
        { i, j, i + 1, next_j, i, next_j }
      };

      // loop on the 2 triangles which makes the body's square
      for (int k = 0; k < 2; k++) {
        // inner and outer planes
        model.face_indices.emplace_back(model.get_vert_id(id_set[k][0], id_set[k][1], true));
        model.face_indices.emplace_back(model.get_vert_id(id_set[k][2], id_set[k][3], true));
        model.face_indices.emplace_back(model.get_vert_id(id_set[k][4], id_set[k][5], true));
        model.face_indices.emplace_back(model.get_vert_id(id_set[k][0], id_set[k][1], false));
        model.face_indices.emplace_back(model.get_vert_id(id_set[k][4], id_set[k][5], false));
        model.face_indices.emplace_back(model.get_vert_id(id_set[k][2], id_set[k][3], false));
      }
    }
  }

  // stats
  assert(model.vertex_coords.size() % 3 == 0);
  model.vertex_count = model.vertex_coords.size() / 3;
  assert(model.face_indices.size() % 3 == 0);
  model.face_count = model.face_indices.size() / 3;
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
  for (int j = 0; j < VERTEX_PER_CIRCLE; j++) {
    // top circle
    model.face_indices.emplace_back(id_offset[0]);
    model.face_indices.emplace_back(id_offset[0] + 1 + ((j + 1) % VERTEX_PER_CIRCLE));
    model.face_indices.emplace_back(id_offset[0] + 1 + (j % VERTEX_PER_CIRCLE));
    // bottom circle
    model.face_indices.emplace_back(id_offset[1]);
    model.face_indices.emplace_back(id_offset[1] + 1 + (j % VERTEX_PER_CIRCLE));
    model.face_indices.emplace_back(id_offset[1] + 1 + ((j + 1) % VERTEX_PER_CIRCLE));
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
  model.face_count = model.face_indices.size() / 3;
  model.face_sizes.resize(model.face_count, 3);

  return model;
}

obj_model create_cube(float scale, float x_offset)
{
  obj_model cube;
  std::vector<vec3d> vertices = {
    { -1.f, -1.f, -1.f },
    {  1.f, -1.f, -1.f },
    {  1.f,  1.f, -1.f },
    { -1.f,  1.f, -1.f },
    { -1.f, -1.f,  1.f },
    {  1.f, -1.f,  1.f },
    {  1.f,  1.f,  1.f },
    { -1.f,  1.f,  1.f },
  };

  std::vector<ivec3> faces = {
    { 0, 3, 1 },
    { 1, 3, 2 },
    { 0, 1, 4 },
    { 1, 5, 4 },
    { 1, 2, 5 },
    { 2, 6, 5 },
    { 2, 3, 6 },
    { 3, 7, 6 },
    { 3, 0, 7 },
    { 0, 4, 7 },
    { 4, 5, 7 },
    { 5, 6, 7 },
  };

  for (const auto& v : vertices) {
    cube.vertex_coords.emplace_back(x_offset + (v.x() * scale));
    cube.vertex_coords.emplace_back(v.y() * scale);
    cube.vertex_coords.emplace_back(v.z() * scale);
  }

  for (const auto& f : faces) {
    cube.face_indices.emplace_back(f.x());
    cube.face_indices.emplace_back(f.y());
    cube.face_indices.emplace_back(f.z());
  }

  cube.vertex_count = vertices.size();
  cube.face_count = faces.size();
  cube.face_sizes.resize(cube.face_count, 3);

  return cube;
}

// TODO adjust a mouthpiece
// takes the list of the offsets of each bore segment.
void convert_to_obj(
  const std::string& dir_name,
  const std::string& file_name,
  float dx,
  float thickness,
  float start_x,
  const std::vector<float>& offsets,
  const std::vector<int>& hole_ids,
  float hole_radius)
{
  auto bore = create_instrument(dx, thickness, offsets);

  // geometry
  std::vector<double> ccVertices = std::move(bore.vertex_coords);
  std::vector<uint32_t> ccFaceIndices = std::move(bore.face_indices);

  {
    // mc init
    McContext context = MC_NULL_HANDLE;
    McResult err = mcCreateContext(&context, MC_DEBUG);
    assert(err == MC_NO_ERROR);
    McConnectedComponent connComp;
    std::vector<McConnectedComponent> CCs;
    McFlags bool_op = MC_DISPATCH_FILTER_FRAGMENT_SEALING_INSIDE | MC_DISPATCH_FILTER_FRAGMENT_LOCATION_BELOW;

    for (int i = 0; i < hole_ids.size(); i++) {
      auto cylinder = create_cylinder(hole_radius, hole_ids[i] * dx);

      err = mcDispatch(
        context,
        MC_DISPATCH_VERTEX_ARRAY_DOUBLE | MC_DISPATCH_ENFORCE_GENERAL_POSITION | bool_op,
        // source
        reinterpret_cast<const void *>(ccVertices.data()),
        reinterpret_cast<const uint32_t *>(ccFaceIndices.data()),
        std::vector<uint32_t>(ccFaceIndices.size() / 3, 3).data(),
        static_cast<uint32_t>(ccVertices.size() / 3),
        static_cast<uint32_t>(ccFaceIndices.size() / 3),
        // cut mesh
        reinterpret_cast<const void *>(cylinder.vertex_coords.data()),
        reinterpret_cast<const uint32_t *>(cylinder.face_indices.data()),
        cylinder.face_sizes.data(),
        static_cast<uint32_t>(cylinder.vertex_count),
        static_cast<uint32_t>(cylinder.face_sizes.size())
      );
      assert(err == MC_NO_ERROR);

      // the number of available connected component should be 1
      uint32_t numCC; // connected component
      err = mcGetConnectedComponents(context, MC_CONNECTED_COMPONENT_TYPE_FRAGMENT, 0, NULL, &numCC);
      assert(err == MC_NO_ERROR);

      CCs.resize(numCC, MC_NULL_HANDLE);

      err = mcGetConnectedComponents(context, MC_CONNECTED_COMPONENT_TYPE_FRAGMENT, (uint32_t) CCs.size(), CCs.data(), NULL);
      assert(err == MC_NO_ERROR);

      // query the data of each connected component from MCUT
      connComp = CCs[0];

      // query the vertices
      McSize numBytes = 0;
      err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_VERTEX_DOUBLE, 0, NULL, &numBytes);
      assert(err == MC_NO_ERROR);
      uint32_t ccVertexCount = (uint32_t) (numBytes / (sizeof(double) * 3));
      ccVertices.resize((McSize)
      ccVertexCount * 3u, 0);
      err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_VERTEX_DOUBLE, numBytes,
                                        (void *) ccVertices.data(), NULL);
      assert(err == MC_NO_ERROR);

      // query the faces
      numBytes = 0;
      err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_FACE_TRIANGULATION, 0, NULL,
                                        &numBytes);
      assert(err == MC_NO_ERROR);
      ccFaceIndices.resize(numBytes / sizeof(uint32_t), 0);
      err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_FACE_TRIANGULATION, numBytes,
                                        ccFaceIndices.data(), NULL);
      assert(err == MC_NO_ERROR);
    }

    auto fixed_dir = dir_name + "/" + file_name;
    utils::mkdir_p(dir_name);
    utils::mkdir_p(fixed_dir);
    write_obj(fixed_dir + "/whole.obj", context, connComp, ccVertices, ccFaceIndices);

    err = mcReleaseConnectedComponents(context, (uint32_t) CCs.size(), CCs.data());
    assert(err == MC_NO_ERROR);

    err = mcReleaseContext(context);
    assert(err == MC_NO_ERROR);
  }

  // -------------------------------------------------------------------------------
  // bore separation
  {
    McFlags bool_op = MC_DISPATCH_FILTER_FRAGMENT_SEALING_INSIDE | MC_DISPATCH_FILTER_FRAGMENT_LOCATION_ABOVE;

    float offset[4] = {
      start_x,
      float(hole_ids[4] + hole_ids[5]) / 2.f * dx,
      float(hole_ids[2] + hole_ids[1]) / 2.f * dx,
      float(offsets.size()) * dx,
    };

    for (int i = 0; i < 3; i++) {
      // init
      McContext context = MC_NULL_HANDLE;
      McResult err = mcCreateContext(&context, MC_DEBUG);
      assert(err == MC_NO_ERROR);
      McConnectedComponent connComp;
      std::vector<McConnectedComponent> CCs;
      std::vector<double> partVertices;
      std::vector<uint32_t> partFaceIndices;

      for (int j = 0; j < 2; j++) {
        auto cube = create_cube(40.f, offset[j + i] * MODEL_SCALE + float(j * 2 - 1) * 40.f);

        if (j == 0)
          err = mcDispatch(
            context,
            MC_DISPATCH_VERTEX_ARRAY_DOUBLE | MC_DISPATCH_ENFORCE_GENERAL_POSITION | bool_op,
            // source
            reinterpret_cast<const void *>(ccVertices.data()),
            reinterpret_cast<const uint32_t *>(ccFaceIndices.data()),
            std::vector<uint32_t>(ccFaceIndices.size() / 3, 3).data(),
            static_cast<uint32_t>(ccVertices.size() / 3),
            static_cast<uint32_t>(ccFaceIndices.size() / 3),
            // cut mesh
            reinterpret_cast<const void *>(cube.vertex_coords.data()),
            reinterpret_cast<const uint32_t *>(cube.face_indices.data()),
            cube.face_sizes.data(),
            static_cast<uint32_t>(cube.vertex_count),
            static_cast<uint32_t>(cube.face_count)
          );
        else
          err = mcDispatch(
            context,
            MC_DISPATCH_VERTEX_ARRAY_DOUBLE | MC_DISPATCH_ENFORCE_GENERAL_POSITION | bool_op,
            // source
            reinterpret_cast<const void *>(partVertices.data()),
            reinterpret_cast<const uint32_t *>(partFaceIndices.data()),
            std::vector<uint32_t>(partFaceIndices.size() / 3, 3).data(),
            static_cast<uint32_t>(partVertices.size() / 3),
            static_cast<uint32_t>(partFaceIndices.size() / 3),
            // cut mesh
            reinterpret_cast<const void *>(cube.vertex_coords.data()),
            reinterpret_cast<const uint32_t *>(cube.face_indices.data()),
            cube.face_sizes.data(),
            static_cast<uint32_t>(cube.vertex_count),
            static_cast<uint32_t>(cube.face_count)
          );
        assert(err == MC_NO_ERROR);

        // the number of available connected component should be 1
        uint32_t numCC; // connected component
        err = mcGetConnectedComponents(context, MC_CONNECTED_COMPONENT_TYPE_FRAGMENT, 0, NULL, &numCC);
        assert(err == MC_NO_ERROR);
        assert(numCC >= 1);

        CCs.resize(numCC, MC_NULL_HANDLE);

        err = mcGetConnectedComponents(context, MC_CONNECTED_COMPONENT_TYPE_FRAGMENT, (uint32_t) CCs.size(), CCs.data(),
                                       NULL);
        assert(err == MC_NO_ERROR);

        // query the data of each connected component from MCUT
        connComp = CCs[0];

        // query the vertices
        McSize numBytes = 0;
        err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_VERTEX_DOUBLE, 0, NULL,
                                          &numBytes);
        assert(err == MC_NO_ERROR);
        uint32_t ccVertexCount = (uint32_t) (numBytes / (sizeof(double) * 3));
        partVertices.resize((McSize)ccVertexCount * 3u, 0);
        err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_VERTEX_DOUBLE, numBytes, (void *)partVertices.data(), NULL);
        assert(err == MC_NO_ERROR);

        // query the faces
        numBytes = 0;
        err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_FACE_TRIANGULATION, 0, NULL, &numBytes);
        assert(err == MC_NO_ERROR);
        partFaceIndices.resize(numBytes / sizeof(uint32_t), 0);
        err = mcGetConnectedComponentData(context, connComp, MC_CONNECTED_COMPONENT_DATA_FACE_TRIANGULATION, numBytes, partFaceIndices.data(), NULL);
        assert(err == MC_NO_ERROR);
      }

      auto fixed_dir = dir_name + "/" + file_name;
      utils::mkdir_p(dir_name);
      utils::mkdir_p(fixed_dir);
      write_obj(fixed_dir + "/part" + std::to_string(i) + ".obj", context, connComp, partVertices, partFaceIndices);

      err = mcReleaseConnectedComponents(context, (uint32_t) CCs.size(), CCs.data());
      assert(err == MC_NO_ERROR);

      err = mcReleaseContext(context);
      assert(err == MC_NO_ERROR);
    }
  }
}

} // namespace hnll