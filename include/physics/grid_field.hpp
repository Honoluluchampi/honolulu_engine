#pragma once

// hnll
#include <utils/common_alias.hpp>

namespace hnll::physics {

template <std::floating_point T, size_t x_grid, size_t y_grid>
class grid_field
{
  public:

    // getter
    T* get_raw_data() { return data_.data(); }
    size_t get_x_grid_count() const { return x_grid; }
    size_t get_y_grid_count() const { return y_grid; }

    // setter
    void set_value(int x, int y, const T& value) { data_[x + y * x_grid] = value; }

  private:
    std::array<T, x_grid * y_grid> data_;
};

} // namespace hnll::physics