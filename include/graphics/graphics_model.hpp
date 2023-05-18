#pragma once

// hnll
#include <utils/rendering_utils.hpp>
#include <graphics/texture_image.hpp>

#define DEFINE_GRAPHICS_MODEL(derived, type) class derived : public graphics::graphics_model<derived, type>

namespace hnll::graphics {

template <typename Derived, utils::shading_type type>
class graphics_model
{
  public:
    // getter
    constexpr static utils::shading_type get_shading_type() { return type; }
    inline bool is_textured() const { return is_textured_; }
    inline VkDescriptorSet get_texture_desc_set() const { return texture_image_->get_vk_desc_set(); }

    // setter
    void set_texture(u_ptr<texture_image>&& texture)
    {
      texture_image_ = std::move(texture);
      is_textured_ = true;
    }

  private:
    u_ptr<graphics::texture_image> texture_image_;
    bool is_textured_ = false;
};

} // namespace hnll::graphics