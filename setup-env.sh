# configure this part depending on your environment
export HNLL_ENGN=$(cd $(dirname ${BASH_SOURCE:-$0}); pwd)
VULKAN_DIR=~/programs/external_libraries/vulkanSDK

source $VULKAN_DIR/setup-env.sh
if [ "$(uname)" == 'Darwin' ]; then
  export VULKAN_DIR=$VULKAN_DIR/macOS
elif [ "$(expr substr $(uname -s) 1 5)" == 'Linux' ]; then
  export VULKAN_DIR=$VULKAN_DIR/x86_64
fi

# download submodules
mkdir -p $HNLL_ENGN/submodules
# download imgui
if [ ! -e $HNLL_ENGN/submodules/imgui/imgui.cpp ]; then
  mkdir -p $HNLL_ENGN/submodules/imgui
  echo "downloading imgui"
  curl -o $HNLL_ENGN/submodules/imgui/imgui.cpp https://raw.githubusercontent.com/ocornut/imgui/master/imgui.cpp
  curl -o $HNLL_ENGN/submodules/imgui/imgui_demo.cpp https://raw.githubusercontent.com/ocornut/imgui/master/imgui_demo.cpp
  curl -o $HNLL_ENGN/submodules/imgui/imgui_draw.cpp https://raw.githubusercontent.com/ocornut/imgui/master/imgui_draw.cpp
  curl -o $HNLL_ENGN/submodules/imgui/imgui_widgets.cpp https://raw.githubusercontent.com/ocornut/imgui/master/imgui_widgets.cpp
  curl -o $HNLL_ENGN/submodules/imgui/imgui_tables.cpp https://raw.githubusercontent.com/ocornut/imgui/master/imgui_tables.cpp
  mkdir -p $HNLL_ENGN/submodules/imgui/backends
  curl -o $HNLL_ENGN/submodules/imgui/backends/imgui_impl_vulkan.cpp https://raw.githubusercontent.com/ocornut/imgui/master/backends/imgui_impl_vulkan.cpp
  curl -o $HNLL_ENGN/submodules/imgui/backends/imgui_impl_glfw.cpp https://raw.githubusercontent.com/ocornut/imgui/master/backends/imgui_impl_glfw.cpp
  curl -o $HNLL_ENGN/submodules/imgui/roboto_regular.embed https://raw.githubusercontent.com/TheCherno/Walnut/master/Walnut/src/Walnut/ImGui/Roboto-Regular.embed
fi
export IMGUI_DIR=$HNLL_ENGN/submodules/imgui

# download tiny_obj_loader
if [ ! -e $HNLL_ENGN/submodules/tiny_obj_loader/tiny_obj_loader.h ]; then
  mkdir -p $HNLL_ENGN/submodules/tiny_obj_loader
  echo "download tiny obj loader"
  curl -o $HNLL_ENGN/submodules/tiny_obj_loader/tiny_obj_loader.h https://raw.githubusercontent.com/tinyobjloader/tinyobjloader/master/tiny_obj_loader.h
fi
export TINY_OBJ_LOADER_DIR=$HNLL_ENGN/submodules/tiny_obj_loader

# download tiny_gltf
if [ ! -e $HNLL_ENGN/submodules/tiny_gltf/tiny_gltf.h ]; then
  mkdir -p $HNLL_ENGN/submodules/tiny_gltf
  echo "download tiny gltf"
  curl -o $HNLL_ENGN/submodules/tiny_gltf/tiny_gltf.h       https://raw.githubusercontent.com/syoyo/tinygltf/release/tiny_gltf.h
  curl -o $HNLL_ENGN/submodules/tiny_gltf/stb_image.h       https://raw.githubusercontent.com/syoyo/tinygltf/release/stb_image.h
  curl -o $HNLL_ENGN/submodules/tiny_gltf/stb_image_write.h https://raw.githubusercontent.com/syoyo/tinygltf/release/stb_image_write.h
  curl -o $HNLL_ENGN/submodules/tiny_gltf/json.hpp          https://raw.githubusercontent.com/syoyo/tinygltf/release/json.hpp
fi
export TINY_GLTF_DIR=$HNLL_ENGN/submodules/tiny_gltf

export MODEL_DIR=$HNLL_ENGN/models

# compile shaders
#source ${HNLL_ENGN}/modules/graphics/compile.sh