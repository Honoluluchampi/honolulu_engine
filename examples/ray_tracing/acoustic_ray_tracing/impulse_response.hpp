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
  float sound_speed,
  int sample_x,
  int sample_y,
  vec3 ear_dir) // should be normalized
{
  std::vector<double> ret(segment_size, 0);

  std::vector<double> tmp;
  auto ray_count = raw_data.size() / 4;
  assert(raw_data.size() % 4 == 0 && "invalid data size");
  for (int i = 0; i < ray_count; i ++) {
    // raw_data[i]     : reflection count;
    // raw_data[i + 1] : source id;
    // raw_data[i + 2] : distance;
    if (raw_data[i * 4 + 1] == 2) {
      // calculate cos of ear_dir and ray_dir
      const float theta = float(i % sample_x) / float(sample_x) * 2.f * M_PI;
      const float phi   = float(int(i / sample_x)) / float(sample_y) * 2.f * M_PI;
      const vec3 rad_direction = vec3(cos(theta), sin(theta) * cos(phi), sin(theta) * sin(phi));

      tmp.emplace_back(ear_dir.dot(rad_direction));
      float magnitude = (ear_dir.dot(rad_direction) + 1.2f) / 3.f;
      magnitude *= std::pow(ref_rate, raw_data[i * 4]);
      float delay = raw_data[i * 4 + 2] / sound_speed;
      if (delay < segment_size / ir_sampling_rate)
        ret[delay * ir_sampling_rate] += magnitude;
    }
  }
  return ret;
}

} // namespace hnll