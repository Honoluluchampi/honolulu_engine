#pragma once

// hnll
#include <game/modules/graphics_engine.hpp>
#include <utils/common_alias.hpp>
#include <utils/rendering_utils.hpp>

namespace hnll {

namespace game {

class engine
{
  public:
    engine(const std::string& application_name = "honolulu engine", utils::rendering_type rendering_type = utils::rendering_type::VERTEX_SHADING);
    ~engine(){}

    void run();

  private:
    void update();
    void render();

    // variables
    u_ptr<graphics_engine> graphics_engine_;
};

}} // namespace hnll::game