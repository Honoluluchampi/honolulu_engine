#pragma once

// hnll
#include <utils/common_alias.hpp>

// std
#include <complex>

// lib
#include <AL/al.h>

namespace hnll::audio::utils {

std::vector<ALshort> create_sine_wave(
  float duration,
  float pitch,
  float amplitude = 1.f, // 1 is max
  float sampling_rate = 44100.f,
  float* phase = nullptr
);

// input data size should be equal to 2^k
std::vector<std::complex<double>> fft(std::vector<std::complex<double>>& time_series);
std::vector<std::complex<double>> ifft(std::vector<std::complex<double>>& freq_series);

// convert real number to complex number
template <typename T>
std::vector<std::complex<double>> fft(const std::vector<T>& time_series)
{
  std::vector<std::complex<double>> comp_series;
  for (const auto& val : time_series) {
    comp_series.template emplace_back(val, 0.f);
  }

  return fft(comp_series);
}

} // namespace hnll::audio::utils