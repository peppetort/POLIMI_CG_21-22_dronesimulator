cmake_minimum_required(VERSION 3.10)

project(CG_project)
find_package(Vulkan REQUIRED FATAL_ERROR)
find_package(glfw3 REQUIRED FATAL_ERROR)
add_executable(CG_project MyProject.cpp)

target_include_directories (CG_project
    PUBLIC ${GLFW_INCLUDE_DIRS}
    PUBLIC ${Vulkan_INCLUDE_DIRS}
)

include_directories(headers)
target_compile_features(CG_project PRIVATE cxx_std_17)

target_link_libraries (CG_project glfw)
target_link_libraries (CG_project ${Vulkan_LIBRARIES})