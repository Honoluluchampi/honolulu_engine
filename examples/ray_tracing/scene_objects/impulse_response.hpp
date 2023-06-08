#pragma once

// hnll
#include <utils/common_alias.hpp>

// lib
#include <AL/al.h>

namespace hnll {

template <size_t SourceCount>
class impulse_response
{

  private:
    ;
};

// temp : only support source 2
std::vector<float> unpack_ir(
  const std::vector<float>& raw_data,
  float ir_sampling_rate,
  float ir_duration,
  float ref_rate,
  float sound_speed = 340.f)
{
  std::vector<float> ret(static_cast<size_t>(ir_duration * ir_sampling_rate), 0);

  auto ray_count = raw_data.size() / 4;
  assert(raw_data.size() % 4 == 0 && "invalid data size");
  for (int i = 0; i < ray_count; i ++) {
    // raw_data[i]     : reflection count;
    // raw_data[i + 1] : source id;
    // raw_data[i + 2] : distance;
    if (raw_data[i * 4 + 1] == 2) {
      float magnitude = std::pow(ref_rate, raw_data[i * 4]);
      float delay = raw_data[i * 4 + 2] / sound_speed;
      if (delay < ir_duration)
        ret[delay * ir_sampling_rate] += magnitude;
    }
  }
  return ret;
}

std::vector<ALshort> convolve(
  const std::vector<float>& ir_series,
  float ir_sampling_rate,
  float ir_duration,
  const std::vector<ALshort>& source_series,
  float source_sampling_rate,
  float source_duration,
  int source_start_point)
{
  auto sample_count = static_cast<int>(source_duration * source_sampling_rate);
  std::vector<ALshort> ret(sample_count, 0);

  auto ir_sample_count = static_cast<int>(ir_duration * source_sampling_rate);

  // distribute ir from each source sample
  for (int s = -ir_sample_count - 1; s < sample_count; s++) {
    // source id
    auto s_id = s + source_start_point;
    if (s_id < 0)
      continue;
    auto s_value = source_series[s_id];

    // filter id
    for (int f_id = 0; f_id < ir_sample_count; f_id++) {
      auto target_id = s + f_id;
      if (target_id < 0 || target_id >= sample_count)
        continue;
      size_t ir_id = static_cast<size_t>(float(f_id) / source_sampling_rate * ir_sampling_rate);
      ret[target_id] += ir_series[ir_id] * s_value;
    }
  }

  return ret;
}

} // namespace hnll