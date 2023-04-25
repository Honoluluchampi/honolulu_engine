#pragma once

// std
#include <vector>

// openAL
#include <AL/al.h>
#include <AL/alc.h>

namespace hnll::audio {

using buffer_id = ALuint;
using source_id = ALuint;

class audio_data
{
  public:
    explicit audio_data(ALenum format = AL_FORMAT_MONO16, ALsizei sampling_rate = 44100)
      : format_(format), sampling_rate_(sampling_rate) {}

    // setter (enable chain notation)
    // complete transport std::vector<ALshort>
    template<typename T>
    audio_data &set_data(T &&data)              { data_ = std::forward<T>(data); return *this; }
    audio_data &set_format(const ALenum format) { format_ = format; return *this; }
    audio_data &set_sampling_rate(const ALsizei sampling_rate) { sampling_rate_ = sampling_rate; return *this; }
    void set_buffer_id(buffer_id id)        { buffer_id_ = id; }
    void set_source_id(source_id id)        { source_id_ = id; }
    void set_is_bound_to_buffer(bool state) { is_bound_to_buffer_ = state; }
    void set_is_bound_to_source(bool state) { is_bound_to_source_ = state; }

    // getter
    [[nodiscard]] const ALshort *get_data()       const { return &data_[0]; }
    [[nodiscard]] buffer_id get_buffer_id()       const { return buffer_id_; }
    [[nodiscard]] bool is_bound_to_buffer()       const { return is_bound_to_buffer_; }
    [[nodiscard]] source_id get_source_id()       const { return source_id_; }
    [[nodiscard]] bool is_bound_to_source()       const { return is_bound_to_source_; }
    [[nodiscard]] ALenum get_format()             const { return format_; }
    [[nodiscard]] ALsizei get_data_size_in_byte() const { return data_.size() * sizeof(ALshort); }
    [[nodiscard]] ALsizei get_sampling_rate()     const { return sampling_rate_; }

  private:
    buffer_id buffer_id_;
    bool is_bound_to_buffer_ = false;
    source_id source_id_;
    bool is_bound_to_source_ = false;

    std::vector<ALshort> data_;
    ALenum format_;
    ALsizei sampling_rate_;
};
} // namespace hnll::audio