// hnll
#include <game/engine.hpp>

// std
#include <iostream>

int main()
{
  hnll::game::engine engine;

  try { engine.run(); }
  catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
}