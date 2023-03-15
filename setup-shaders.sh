# common
TARGET_ENV=vulkan1.3
OUTPUTDIR=${HNLL_ENGN}/modules/graphics/shaders/spv


# for vertex shader (traditional graphics pipeline) ------------------------
COMPILER=${VULKAN_DIR}/bin/glslc
SHADERDIR=${HNLL_ENGN}/modules/graphics/shaders/vertex_shader

mkdir -p ${OUTPUTDIR}

for FILE in ${SHADERDIR}/*.*
  do
    FILENAME=$(basename ${FILE})
    ${COMPILER} $FILE -o ${OUTPUTDIR}/${FILENAME}.spv
done

# for mesh shader ----------------------------------------------------------
COMPILER=${VULKAN_DIR}/bin/glslangValidator
SHADERDIR=${HNLL_ENGN}/modules/graphics/shaders/mesh_shader

extensions=("mesh" "frag" "task")

for extension in ${extensions[@]}
  do
    for FILE in ${SHADERDIR}/*.${extension}.glsl
      do
        FILENAME=$(basename ${FILE})
        ${COMPILER} -S ${extension} ${FILE} --target-env ${TARGET_ENV} -o ${OUTPUTDIR}/${FILENAME}.spv
    done
done