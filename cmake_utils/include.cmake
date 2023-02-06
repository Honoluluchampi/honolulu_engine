function(detect_brew_prefix output)
    # search brew's root
    execute_process(
            COMMAND brew --prefix
            RESULT_VARIABLE BREW
            OUTPUT_VARIABLE BREW_PREFIX
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(${output} ${BREW_PREFIX} PARENT_SCOPE)
endfunction()

function(setup_brew_module_directory module_name module_directory_parameter)
    execute_process(
            COMMAND ls ${BREW_PREFIX}/Cellar/${module_name}
            RESULT_VARIABLE MODULE_EXIST
            OUTPUT_VARIABLE MODULE_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(${module_directory_parameter} ${BREW_PREFIX}/Cellar/${module_name}/${MODULE_VERSION} PARENT_SCOPE)
endfunction()

function(include_common_dependencies target)
    target_include_directories(${target} PUBLIC
            $ENV{HNLL_ENGN}/include
            $ENV{HNLL_ENGN}/submodules
            $ENV{VULKAN_DIR}/include
            )
    target_link_directories(${target} PUBLIC $ENV{VULKAN_DIR}/lib)
endfunction()

# include necessary libraries for this engine
function(include_mac_dependencies target)
    detect_brew_prefix(BREW_PREFIX)
    setup_brew_module_directory(eigen EIGEN_DIRECTORY)
    setup_brew_module_directory(glm GLM_DIRECTORY)
    setup_brew_module_directory(glfw GLFW_DIRECTORY)
    setup_brew_module_directory(openal-soft OPEN_AL_DIRECTORY)
    setup_brew_module_directory(googletest GOOGLE_TEST_DIRECTORY)
    target_include_directories(${target} PUBLIC
            ${EIGEN_DIRECTORY}/include
            ${GLM_DIRECTORY}/include
            ${GLFW_DIRECTORY}/include
            ${OPEN_AL_DIRECTORY}/include
            ${GOOGLE_TEST_DIRECTORY}/include
            )
    target_link_directories(${target} PUBLIC
            ${OPEN_AL_DIRECTORY}/lib
            ${GLFW_DIRECTORY}/lib
            )
endfunction()