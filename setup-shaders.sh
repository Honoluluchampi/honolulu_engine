# compile shaders in a chosen directory
if [ $# == 1 ]; then
  TARGET_ENV=vulkan1.3
  SHADERDIR=$1
  OUTPUTDIR=$1/spv/

  mkdir -p ${OUTPUTDIR}
  COMPILER=${VULKAN_DIR}/bin/glslc

  for FILE in ${SHADERDIR}/*.*
    do
      FILENAME=$(basename ${FILE})
      ${COMPILER} $FILE -o ${OUTPUTDIR}/${FILENAME}.spv
    done
fi

# compile all the default shaders
if [ $# == 0 ]; then

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

  # for physics compute shader
  COMPILER=${VULKAN_DIR}/bin/glslc
  SHADERDIR=${HNLL_ENGN}/modules/physics/shaders/compute_shader
  OUTPUTDIR=${HNLL_ENGN}/modules/physics/shaders/spv

  mkdir -p ${OUTPUTDIR}

  for FILE in ${SHADERDIR}/*.*
    do
      FILENAME=$(basename ${FILE})
      ${COMPILER} $FILE -o ${OUTPUTDIR}/${FILENAME}.spv
  done

  SHADERDIR=${HNLL_ENGN}/modules/physics/shaders/vertex_shader

  mkdir -p ${OUTPUTDIR}

  for FILE in ${SHADERDIR}/*.*
    do
      FILENAME=$(basename ${FILE})
      ${COMPILER} $FILE -o ${OUTPUTDIR}/${FILENAME}.spv
  done

fi