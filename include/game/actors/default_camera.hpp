#pragma once

// hnll
#include <game/actor.hpp>
#include <game/components/viewer_comp.hpp>
#include <game/components/key_move_comp.hpp>
#include <utils/rendering_utils.hpp>
#include <utils/utils.hpp>

namespace hnll {
namespace game {

DEFINE_PURE_ACTOR(default_camera)
{
  public:
    default_camera();
    ~default_camera(){}

    default_camera(const default_camera &) = delete;
    default_camera& operator=(const default_camera &) = delete;

    void update(const float& dt);
    void update_frustum() { viewer_comp_->update_frustum(); }
    // getter
    utils::viewer_info  get_viewer_info()  const { return {viewer_comp_->get_projection(), viewer_comp_->get_view(), viewer_comp_->get_inverse_view()}; }
    utils::frustum_info get_frustum_info() const;
    bool is_movement_updating() const { return key_comp_->is_updating(); }
    inline hnll::utils::transform& get_transform() { return transform_; }
    inline const viewer_comp& get_viewer_comp() const { return *viewer_comp_; }

    // setter
    void set_movement_updating_on()  { key_comp_->set_updating_on(); }
    void set_movement_updating_off() { key_comp_->set_updating_off(); }

    template<class V> void set_translation(V&& vec) { transform_.translation = std::forward<V>(vec); }
    template<class V> void set_scale(V&& vec) { transform_.scale = std::forward<V>(vec); }
    template<class V> void set_rotation(V&& vec) { transform_.rotation = std::forward<V>(vec); }

  private:
    u_ptr<viewer_comp>   viewer_comp_;
    u_ptr<key_move_comp> key_comp_;
};

}} // namespace hnll::game