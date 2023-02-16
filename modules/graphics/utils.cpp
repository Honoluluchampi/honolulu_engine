// hnll
#include <graphics/utils.hpp>

// libs
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>

namespace hnll::graphics {

std::vector<VkVertexInputBindingDescription> vertex::get_binding_descriptions() {
  std::vector<VkVertexInputBindingDescription> binding_descriptions(1);
  // per-vertex data is packed together in one array, so the index of the
  // binding in the array is always 0
  binding_descriptions[0].binding = 0;
  // number of bytes from one entry to the next
  binding_descriptions[0].stride = sizeof(vertex);
  binding_descriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  return binding_descriptions;
}

std::vector<VkVertexInputAttributeDescription> vertex::get_attribute_descriptions() {
  // how to extract a vertex attribute from a chunk of vertex data
  std::vector<VkVertexInputAttributeDescription> attribute_descriptions{};

  // location, binding, format, offset
  attribute_descriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vertex, position)});
  attribute_descriptions.push_back({1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vertex, color)});
  attribute_descriptions.push_back({2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vertex, normal)});
  attribute_descriptions.push_back({3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(vertex, uv)});

  return attribute_descriptions;
}

void obj_loader::load_model(const std::string& filename)
{
  // loader
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;

  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str()))
    throw std::runtime_error(warn + err);

  vertices.clear();
  indices.clear();

  std::unordered_map<vertex, uint32_t> unique_vertices{};

  for (const auto &shape : shapes) {
    for (const auto &index : shape.mesh.indices) {
      vertex vertex{};
      // copy the vertex
      if (index.vertex_index >= 0) {
        vertex.position = {
          attrib.vertices[3 * index.vertex_index + 0],
          -attrib.vertices[3 * index.vertex_index + 1],
          -attrib.vertices[3 * index.vertex_index + 2]
        };
        // color support
        vertex.color = {
          attrib.colors[3 * index.vertex_index + 0],
          attrib.colors[3 * index.vertex_index + 1],
          attrib.colors[3 * index.vertex_index + 2]
        };
      }
      // copy the normal
      if (index.normal_index >= 0) {
        vertex.normal = {
          attrib.normals[3 * index.normal_index + 0],
          -attrib.normals[3 * index.normal_index + 1],
          -attrib.normals[3 * index.normal_index + 2]
        };
      }
      // copy the texture coordinate
      if (index.texcoord_index >= 0) {
        vertex.uv = {
          attrib.vertices[2 * index.texcoord_index + 0],
          attrib.vertices[2 * index.texcoord_index + 1]
        };
      }
      // if vertex is a new vertex
      if (unique_vertices.find(vertex) == unique_vertices.end()) {
        unique_vertices[vertex] = static_cast<uint32_t>(vertices.size());
        vertices.push_back(vertex);
      }
      indices.push_back(unique_vertices[vertex]);
    }
  }
}

} // namespace hnll::graphics