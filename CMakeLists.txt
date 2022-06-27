cmake_minimum_required(VERSION 3.13)

function(add_shader TARGET SHADER SHADER_OUT_NAME)
    find_program(GLSLC glslc)

    set(current-shader-path ${CMAKE_CURRENT_SOURCE_DIR}/shaders/${SHADER})
    set(current-output-path ${CMAKE_CURRENT_SOURCE_DIR}/shaders/${SHADER_OUT_NAME}.spv)

    # Add a custom command to compile GLSL to SPIR-V.
    get_filename_component(current-output-dir ${current-output-path} DIRECTORY)
    file(MAKE_DIRECTORY ${current-output-dir})

    add_custom_command(
           OUTPUT ${current-output-path}
           COMMAND ${GLSLC} -o ${current-output-path} ${current-shader-path}
           DEPENDS ${current-shader-path}
           IMPLICIT_DEPENDS CXX ${current-shader-path}
           VERBATIM)

    # Make sure our build depends on this output.
    set_source_files_properties(${current-output-path} PROPERTIES GENERATED TRUE)
    target_sources(${TARGET} PRIVATE ${current-output-path})
endfunction(add_shader)

project(CG_project)
find_package(Vulkan REQUIRED FATAL_ERROR)
find_package(glfw3 REQUIRED FATAL_ERROR)


add_executable(CG_project MyProject.cpp)

target_include_directories (CG_project
    PUBLIC ${GLFW_INCLUDE_DIRS}
    PUBLIC ${Vulkan_INCLUDE_DIRS}
)

SET(BASEPATH "${CMAKE_SOURCE_DIR}")
INCLUDE_DIRECTORIES("${BASEPATH}" headers)
target_compile_features(CG_project PRIVATE cxx_std_17)

target_link_libraries (CG_project glfw)
target_link_libraries (CG_project ${Vulkan_LIBRARIES})

add_shader(CG_project shader.frag frag)
add_shader(CG_project shader.vert vert)
add_shader(CG_project shaderSkyBox.frag SkyBoxfrag)
add_shader(CG_project shaderSkyBox.vert SkyBoxvert)