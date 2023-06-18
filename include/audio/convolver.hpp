#pragma once

// hnll
#include <audio/audio_data.hpp>
#include <utils/common_alias.hpp>

namespace hnll::audio {

// overlap-add based convolver
class convolver
{
  public :
    // data_size must be 2^k
    static u_ptr<convolver> create(size_t data_size);

    explicit convolver(size_t data_size);

    void add_segment(std::vector<ALshort>&& data, std::vector<double>&& filter);

    std::vector<ALshort> move_buffer()
    {
      frame_index = !frame_index;
      return std::move(buffers_[!frame_index]);
    }

  private :
    int frame_index = 0;
    size_t data_size_;
    std::array<std::vector<ALshort>, 2> buffers_;
};

} // namespace hnll::audio