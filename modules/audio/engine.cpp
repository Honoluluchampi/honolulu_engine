// hnll
#include <audio/engine.hpp>
#include <audio/audio_data.hpp>

// std
#include <iostream>

namespace hnll::audio {

// define static members
std::queue<source_id> engine::pending_source_ids_;
ALCdevice*  engine::device_ = nullptr;
ALCcontext* engine::context_ = nullptr;

void engine::start_hae_context()
{
  if (device_ || context_) return;
  // initialize openAL
  device_ = alcOpenDevice(nullptr);
  // TODO : configure context's attributes
  context_ = alcCreateContext(device_, nullptr);
  alcMakeContextCurrent(context_);

  // create pending sources
  source_id sources[SOURCE_COUNT];
  alGenSources(SOURCE_COUNT, sources);
  for (const auto& source : sources) {
    pending_source_ids_.push(source);
  }
}

void engine::kill_hae_context()
{
  // delete openAL resources
  alcMakeContextCurrent(nullptr);
  alcDestroyContext(context_);
  alcCloseDevice(device_);
}

result engine::bind_audio_to_buffer(audio_data &audio_data)
{
  // generate buffer
  buffer_id buffer_id;
  if (audio_data.is_bound_to_buffer())
    buffer_id = audio_data.get_buffer_id();
  else
    alGenBuffers(1, &buffer_id);

  // TODO : use try & catch to check alBufferData is successfully processed
  // copy the data to the buffer
  alBufferData(buffer_id,
               audio_data.get_format(),
               audio_data.get_data(),
               audio_data.get_data_size_in_byte(),
               audio_data.get_sampling_rate());

  // change audio_data's state
  audio_data.set_buffer_id(buffer_id);
  audio_data.set_is_bound_to_buffer(true);

  return result::SUCCESS;
}

result engine::bind_buffer_to_source(audio_data &audio_data)
{
  // check whether audio_data has a buffer
  if (!audio_data.is_bound_to_buffer()) return result::FAILURE;
  // check whether there is a remaining pending source
  if (pending_source_ids_.empty()) return result::FAILURE;

  // get source from pending_source
  auto source_id = pending_source_ids_.front();
  pending_source_ids_.pop();

  // bind buffer to source
  alSourcei(source_id, AL_BUFFER, audio_data.get_buffer_id());

  // change audio_data's state
  audio_data.set_source_id(source_id);
  audio_data.set_is_bound_to_source(true);

  return result::SUCCESS;
}

result engine::remove_audio_resources(audio_data& audio_data)
{
  // remove source
  if (audio_data.is_bound_to_source()) {
    remove_audio_from_source(audio_data);
  }

  // remove buffer
  if (audio_data.is_bound_to_buffer()) {
    auto buffer = audio_data.get_buffer_id();
    alDeleteBuffers(1, &buffer);
    audio_data.set_is_bound_to_buffer(false);
  }

  return result::SUCCESS;
}

result engine::remove_audio_from_source(audio_data& audio_data)
{
  // remove the audio from  the source
  alSourcei(audio_data.get_source_id(), AL_BUFFER, 0);

  // change the audio_data's state
  audio_data.set_is_bound_to_source(false);

  // make the source pending
  pending_source_ids_.push(audio_data.get_source_id());

  return result::SUCCESS;
}

source_id engine::get_available_source_id()
{
  source_id ret;
  if (!pending_source_ids_.empty()) {
    ret = pending_source_ids_.front();
    pending_source_ids_.pop();
  }
  else {
    ret = -1;
    std::cerr << "all the openAL sources are not available." << std::endl;
  }

  return ret;
}

result engine::queue_buffer_to_source(source_id source, buffer_id buffer)
{
  alSourceQueueBuffers(source, 1, &buffer);
  return result::SUCCESS;
}

void engine::erase_finished_audio_on_queue(source_id source)
{
  int processed;
  alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);

  if (processed > 0) {
    std::vector<buffer_id> finished_buffer_id(processed);
    // remove from queue
    alSourceUnqueueBuffers(source, processed, finished_buffer_id.data());
    // erase buffer
    alDeleteBuffers(processed, finished_buffer_id.data());
  }
}

int engine::get_audio_count_on_queue(source_id source)
{
  int count;
  alGetSourcei(source, AL_BUFFERS_QUEUED, &count);
  return count;
}

} // namespace hnll::audio