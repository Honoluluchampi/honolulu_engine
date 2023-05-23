#include <audio/utils.hpp>

namespace hnll::audio::utils {

std::vector<ALshort> create_sine_wave(
  float duration,
  float pitch,
  float amplitude,
  float sampling_rate,
  float* phase
)
{
  int data_count = static_cast<int>(sampling_rate * duration);

  std::vector<ALshort> data(data_count);

  assert(amplitude <= 1.f && "amplitude should be or less than 1");

  float initial_phase = 0;
  if (phase != nullptr) {
    initial_phase = *phase;
  }

  for (int i = 0; i < data.size(); i++) {
    data[i] = std::sin(initial_phase + pitch * M_PI * 2.f * i / sampling_rate)
              * amplitude * std::numeric_limits<ALshort>::max();
  }

  if (phase != nullptr) {
    *phase = std::fmod(initial_phase + pitch * M_PI * 2.f * data.size() / sampling_rate, 2 * M_PI);
  }

  return data;
}

} // namespace hnll::audio::utils