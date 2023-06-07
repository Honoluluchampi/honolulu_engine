#pragma once

// hnll
#include <utils/common_alias.hpp>

namespace hnll {

// temp : only support source 2
std::vector<float> unpack_ir(
  const std::vector<float>& raw_data,
  float ir_sampling_rate,
  float ir_duration,
  float ref_rate,
  float sound_speed = 340.f)
{
  std::vector<float> ret(static_cast<size_t>(ir_duration * ir_sampling_rate), 0);

  assert(raw_data.size() % 4 == 0 && "invalid data size");
  for (int i = 0; i < raw_data.size() % 4; i += 4) {
    // raw_data[i]     : reflection count;
    // raw_data[i + 1] : source id;
    // raw_data[i + 2] : distance;
    if (raw_data[i + 1] == 2) {
      float magnitude = std::pow(ref_rate, raw_data[i]);
      float delay = raw_data[i + 2] / sound_speed;
      if (delay < ir_duration)
        ret[delay * ir_sampling_rate] += magnitude;
    }
  }

  return ret;
}

} // namespace hnll