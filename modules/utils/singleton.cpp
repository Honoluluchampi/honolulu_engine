// hnll
#include <utils/singleton.hpp>

// std
#include <vector>

namespace hnll::utils {

std::mutex singleton_deleter::mutex_;
std::vector<singleton_deleter::deleter> singleton_deleter::deleters_;

void singleton_deleter::add_deleter(const deleter& func) {
  std::lock_guard<std::mutex> lock(mutex_);
  deleters_.push_back(func);
}

void singleton_deleter::delete_reversely() {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto iter = deleters_.rbegin(); iter != deleters_.rend(); iter++) {
    (*iter)();
  }
}

} // namespace hnll::utils