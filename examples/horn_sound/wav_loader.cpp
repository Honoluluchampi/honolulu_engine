#include <AudioFile/AudioFile.h>
#include <audio/engine.hpp>
#include <audio/audio_data.hpp>

int main() {
  AudioFile<short> audio_file;
  audio_file.load("/home/honolulu/programs/honolulu_engine/sounds/nandesyou.wav");
  audio_file.printSummary();

  int sample_rate = audio_file.getSampleRate();
  int num_samples = audio_file.getNumSamplesPerChannel();
  int bit_depth = audio_file.getBitDepth();
  bool is_mono = audio_file.isMono();
  bool is_stereo = audio_file.isStereo();

  // choose format
  ALuint format;
  if (bit_depth == 8 && is_mono)
    format = AL_FORMAT_MONO8;
  else if (bit_depth == 16 && is_mono)
    format = AL_FORMAT_MONO16;
  else if (bit_depth == 8 && is_stereo)
    format = AL_FORMAT_STEREO8;
  else if (bit_depth == 16 && is_stereo)
    format = AL_FORMAT_STEREO16;
  else
    throw std::runtime_error("unsupported wav format.");

  // open al
  hnll::audio::engine::start_hae_context();
  auto source = hnll::audio::engine::get_available_source_id();

  float dt = 0.016; // 16 ms
  int queue_capability = 4;

  bool started = false;
  int cursor = 0;
  do {
    // continue if queue is full
    hnll::audio::engine::erase_finished_audio_on_queue(source);

    if (hnll::audio::engine::get_audio_count_on_queue(source) > queue_capability)
      continue;

    int segment_count = static_cast<int>(dt * sample_rate);

    // load raw data
    std::vector<ALshort> segment(segment_count * 2, 0.f);
    for (int i = 0; i < segment_count; i++) {
      segment[2 * i] = audio_file.samples[0][i + cursor];
      segment[2 * i + 1] = audio_file.samples[1][i + cursor];
    }
    cursor += segment_count;

    hnll::audio::audio_data data;
    data.set_sampling_rate(sample_rate)
      .set_format(format)
      .set_data(std::move(segment));

    hnll::audio::engine::bind_audio_to_buffer(data);
    hnll::audio::engine::queue_buffer_to_source(source, data.get_buffer_id());

    if (!started) {
      hnll::audio::engine::play_audio_from_source(source);
      started = true;
    }
  } while (hnll::audio::engine::get_audio_count_on_queue(source) > 0);

  hnll::audio::engine::kill_hae_context();
}