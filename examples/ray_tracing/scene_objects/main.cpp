// hnll
#include <game/engine.hpp>
#include <game/actor.hpp>
#include <game/ray_tracing_system.hpp>
#include <graphics/acceleration_structure.hpp>
#include <graphics/graphics_models/static_mesh.hpp>
#include <graphics/utils.hpp>
#include <utils/rendering_utils.hpp>
#include <utils/utils.hpp>

// std
#include <iostream>
#include <algorithm>
#include <filesystem>

namespace hnll {

const std::string SHADERS_DIRECTORY =
  std::string(std::getenv("HNLL_ENGN")) + "/examples/ray_tracing/model/shaders/spv/";

#define MODEL_NAME utils::get_full_path("bunny.obj")


// shading system
DEFINE_RAY_TRACER(model_ray_tracer, utils::shading_type::RAY1)
{
  public:
    model_ray_tracer(graphics::device& device) : game::ray_tracing_system<model_ray_tracer, utils::shading_type::RAY1>(device) {}

  private:
};

DEFINE_PURE_ACTOR(object)
{

};

SELECT_SHADING_SYSTEM(model_ray_tracer);
SELECT_COMPUTE_SHADER();
SELECT_ACTOR(object);

DEFINE_ENGINE(hello_model) {

};
}

int main() {
  hnll::hello_model app {};

  try { app.run(); }
  catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}