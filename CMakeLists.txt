cmake_minimum_required(VERSION 3.11)

# ------------------------------------------------------------------------------
# ----------------------------------- Options ----------------------------------
# ------------------------------------------------------------------------------
set(FACELMK3D_EXTERNAL_INCLUDE "" CACHE STRING "External include paths.")
set(FACELMK3D_EXTERNAL_LIBRARY "" CACHE STRING "External libraries.")

# External includes/libraries
list(APPEND FACELMK3D_INCLUDE ${FACELMK3D_EXTERNAL_INCLUDE})
list(APPEND FACELMK3D_LIBRARY ${FACELMK3D_EXTERNAL_LIBRARY})

# ------------------------------------------------------------------------------
# ----------------------------------- Common -----------------------------------
# ------------------------------------------------------------------------------
project(facial_landmark_3d CXX C)

set(CMAKE_CXX_STANDARD 14)  # C++ 14

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(LINK_TYPE STATIC)
else()
    set(LINK_TYPE SHARED)
endif()

# Print make commands for debug
# set(CMAKE_VERBOSE_MAKEFILE 1)

# Set default build type
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Export `compile_commands.json`
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Warning options
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(warning_options "-Wall -Wextra -Wconversion")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    set(warning_options "-Wall -Wextra -Wcast-align -Wcast-qual \
                         -Wctor-dtor-privacy -Wdisabled-optimization \
                         -Wformat=2 -Winit-self \
                         -Wmissing-declarations -Wmissing-include-dirs \
                         -Woverloaded-virtual -Wredundant-decls -Wshadow \
                         -Wsign-conversion -Wsign-promo \
                         -Wstrict-overflow=5 -Wundef -Wno-unknown-pragmas")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(warning_options "--pedantic -Wall -Wextra -Wcast-align -Wcast-qual \
                         -Wctor-dtor-privacy -Wdisabled-optimization \
                         -Wformat=2 -Winit-self -Wlogical-op \
                         -Wmissing-declarations -Wmissing-include-dirs \
                         -Wnoexcept -Woverloaded-virtual \
                         -Wredundant-decls -Wshadow -Wsign-conversion \
                         -Wsign-promo -Wstrict-null-sentinel \
                         -Wstrict-overflow=5 -Wundef -Wno-unknown-pragmas")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(warning_options "/W4")
else()
    message(WARNING "Unsupported compiler for warning options")
    message ("CMAKE_CXX_COMPILER_ID is ${CMAKE_CXX_COMPILER_ID}")
endif()

# Utility function to setup a target (include, link, warning, sanitizer)
function(setup_target target includes libs)
    target_include_directories(${target} PUBLIC ${includes})
    target_link_libraries(${target} ${libs})
    set_target_properties(${target} PROPERTIES COMPILE_FLAGS ${warning_options})
endfunction(setup_target)

# Utility function to setup a target simply (include, link)
function(setup_target_simple target includes libs)
    target_include_directories(${target} PUBLIC ${includes})
    target_link_libraries(${target} ${libs})
endfunction(setup_target_simple)

# ------------------------------------------------------------------------------
# --------------------------- Third Party Libraries ----------------------------
# ------------------------------------------------------------------------------
# Vulkan Wrapper
set(VKW_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third_party/VulkanWrapper)
set(VKW_BUILD_EXAMPLES OFF CACHE BOOL "")
set(VKW_LINK_DYNAMIC_VK ON CACHE BOOL "")
add_subdirectory(${VKW_PATH})
list(APPEND FACELMK3D_INCLUDE ${VKW_INCLUDE})
list(APPEND FACELMK3D_LIBRARY ${VKW_LIBRARY})
list(APPEND FACELMK3D_DEFINE ${VKW_DEFINE})

# GLM
list(APPEND FACELMK3D_INCLUDE ${VKW_PATH}/third_party/glm)

# TinyObjLoader
add_library(tinyobjloader ${VKW_PATH}/third_party/tinyobjloader/tiny_obj_loader.cc)
list(APPEND FACELMK3D_LIBRARY tinyobjloader)

# STB
add_library(stb ${VKW_PATH}/example/utils/stb_impl.cpp)
setup_target_simple(stb "${FACELMK3D_INCLUDE}" "${FACELMK3D_LIBRARY}")
list(APPEND FACELMK3D_LIBRARY stb)

# ------------------------------------------------------------------------------
# ------------------------------- Main Internal --------------------------------
# ------------------------------------------------------------------------------
add_definitions(${FACELMK3D_DEFINE})
add_executable(face_lmk_3d
               ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
setup_target(face_lmk_3d "${FACELMK3D_INCLUDE}" "${FACELMK3D_LIBRARY}")
