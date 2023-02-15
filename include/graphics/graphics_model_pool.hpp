#pragma once

// hnll
#include <graphics/device.hpp>
#include <graphics/buffer.hpp>
#include <graphics/utils.hpp>
#include <graphics/graphics_models/static_mesh.hpp>
#include <utils/utils.hpp>

// std
#include <filesystem>

namespace hnll::graphics {

class graphics_model_pool
{
    template <utils::shading_type type>
    using model_map = std::unordered_map<std::string, u_ptr<graphics_model<type>>>;

  public:
    static u_ptr<graphics_model_pool> create(device& device)
    { return std::make_unique<graphics_model_pool>(device); }

    graphics_model_pool(device& device) : device_(device) {}
    ~graphics_model_pool()
    {
      static_mesh_map_.clear();
      static_meshlet_map_.clear();
      skinning_mesh_map_.clear();
      frame_anim_mesh_map_.clear();
      frame_anim_meshlet_map_.clear();
    }

    template <utils::shading_type type>
    graphics_model<type>& get_model(const std::string& name)
    {
      auto full_path = utils::get_full_path(name);

      if (type == utils::shading_type::MESH) {
        load_model(full_path, static_mesh_map_, ".obj");
        return *static_mesh_map_[name];
      }
    }

  private:
    template <utils::shading_type type>
    void load_model(const std::string &path, model_map<type>& map_, const std::string& extension) {
      // if already loaded
      if (static_mesh_map_.find(path) != static_mesh_map_.end()) return;

      if (std::filesystem::path(path).extension().string() == extension) {
        auto model = graphics_model<type>::create_from_file(device_, path);
        map_.emplace(path, std::move(model));
      }
      else {
        std::cerr << "failed to load \"" << path << "\"." << std::endl;
      }
    }

    device& device_;

    model_map<utils::shading_type::MESH> static_mesh_map_;
    model_map<utils::shading_type::MESHLET> static_meshlet_map_;
    model_map<utils::shading_type::SKINNING_MESH> skinning_mesh_map_;
    model_map<utils::shading_type::FRAME_ANIM_MESH> frame_anim_mesh_map_;
    model_map<utils::shading_type::FRAME_ANIM_MESHLET> frame_anim_meshlet_map_;
};

} // namespace hnll::game