#pragma once

// hnll
#include <game/modules/graphics_engine.hpp>
#include <utils/common_alias.hpp>

namespace hnll {

namespace game {

class engine
{
  public:
    engine();
    ~engine(){}

    void run();

  private:
    void update();
    void render();

    // variables
    u_ptr<graphics_engine> graphics_engine_;
};

}} // namespace hnll::game