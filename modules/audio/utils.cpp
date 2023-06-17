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
  std::vector<std::complex<double>>& buffer,
  int first_index,
  int n,
  std::complex<double> w)
{
  if (n > 1) {
    assert(n % 2 == 0);
    int h = n / 2;
    std::complex<double> wi { 1.f, 0.f };

    for (int i = first_index; i < h; i++) {
      buffer[i] = data[i] + data[i + h];
      buffer[i + h] = wi * (data[i] - data[i + h]);
      wi *= w;
    }

    fft_rec(buffer, data, 0, h, w * w);
    fft_rec(buffer, data, h, h, w * w);

    for (int i = first_index; i < h; i++) {
      data[2 * i] = buffer[i];
      data[2 * i + 1] = buffer[i + h];
    }
  }
}

std::vector<std::complex<double>> fft(std::vector<std::complex<double>>& time_series)
{
  int n = time_series.size();
  std::complex<double> w = std::exp(std::complex<double>{0.f, -2.f * M_PI} / static_cast<double>(n));
  std::vector<std::complex<double>> buffer(n, 0);
  std::vector<std::complex<double>> ans = time_series;

  fft_rec(ans, buffer, 0, n, w);
  for (int i = 0; i < n; i++) {
    ans[i] / static_cast<double>(n);
  }

  return ans;
}

std::vector<std::complex<double>> ifft(std::vector<std::complex<double>>& freq_series)
{
  int n = freq_series.size();
  std::complex<double> w = std::exp(std::complex<double>{0.f, 2.f * M_PI} / static_cast<double>(n));
  std::vector<std::complex<double>> buffer(n, 0);
  std::vector<std::complex<double>> ans = freq_series;

  fft_rec(ans, buffer, 0, n, w);
  for (int i = 0; i < n; i++) {
    ans[i] / static_cast<double>(n * n);
  }

  return ans;
}

} // namespace hnll::audio::utils