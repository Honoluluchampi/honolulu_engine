# download submodules
mkdir -p $HNLL_ENGN/submodules
# download imgui
if [ ! -e $HNLL_ENGN/submodules/imgui/imgui.cpp ]; then
  mkdir -p $HNLL_ENGN/submodules/imgui
  echo "downloading imgui"
  curl -o $HNLL_ENGN/submodules/imgui/imgui.cpp https://raw.githubusercontent.com/ocornut/imgui/docking/imgui.cpp
  curl -o $HNLL_ENGN/submodules/imgui/imgui_demo.cpp https://raw.githubusercontent.com/ocornut/imgui/docking/imgui_demo.cpp
  curl -o $HNLL_ENGN/submodules/imgui/imgui_draw.cpp https://raw.githubusercontent.com/ocornut/imgui/docking/imgui_draw.cpp
  curl -o $HNLL_ENGN/submodules/imgui/imgui_widgets.cpp https://raw.githubusercontent.com/ocornut/imgui/docking/imgui_widgets.cpp
  curl -o $HNLL_ENGN/submodules/imgui/imgui_tables.cpp https://raw.githubusercontent.com/ocornut/imgui/docking/imgui_tables.cpp
  mkdir -p $HNLL_ENGN/submodules/imgui/backends
  curl -o $HNLL_ENGN/submodules/imgui/backends/imgui_impl_vulkan.cpp https://raw.githubusercontent.com/ocornut/imgui/docking/backends/imgui_impl_vulkan.cpp
  curl -o $HNLL_ENGN/submodules/imgui/backends/imgui_impl_glfw.cpp https://raw.githubusercontent.com/ocornut/imgui/docking/backends/imgui_impl_glfw.cpp
  curl -o $HNLL_ENGN/submodules/imgui/roboto_regular.embed https://raw.githubusercontent.com/TheCherno/Walnut/master/Walnut/src/Walnut/ImGui/Roboto-Regular.embed
  curl -o $HNLL_ENGN/submodules/imgui/imgui.h https://raw.githubusercontent.com/ocornut/imgui/docking/imgui.h
  curl -o $HNLL_ENGN/submodules/imgui/imgui_internal.h https://raw.githubusercontent.com/ocornut/imgui/docking/imgui_internal.h
  curl -o $HNLL_ENGN/submodules/imgui/imstb_rectpack.h https://raw.githubusercontent.com/ocornut/imgui/docking/imstb_rectpack.h
  curl -o $HNLL_ENGN/submodules/imgui/imstb_textedit.h https://raw.githubusercontent.com/ocornut/imgui/docking/imstb_textedit.h
  curl -o $HNLL_ENGN/submodules/imgui/imstb_truetype.h https://raw.githubusercontent.com/ocornut/imgui/docking/imstb_truetype.h
  curl -o $HNLL_ENGN/submodules/imgui/imconfig.h https://raw.githubusercontent.com/ocornut/imgui/docking/imconfig.h
  curl -o $HNLL_ENGN/submodules/imgui/backends/imgui_impl_vulkan.h https://raw.githubusercontent.com/ocornut/imgui/docking/backends/imgui_impl_vulkan.h
  curl -o $HNLL_ENGN/submodules/imgui/backends/imgui_impl_glfw.h https://raw.githubusercontent.com/ocornut/imgui/docking/backends/imgui_impl_glfw.h
fi
export IMGUI_DIR=$HNLL_ENGN/submodules/imgui

# download tiny_obj_loader
if [ ! -e $HNLL_ENGN/submodules/tiny_obj_loader/tiny_obj_loader.h ]; then
  mkdir -p $HNLL_ENGN/submodules/tiny_obj_loader
  echo "downloading tiny obj loader"
  curl -o $HNLL_ENGN/submodules/tiny_obj_loader/tiny_obj_loader.h https://raw.githubusercontent.com/tinyobjloader/tinyobjloader/master/tiny_obj_loader.h
fi
export TINY_OBJ_LOADER_DIR=$HNLL_ENGN/submodules/tiny_obj_loader

# download tiny_gltf
if [ ! -e $HNLL_ENGN/submodules/tiny_gltf/tiny_gltf.h ]; then
  mkdir -p $HNLL_ENGN/submodules/tiny_gltf
  echo "downloading tiny gltf"
  curl -o $HNLL_ENGN/submodules/tiny_gltf/tiny_gltf.h       https://raw.githubusercontent.com/syoyo/tinygltf/release/tiny_gltf.h
  curl -o $HNLL_ENGN/submodules/tiny_gltf/stb_image.h       https://raw.githubusercontent.com/syoyo/tinygltf/release/stb_image.h
  curl -o $HNLL_ENGN/submodules/tiny_gltf/stb_image_write.h https://raw.githubusercontent.com/syoyo/tinygltf/release/stb_image_write.h
  curl -o $HNLL_ENGN/submodules/tiny_gltf/json.hpp          https://raw.githubusercontent.com/syoyo/tinygltf/release/json.hpp
fi
export TINY_GLTF_DIR=$HNLL_ENGN/submodules/tiny_gltf

# download stb image
if [ ! -e $HNLL_ENGN/submodules/stb_image/stb_image.h ]; then
  mkdir -p $HNLL_ENGN/submodules/stb_image
  echo "downloading stb image"
  curl -o $HNLL_ENGN/submodules/stb_image/stb_image.h https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
fi

# download nv vulkan extensions
if [ ! -e $HNLL_ENGN/submodules/extensions/extensions_vk.hpp ]; then
  mkdir -p $HNLL_ENGN/submodules/extensions
  echo "downloading nv vulkan extensions"
  curl -o $HNLL_ENGN/submodules/extensions/extensions_vk.hpp https://raw.githubusercontent.com/nvpro-samples/nvpro_core/master/nvvk/extensions_vk.hpp
  curl -o $HNLL_ENGN/submodules/extensions/extensions_vk.cpp https://raw.githubusercontent.com/nvpro-samples/nvpro_core/master/nvvk/extensions_vk.cpp
  # dummy
  mkdir -p $HNLL_ENGN/submodules/extensions/nvh
  touch $HNLL_ENGN/submodules/extensions/nvh/nvprint.hpp
fi

# download audio file
if [ ! -e $HNLL_ENGN/submodules/AudioFile/AudioFile.h ]; then
  mkdir -p $HNLL_ENGN/submodules/AudioFile
  echo "downloading AudioFile"
  curl -o $HNLL_ENGN/submodules/AudioFile/AudioFile.h https://raw.githubusercontent.com/adamstark/AudioFile/master/AudioFile.h
fi

# download igl
if [ ! -e $HNLL_ENGN/submodules/igl/igl_inline.h ]; then
  mkdir -p $HNLL_ENGN/submodules/igl
  echo "downloading igl"
  curl -o $HNLL_ENGN/submodules/igl/igl_inline.h https://raw.githubusercontent.com/libigl/libigl/main/include/igl/igl_inline.h
  curl -o $HNLL_ENGN/submodules/igl/read_triangle_mesh.h https://raw.githubusercontent.com/libigl/libigl/main/include/igl/read_triangle_mesh.h
  curl -o $HNLL_ENGN/submodules/igl/writeOBJ.h https://raw.githubusercontent.com/libigl/libigl/main/include/igl/writeOBJ.h
fi