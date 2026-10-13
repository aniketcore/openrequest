cmake_minimum_required(VERSION 3.22...4.2)

# If CMAKE_TOOLCHAIN_FILE is not specified, auto-detect VCPKG_ROOT from environment if present
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE FILEPATH "vcpkg toolchain file")
endif()

project(openrequest C CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependencies
find_package(Vulkan REQUIRED)
find_package(Freetype REQUIRED)
find_package(CURL REQUIRED)

# GLFW: try system / vcpkg package first; fallback to vendored submodule
find_package(glfw3 CONFIG QUIET)
if(NOT TARGET glfw AND NOT TARGET glfw3::glfw)
  if(NOT GLFW_DIR)
    set(GLFW_DIR ext/glfw)
  endif()
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${GLFW_DIR}/CMakeLists.txt")
    option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
    option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
    option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
    option(GLFW_INSTALL "Generate installation target" OFF)
    option(GLFW_DOCUMENT_INTERNALS "Include internals in documentation" OFF)
    add_subdirectory(${GLFW_DIR} binary_dir EXCLUDE_FROM_ALL)
  else()
    find_package(glfw3 REQUIRED)
  endif()
endif()

# Normalize GLFW target name
if(TARGET glfw3::glfw)
  set(GLFW_TARGET glfw3::glfw)
else()
  set(GLFW_TARGET glfw)
endif()

# Dear ImGui & ImGuiColorTextEdit
set(IMGUI_DIR ext/imgui)
set(IMGUI_COLOR_TEXT_EDIT_DIR ext/ImGuiColorTextEdit)

# Gather all source files
file(GLOB_RECURSE sources CONFIGURE_DEPENDS src/*.cpp src/**/*.cpp)
list(FILTER sources EXCLUDE REGEX "gtk_glarea_demo\\.cpp$")

add_executable(openrequest
  ${sources}
  ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
  ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
  ${IMGUI_DIR}/misc/freetype/imgui_freetype.cpp
  ${IMGUI_COLOR_TEXT_EDIT_DIR}/TextEditor.cpp
)

target_include_directories(openrequest PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
  ${CMAKE_CURRENT_SOURCE_DIR}/src/app
  ${CMAKE_CURRENT_SOURCE_DIR}/${IMGUI_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/${IMGUI_DIR}/backends
  ${CMAKE_CURRENT_SOURCE_DIR}/${IMGUI_DIR}/misc/cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/${IMGUI_COLOR_TEXT_EDIT_DIR}
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${GLFW_DIR}/include")
  target_include_directories(openrequest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/${GLFW_DIR}/include)
endif()

target_compile_definitions(openrequest PRIVATE
  VK_PROTOTYPES
  FONT_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/ext/imgui/misc/fonts/Roboto-Medium.ttf\"
  MONO_FONT_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/ext/imgui/misc/fonts/Cousine-Regular.ttf\"
  EDITOR_FONT_SIZE=16
)

if(MSVC)
  target_compile_options(openrequest PRIVATE /utf-8)
endif()

target_link_libraries(openrequest PRIVATE
  ${GLFW_TARGET}
  Vulkan::Vulkan
  CURL::libcurl
  Freetype::Freetype
)

if(UNIX AND NOT APPLE)
  find_package(X11 QUIET)
  if(X11_FOUND)
    target_include_directories(openrequest PRIVATE ${X11_INCLUDE_DIR})
    target_link_libraries(openrequest PRIVATE ${X11_LIBRARIES})
  endif()
endif()
