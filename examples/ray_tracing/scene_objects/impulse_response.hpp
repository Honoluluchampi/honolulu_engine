#pragma once

// hnll
#include <utils/common_alias.hpp>

// lib
#include <AL/al.h>

namespace hnll {

// temp : only support source 2
std::vector<double> unpack_ir(
  const std::vector<float>& raw_data,
  float ir_sampling_rate,
  int   segment_size,
  float ref_rate,
  float sound_speed = 340.f)
{
  std::vector<double> ret(segment_size, 0);

  auto ray_count = raw_data.size() / 4;
  assert(raw_data.size() % 4 == 0 && "invalid data size");
  for (int i = 0; i < ray_count; i ++) {
    // raw_data[i]     : reflection count;
    // raw_data[i + 1] : source id;
    // raw_data[i + 2] : distance;
    if (raw_data[i * 4 + 1] == 2) {
      float magnitude = std::pow(ref_rate, raw_data[i * 4]);
      float delay = raw_data[i * 4 + 2] / sound_speed;
      if (delay < segment_size / ir_sampling_rate)
        ret[delay * ir_sampling_rate] += magnitude;
    }
  }
  return ret;
}

} // namespace hnll