// std
#include <queue>
#include <memory>

// openAL
#include <AL/al.h>
#include <AL/alc.h>

// engine
namespace hnll::audio {

// forward declaration
class audio_data;

using buffer_id = ALuint;
using source_id = ALuint;

constexpr size_t SOURCE_COUNT = 4;

// represents hae process's result state
enum class result { SUCCESS, FAILURE };

class engine
{
  public:
    static void start_hae_context();
    static void kill_hae_context();

    // audio process functions
    static result bind_audio_to_buffer(audio_data& audio_data);
    static result bind_buffer_to_source(audio_data& audio_data);

    static result remove_audio_resources(audio_data& audio_data);
    static result remove_audio_from_source(audio_data& audio_data);

    inline static void play_audio_from_source(source_id id) { alSourcePlay(id); }
    inline static void stop_audio_from_source(source_id id) { alSourceStop(id); }

    // for queueing
    static source_id get_available_source_id();
    static result    queue_buffer_to_source(source_id source, buffer_id buffer);
    static void      erase_finished_audio_on_queue(source_id source);
    static int       get_audio_count_on_queue(source_id source);

    // getter
    static size_t remaining_pending_sources_count() { return pending_source_ids_.size(); }

  private:
    // openAL resources
    static std::queue<source_id> pending_source_ids_;

    static ALCdevice* device_;
    static ALCcontext* context_;
};
} // namespace hnll::audio#pragma once