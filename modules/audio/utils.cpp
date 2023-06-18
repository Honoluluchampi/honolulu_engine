// hnll
#include <audio/utils.hpp>

// std
#include <complex>

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

void fft_rec(
  std::vector<std::complex<double>>& data,
  int stride,
  int first_index,
  int bit,
  int n)
{
  if (n > 1) {
    assert(n % 2 == 0 && "fft : input size must be equal to 2^k.");
    const int h = n / 2;
    const double theta = 2.f * M_PI / n;

    for (int i = 0; i < h; i++) {
      const std::complex<double> wi = { std::cos(i * theta), -sin(i * theta) };
      const std::complex<double> a = data[first_index + i + 0];
      const std::complex<double> b = data[first_index + i + h];

      data[first_index + i + 0] = a + b;
      data[first_index + i + h] = (a - b) * wi;
    }

    fft_rec(data, 2 * stride, first_index + 0, bit + 0,      h);
    fft_rec(data, 2 * stride, first_index + h, bit + stride, h);
  }
  // bit reverse
  else if (first_index > bit) {
    std::swap(data[first_index], data[bit]);
  }
}

std::vector<std::complex<double>> fft(std::vector<std::complex<double>>& time_series)
{
  int n = time_series.size();

  fft_rec(time_series, 1, 0, 0, n);

  return time_series;
}

std::vector<std::complex<double>> ifft(std::vector<std::complex<double>>& freq_series)
{
  int n = freq_series.size();

  for (int i = 0; i < n; i++) {
    freq_series[i] = conj(freq_series[i]);
  }

  fft_rec(freq_series, 1, 0, 0, n);

  for (int i = 0; i < n; i++) {
    freq_series[i] = conj(freq_series[i]) / static_cast<double>(n);
  }

  return freq_series;
}

} // namespace hnll::audio::utils