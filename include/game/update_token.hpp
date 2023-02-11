#pragma once

// std
#include <unordered_map>
#include <concepts>
#include <variant>

namespace hnll::game {

template <typename T>
concept Updatable =
  requires (T a, const float& dt) { a.update(dt); } &&
  requires (const T a) { a.get_update_token_id(); };

using token_id = unsigned;

template <Updatable Parent, Updatable ...Children>
class update_token
{
  public:
    void update(const float& dt)
    {

    }

    template <Updatable Child>
    void add_child(Child& child) { children_[child->get_update_token_id()] = child; }

    void delete_child(token_id id) { children_.erase(id); }

    // getter
    token_id get_token_id() const { return token_id_; }

  private:
    token_id token_id_;
    Parent& parent_;
    std::unordered_map<token_id, std::variant<Children&...>> children_;
};

} // namespace hnll::game