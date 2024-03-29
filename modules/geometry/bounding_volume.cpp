// hnll
#include <geometry/bounding_volume.hpp>

namespace hnll::geometry {

bounding_volume::bounding_volume(const vec3d &center_point, const vec3d& radius) :
  center_point_(center_point),
  radius_(radius),
  transform_(std::make_shared<utils::transform>()),
  type_(bv_type::AABB)
{}

bounding_volume::bounding_volume(const vec3d &center_point, const double radius) :
  center_point_(center_point),
  radius_(radius, 0.f, 0.f),
  transform_(std::make_shared<utils::transform>()),
  type_(bv_type::SPHERE)
{}

u_ptr<bounding_volume> bounding_volume::create_aabb(const std::vector<vec3d>& vertices)
{
  // TODO : compute convex-hull
  auto convex_hull = vertices;
  double minx = convex_hull[0].x(),
    maxx = convex_hull[0].x(),
    miny = convex_hull[0].y(),
    maxy = convex_hull[0].y(),
    minz = convex_hull[0].z(),
    maxz = convex_hull[0].z();
  for (int i = 1; i < convex_hull.size(); i++) {
    const auto& vertex = convex_hull[i];
    if (minx > vertex.x()) minx = vertex.x();
    if (maxx < vertex.x()) maxx = vertex.x();
    if (miny > vertex.y()) miny = vertex.y();
    if (maxy < vertex.y()) maxy = vertex.y();
    if (minz > vertex.z()) minz = vertex.z();
    if (maxz < vertex.z()) maxz = vertex.z();
  }
  vec3d center_point = {(maxx + minx) / 2, (maxy + miny) / 2, (maxz + minz) / 2};
  vec3d radius = {(maxx - minx) / 2, (maxy - miny) / 2, (maxz - minz) / 2};
  return std::make_unique<bounding_volume>(center_point, radius);
}

u_ptr<bounding_volume> bounding_volume::create_aabb(const vec3d &center, const vec3d &radius)
{ return std::make_unique<bounding_volume>(center, radius); }

u_ptr<bounding_volume> bounding_volume::create_sphere(const vec3d& center, double radius)
{ return std::make_unique<bounding_volume>(center, radius); }

u_ptr<bounding_volume> bounding_volume::create_empty_bv(bv_type type, const hnll::vec3d &initial_point)
{
  auto ret = std::make_unique<bounding_volume>(initial_point, 0.f);
  ret->type_ = type;
  return ret;
}

std::pair<int,int> most_separated_points_on_aabb(const std::vector<vec3d> &vertices)
{
  // represents min/max point's index of each axis
  int minx = 0, maxx = 0, miny = 0, maxy = 0, minz = 0, maxz = 0, min = 0, max = 0;
  int n_vertices = vertices.size();
  for (int i = 1; i < n_vertices; i ++) {
    if (vertices[i].x() < vertices[minx].x()) minx = i;
    if (vertices[i].x() > vertices[maxx].x()) maxx = i;
    if (vertices[i].y() < vertices[miny].y()) miny = i;
    if (vertices[i].y() > vertices[maxy].y()) maxy = i;
    if (vertices[i].z() < vertices[minz].z()) minz = i;
    if (vertices[i].z() > vertices[maxz].z()) maxz = i;
  }
  auto diffx = vertices[maxx] - vertices[minx],
    diffy = vertices[maxy] - vertices[miny],
    diffz = vertices[maxz] - vertices[minz];
  // compute the squared distances for each pairs
  float dist2x = diffx.dot(diffx),
    dist2y = diffy.dot(diffy),
    dist2z = diffz.dot(diffz);
  // pick the most distant pair
  min = minx, max = maxx;
  if (dist2y > dist2x && dist2y > dist2z)
    min = miny, max = maxy;
  if (dist2z > dist2x && dist2z > dist2y)
    min = minz, max = maxz;
  return {min, max};
}

u_ptr<bounding_volume> sphere_from_distant_points(const std::vector<vec3d> &vertices)
{
  auto separated_idx = most_separated_points_on_aabb(vertices);
  auto center_point = (vertices[separated_idx.first] + vertices[separated_idx.second]) * 0.5f;
  double radius = (vertices[separated_idx.first] - center_point).dot(vertices[separated_idx.first] - center_point);
  radius = std::sqrt(radius);
  return std::make_unique<bounding_volume>(center_point, radius);
}

void extend_sphere_to_point(bounding_volume& sphere, const vec3d& point)
{
  auto diff = point - sphere.get_world_center_point();
  auto dist2 = diff.dot(diff);
  if (dist2 > sphere.get_sphere_radius() * sphere.get_sphere_radius()) {
    auto dist = std::sqrt(dist2);
    auto new_radius = (sphere.get_sphere_radius() + dist) * 0.5f;
    auto k = (new_radius - sphere.get_sphere_radius()) / dist;
    sphere.set_sphere_radius(new_radius);
    sphere.set_center_point(sphere.get_world_center_point() + diff * k);
  }
}

// currently only support ritter's algorithm
u_ptr<bounding_volume> bounding_volume::create_sphere(const std::vector<vec3d>& vertices)
{
  auto sphere = sphere_from_distant_points(vertices);
  for (const auto& vertex : vertices)
    extend_sphere_to_point(*sphere, vertex);
  return sphere;
}

} // namespace hnll::physics