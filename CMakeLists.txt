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

# Hot-reloading option: default ON for Debug, OFF for Release builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR NOT CMAKE_BUILD_TYPE)
  set(DEFAULT_HOT_RELOAD ON)
else()
  set(DEFAULT_HOT_RELOAD OFF)
endif()

option(OPENREQUEST_ENABLE_HOT_RELOAD "Enable UI hot-reloading (Debug mode only)" ${DEFAULT_HOT_RELOAD})

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependencies
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

# Glaze JSON library: try system / vcpkg package first; fallback to ext/glaze or FetchContent
find_package(glaze CONFIG QUIET)
if(NOT TARGET glaze::glaze)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ext/glaze/CMakeLists.txt")
    add_subdirectory(ext/glaze EXCLUDE_FROM_ALL)
  else()
    include(FetchContent)
    FetchContent_Declare(
      glaze
      GIT_REPOSITORY https://github.com/stephenberry/glaze.git
      GIT_TAG v9.0.0
    )
    FetchContent_MakeAvailable(glaze)
  endif()
endif()


# Binary output directories (keep executable and shared libraries together)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

# Set RPATH so executable finds sibling shared libraries ($ORIGIN)
set(CMAKE_INSTALL_RPATH "$ORIGIN")
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

if(MSVC)
  add_compile_options(/utf-8)
endif()

# Windows Application Resource (Executable Icon)
set(APP_RESOURCES "")
if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/openrequest.rc")
  set(APP_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/assets/openrequest.rc")
endif()

# ---------------------------------------------------------------------------
# 1. Core Shared Library (Domain, State, Networking, Serialization)
# ---------------------------------------------------------------------------
add_library(openrequest_core SHARED
  src/core/collection.cpp
  src/core/app_state.cpp
  src/core/resource/http_resource.cpp
)

set_target_properties(openrequest_core PROPERTIES
  WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_include_directories(openrequest_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/resource
  ${CMAKE_CURRENT_SOURCE_DIR}/${IMGUI_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/${IMGUI_COLOR_TEXT_EDIT_DIR}
)

target_compile_definitions(openrequest_core PUBLIC
  NOMINMAX
  WIN32_LEAN_AND_MEAN
  OPENREQUEST_CORE_BUILD
)

target_link_libraries(openrequest_core PUBLIC
  CURL::libcurl
  glaze::glaze
)

# ---------------------------------------------------------------------------
# 2. UI Dynamic Library (SDL3, SDL_GPU, ImGui, Rendering, Widgets)
# ---------------------------------------------------------------------------
add_library(openrequest_ui SHARED
  src/ui/ui_entry.cpp
  src/ui/tab.cpp
  src/ui/sidebar.cpp
  src/ui/stb_image.cpp
  src/ui/http_resource_ui.cpp

  ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
  ${IMGUI_DIR}/backends/imgui_impl_sdlgpu3.cpp
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
  ${IMGUI_DIR}/misc/freetype/imgui_freetype.cpp
  ${IMGUI_COLOR_TEXT_EDIT_DIR}/TextEditor.cpp
)

set_target_properties(openrequest_ui PROPERTIES
  WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_include_directories(openrequest_ui PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/resource
  ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
  ${CMAKE_CURRENT_SOURCE_DIR}/ext/cr
  ${CMAKE_CURRENT_SOURCE_DIR}/${IMGUI_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/${IMGUI_DIR}/backends
  ${CMAKE_CURRENT_SOURCE_DIR}/${IMGUI_DIR}/misc/cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/${IMGUI_COLOR_TEXT_EDIT_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/assets
)

target_compile_definitions(openrequest_ui PUBLIC
  NOMINMAX
  WIN32_LEAN_AND_MEAN
  OPENREQUEST_UI_BUILD
  FONT_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/ext/imgui/misc/fonts/Roboto-Medium.ttf\"
  MONO_FONT_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/ext/imgui/misc/fonts/Cousine-Regular.ttf\"
  ASSETS_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/assets/\"
  EDITOR_FONT_SIZE=16
)

target_link_libraries(openrequest_ui PUBLIC
  openrequest_core
  SDL3::SDL3
  Freetype::Freetype
)

if(WIN32)
  target_link_libraries(openrequest_ui PRIVATE dwmapi)
endif()

# ---------------------------------------------------------------------------
# 3. Host Executable (Entry point, Window/GPU supervisor & Hot-Reloader)
# ---------------------------------------------------------------------------
add_executable(openrequest
  src/app/main.cpp
  ${APP_RESOURCES}
  ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
  ${IMGUI_DIR}/backends/imgui_impl_sdlgpu3.cpp
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
  ${IMGUI_DIR}/misc/freetype/imgui_freetype.cpp
)

target_include_directories(openrequest PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/resource
  ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
  ${CMAKE_CURRENT_SOURCE_DIR}/src/app
  ${CMAKE_CURRENT_SOURCE_DIR}/ext/cr
  ${CMAKE_CURRENT_SOURCE_DIR}/${IMGUI_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/${IMGUI_DIR}/backends
  ${CMAKE_CURRENT_SOURCE_DIR}/${IMGUI_DIR}/misc/cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/assets
)

target_compile_definitions(openrequest PRIVATE
  NOMINMAX
  WIN32_LEAN_AND_MEAN
  FONT_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/ext/imgui/misc/fonts/Roboto-Medium.ttf\"
  MONO_FONT_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/ext/imgui/misc/fonts/Cousine-Regular.ttf\"
  ASSETS_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/assets/\"
  EDITOR_FONT_SIZE=16
)

target_link_libraries(openrequest PRIVATE
  openrequest_core
  SDL3::SDL3
  Freetype::Freetype
  ${CMAKE_DL_LIBS}
)

if(OPENREQUEST_ENABLE_HOT_RELOAD)
  target_compile_definitions(openrequest PRIVATE OPENREQUEST_ENABLE_HOT_RELOAD=1)
else()
  target_compile_definitions(openrequest PRIVATE OPENREQUEST_ENABLE_HOT_RELOAD=0)
  target_link_libraries(openrequest PRIVATE openrequest_ui)
endif()

if(WIN32)
  target_link_libraries(openrequest PRIVATE dwmapi)
endif()

# Ensure openrequest_ui is compiled whenever openrequest is built
add_dependencies(openrequest openrequest_ui)

# Copy assets next to executable in build directory
add_custom_command(TARGET openrequest POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_CURRENT_SOURCE_DIR}/assets
  $<TARGET_FILE_DIR:openrequest>/assets
)

# Installation
include(GNUInstallDirs)

install(TARGETS openrequest openrequest_ui openrequest_core
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
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
