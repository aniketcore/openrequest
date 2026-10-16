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

# SDL3: try system / vcpkg package first; fallback to FetchContent
find_package(SDL3 CONFIG QUIET)
if(NOT TARGET SDL3::SDL3)
  include(FetchContent)
  FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.4
  )
  FetchContent_MakeAvailable(SDL3)
endif()


# Dear ImGui & ImGuiColorTextEdit
set(IMGUI_DIR ext/imgui)
set(IMGUI_COLOR_TEXT_EDIT_DIR ext/ImGuiColorTextEdit)

# Gather all source files
file(GLOB_RECURSE sources CONFIGURE_DEPENDS src/*.cpp src/**/*.cpp)
list(FILTER sources EXCLUDE REGEX "gtk_glarea_demo\\.cpp$")

# Windows Application Resource (Executable Icon)
set(APP_RESOURCES "")
if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/openrequest.rc")
  set(APP_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/assets/openrequest.rc")
endif()

add_executable(openrequest
  ${sources}
  ${APP_RESOURCES}
  ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
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
  ${CMAKE_CURRENT_SOURCE_DIR}/assets
)

target_compile_definitions(openrequest PRIVATE
  NOMINMAX
  WIN32_LEAN_AND_MEAN
  VK_PROTOTYPES
  FONT_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/ext/imgui/misc/fonts/Roboto-Medium.ttf\"
  MONO_FONT_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/ext/imgui/misc/fonts/Cousine-Regular.ttf\"
  ASSETS_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/assets/\"
  EDITOR_FONT_SIZE=16
)

# Copy assets next to executable in build directory
add_custom_command(TARGET openrequest POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_CURRENT_SOURCE_DIR}/assets
  $<TARGET_FILE_DIR:openrequest>/assets
)

if(MSVC)
  target_compile_options(openrequest PRIVATE /utf-8)
endif()

target_link_libraries(openrequest PRIVATE
  SDL3::SDL3
  Vulkan::Vulkan
  CURL::libcurl
  Freetype::Freetype
)

if(WIN32)
  target_link_libraries(openrequest PRIVATE dwmapi)
endif()

# Installation
include(GNUInstallDirs)

install(TARGETS openrequest
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(UNIX AND NOT APPLE)
  install(FILES assets/openrequest.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
  )
  install(FILES assets/icon.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
    RENAME openrequest.svg
  )
  install(FILES assets/icon.png
    DESTINATION ${CMAKE_INSTALL_DATADIR}/pixmaps
    RENAME openrequest.png
  )
endif()

# Uninstall target
if(NOT TARGET uninstall)
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake.in" [=[
if(NOT EXISTS "@CMAKE_BINARY_DIR@/install_manifest.txt")
  message(FATAL_ERROR "Cannot find install manifest: @CMAKE_BINARY_DIR@/install_manifest.txt")
endif()

file(STRINGS "@CMAKE_BINARY_DIR@/install_manifest.txt" files)
foreach(file ${files})
  message(STATUS "Uninstalling $ENV{DESTDIR}${file}")
  if(IS_SYMLINK "$ENV{DESTDIR}${file}" OR EXISTS "$ENV{DESTDIR}${file}")
    execute_process(
      COMMAND "@CMAKE_COMMAND@" -E rm -f "$ENV{DESTDIR}${file}"
      RESULT_VARIABLE rm_retval
    )
    if(NOT "${rm_retval}" STREQUAL 0)
      message(FATAL_ERROR "Problem when removing $ENV{DESTDIR}${file}")
    endif()
  else()
    message(STATUS "File $ENV{DESTDIR}${file} does not exist.")
  endif()
endforeach()
]=])

  configure_file(
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    @ONLY
  )

  add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  )
endif()
