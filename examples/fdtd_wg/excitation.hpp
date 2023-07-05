#pragma once

#include "fdtd.hpp"

namespace hnll {

// excitation cells are assumed to be located at the left edge.
void single_reed2(fdtd2& bore);
void air_jet2(fdtd2& bore);
void lip2(fdtd2& bore);

} // namespace