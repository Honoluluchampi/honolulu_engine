// hnll
#include <audio/engine.hpp>
#include <audio/audio_data.hpp>

// std
#include <thread>
#include <cmath>
#include <limits>

using namespace hnll::audio;

int main()
{
  engine::start_hae_context();

  // raw audio creation
  const unsigned int freq = 44100;
  const float pitch = 440.0f;
  const float duration = 2.0f;
  std::vector<ALshort> audio(static_cast<size_t>(freq * duration));
  for (int i = 0; i < audio.size(); i++) {
    audio[i] = std::sin(pitch * M_PI * 2.0 * i / freq)
               * std::numeric_limits<ALshort>::max();
  }

  // audio data creation
  audio_data audio_data;
  audio_data
    .set_sampling_rate(freq)
    .set_format(AL_FORMAT_MONO16)
    .set_data(std::move(audio));

  hnll::audio::engine::bind_audio_to_buffer(audio_data);
  hnll::audio::engine::bind_buffer_to_source(audio_data);
  hnll::audio::engine::play_audio_from_source(audio_data.get_source_id());
  std::this_thread::sleep_for(std::chrono::seconds(3));

  hnll::audio::engine::kill_hae_context();
}