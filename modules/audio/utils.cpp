#include <audio/utils.hpp>

namespace hnll::audio::utils {

std::vector<ALshort> create_sine_wave(
  float duration,
  float pitch,
  float amplitude,
  float sampling_rate
)
{
  std::vector<ALshort> data(static_cast<size_t>(sampling_rate * duration));

  assert(amplitude <= 1.f && "amplitude should be or less than 1");

  for (int i = 0; i < data.size(); i++) {
    data[i] = std::sin(pitch * M_PI * 2.f * i / sampling_rate)
              * amplitude * std::numeric_limits<ALshort>::max();
  }

  return data;
}

} // namespace hnll::audio::utils