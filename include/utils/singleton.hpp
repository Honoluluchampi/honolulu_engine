#pragma once

// hnll
#include <utils/common_alias.hpp>

// std
#include <cassert>
#include <functional>
#include <mutex>

namespace hnll::utils {

class singleton_deleter
{
    using deleter = std::function<void()>;
  public:
    static void add_deleter(const deleter& func);
    static void delete_reversely();

  private:
    static std::mutex mutex_;
    static std::vector<deleter> deleters_;
};

template <typename T>
class singleton
{
  public:
    static T& get_instance()
    {
      std::call_once(init_flag_, create);
      assert(instance_);
      return *instance_;
    }

  private:
    static void create()
    {
      instance_ = std::make_unique<T>();
      singleton_deleter::add_deleter(deleter_);
    }

    static std::once_flag init_flag_;
    static u_ptr<T> instance_;
    static std::function<void()> deleter_;
};

template <typename T> std::once_flag singleton<T>::init_flag_;
template <typename T> u_ptr<T> singleton<T>::instance_;
template <typename T> std::function<void()> singleton<T>::deleter_ = []() { instance_.reset(); };

} // namespace hnll::utils