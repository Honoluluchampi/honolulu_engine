// hnll
#include <audio/convolver.hpp>
#include <audio/utils.hpp>

namespace hnll::audio {

u_ptr<convolver> convolver::create(size_t data_size)
{ return std::make_unique<convolver>(data_size); }

convolver::convolver(size_t data_size)
{
  data_size_ = data_size;
  buffers_[0].resize(data_size);
  buffers_[1].resize(data_size);
}

void convolver::add_segment(std::vector<ALshort>&& data, std::vector<double>&& filter)
{
  assert(data.size() == data_size_);
  assert(filter.size() == data_size_);
  // zero padding
  data.resize(data_size_ * 2);
  filter.resize(data_size_ * 2);

  // convolve
  auto data_f = utils::fft(data);
  auto filter_f = utils::fft(filter);

  std::vector<std::complex<double>> conv_f(data_size_ * 2);
  for (int i = 0; i < data_size_ * 2; i++) {
    conv_f[i] = data_f[i] * filter_f[i];
  }

  auto conv = utils::ifft(conv_f);

  // assign to buffers
  buffers_[!frame_index].resize(data_size_);

  // TODO : handle overflow after summing up
  for (int i = 0; i < data_size_; i++) {
    buffers_[frame_index][i] += std::clamp(
      static_cast<ALshort>(conv[i].real()),
      std::numeric_limits<ALshort>::min(),
      std::numeric_limits<ALshort>::max()
    );
  }

  for (int i = 0; i < data_size_; i++) {
    buffers_[!frame_index][i] += std::clamp(
      static_cast<ALshort>(conv[i + data_size_].real()),
      std::numeric_limits<ALshort>::min(),
      std::numeric_limits<ALshort>::max()
    );
  }
}

} // namespace hnll::audio