#pragma once

#include <cmath>

namespace hnll {

class sine_oscillator
{
  public:
    inline void set_freq(float freq) { freq_ = freq; }

    float tick(float dt)
    {
      phase_ += 6.28318530718 * freq_ * dt;
      phase_ = std::fmod(phase_, 6.28318530718);
      return std::sin(phase_);
    }

  private:
    float freq_ = 0.f;
    float phase_ = 0.f;
};

} // namespace hnll