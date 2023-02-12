// hnll
#include <game/engine.hpp>
#include <game/shading_systems/grid_shading_system.hpp>

// std
#include <iostream>

int main()
{
  hnll::game::engine<hnll::game::type_list<hnll::game::grid_shading_system>> engine;

  try { engine.run(); }
  catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
}