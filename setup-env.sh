# configure this part depending on your environment
export HNLL_ENGN=$(cd $(dirname ${BASH_SOURCE:-$0}); pwd)
VULKAN_DIR=~/programs/external_libraries/vulkanSDK

source $VULKAN_DIR/setup-env.sh
if [ "$(uname)" == 'Darwin' ]; then
  export VULKAN_DIR=$VULKAN_DIR/macOS
elif [ "$(expr substr $(uname -s) 1 5)" == 'Linux' ]; then
  export VULKAN_DIR=$VULKAN_DIR/x86_64
fi

source setup-submodules.sh
source setup-shaders.sh

export MODEL_DIR=$HNLL_ENGN/models