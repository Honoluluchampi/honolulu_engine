#pragma once

#include <utils/common_alias.hpp>
#include <AL/al.h>

namespace hnll::audio::utils {

std::vector<ALshort> create_sine_wave(
  float duration,
  float pitch,
  float amplitude = 1.f, // 1 is max
  float sampling_rate = 44100.f
);

} // namespace hnll::audio::utils