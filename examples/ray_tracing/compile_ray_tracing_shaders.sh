if [ $# != 1 ]; then
  echo "usage : bash compile_ray_tracing_shaders.sh shaders_directory_name"
fi

if [ $# == 1 ]; then

COMPILER=${VULKAN_DIR}/x86_64/bin/glslangValidator
TARGET_ENV=vulkan1.3

SHADERDIR=$1
OUTPUTDIR=$1/spv
mkdir -p ${OUTPUTDIR}

extensions=("rgen" "rmiss" "rchit" "rahit" "rint" "comp")

for extension in ${extensions[@]}
  do
    for FILE in ${SHADERDIR}/*.${extension}
      do
        FILENAME=$(basename ${FILE})
        ${COMPILER} -S ${extension} ${FILE} --target-env ${TARGET_ENV} -o ${OUTPUTDIR}/${FILENAME}.spv
    done
done

fi