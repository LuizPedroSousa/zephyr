cmake_minimum_required(VERSION 3.11)

project(zephyr VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

set(CMAKE_CXX_STANDARD 20)

add_compile_options(-fPIC)
add_definitions(-DENABLE_VALIDATION_LAYER)
add_definitions(-DBACKEND=BACKEND_VULKAN)

set(VCPKG_INSTALLED_ROOT "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-linux")

set(glfw3_DIR "${VCPKG_INSTALLED_ROOT}/share/glfw3")
set(glm_DIR "${VCPKG_INSTALLED_ROOT}/share/glm")

find_package(PkgConfig)
find_package(PkgConfig REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

link_directories(${VCPKG_INSTALLED_ROOT}/lib)

add_definitions(-DGLM_ENABLE_EXPERIMENTAL)

file(GLOB_RECURSE ZEPHYR_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB_RECURSE ZEPHYR_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp)

add_executable(zephyr ${ZEPHYR_SRC} ${ZEPHYR_HEADERS})

target_link_libraries(zephyr PRIVATE glfw glm::glm vulkan)

target_include_directories(zephyr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src) 

target_compile_features(zephyr PRIVATE cxx_std_20)

target_compile_options(zephyr PRIVATE
    -Wall -Wextra -Wpedantic
)
