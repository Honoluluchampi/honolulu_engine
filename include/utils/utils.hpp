#pragma once

// hnll
#include <utils/common_alias.hpp>

// std
#include <memory>
#include <vector>
#include <iostream>
#include <string>
#include <chrono>

namespace hnll::utils {

// model loading ----------------------------------------------------
static std::vector<std::string> loading_directories {
  std::string(std::getenv("HNLL_ENGN")) + "/models/characters",
  std::string(std::getenv("HNLL_ENGN")) + "/models/primitives"
};

inline std::string get_engine_root_path() { return (getenv("HNLL_ENGN")); }
std::string get_full_path(const std::string& _filename);
void mkdir_p(const std::string& _dir_name);
// returns cache directory
std::string create_cache_directory();
// returns sub cache directory
std::string create_sub_cache_directory(const std::string& _dir_name);

std::vector<char> read_file_for_shader(const std::string& filepath);

// 3d transformation -------------------------------------------------
struct transform
{
  vec3 translation { 0.f, 0.f, 0.f }; // position offset
  vec3 scale { 1.f, 1.f, 1.f };
  // y-z-x tait-brian rotation
  vec3 rotation{ 0.f, 0.f, 0.f };

  // Matrix corresponds to Translate * Ry * Rz * Rx * Scale
  // Rotations correspond to Tait-bryan angles of Y(1), Z(2), X(3)
  // https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
  [[nodiscard]] mat4 transform_mat4() const;
  [[nodiscard]] mat4 rotate_mat4() const;
  [[nodiscard]] mat3 rotate_mat3() const;
  // normal = R * S(-1)
  [[nodiscard]] mat4 normal_matrix() const;
};

// timer ------------------------------------------------------------
enum class timer_type
{
    SEC,
    MILLI,
    MICRO
};

struct scope_timer {
  // start timer by ctor
  explicit scope_timer(const std::string& _entry = "", timer_type type = timer_type::MILLI) {
    entry = _entry;
    start = std::chrono::system_clock::now();
    type_ = type;
  }

  // stop and output elapsed time by dtor
  ~scope_timer() {
    auto end = std::chrono::system_clock::now();
    std::cout << "\x1b[35m" << "SCOPE TIMER : " << entry << std::endl;

    long elapsed;

    switch (type_) {
      case timer_type::SEC :
        elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
        std::cout << "\t elapsed time : " << elapsed << " s" << "\x1b[0m" << std::endl;
        break;

      case timer_type::MILLI :
        elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "\t elapsed time : " << elapsed << " ms" << "\x1b[0m" << std::endl;
        break;

      case timer_type::MICRO :
        elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        std::cout << "\t elapsed time : " << elapsed << " micro s" << "\x1b[0m" << std::endl;
        break;

      default :
        std::cout << "\t invalid timer type" << "\x1b[0m" << std::endl;
        break;
    }
  }

  std::string entry;
  std::chrono::system_clock::time_point start;
  timer_type type_;
};

} // namespace hnll::utils