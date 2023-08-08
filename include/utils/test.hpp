#pragma once

// hnll
#include <graphics/window.hpp>
#include <graphics/device.hpp>
#include <utils/singleton.hpp>
#include <utils/rendering_utils.hpp>

// lib
#include <gtest/gtest.h>

namespace hnll {

// setup singleton objects
void setup_singleton()
{
  auto &window = utils::singleton<graphics::window>::get_instance(100, 100, "test");
  auto &device = utils::singleton<graphics::device>::get_instance(utils::rendering_type::VERTEX_SHADING);
}

} // namespace hnll