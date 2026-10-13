cmake_minimum_required(VERSION 3.10)
project(openrequest C CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 23)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVK_PROTOTYPES")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_PROTOTYPES")

# GLFW
if(NOT GLFW_DIR)
   set(GLFW_DIR ext/glfw) # Set this to point to an up-to-date GLFW repo
endif()
option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_INSTALL "Generate installation target" OFF)
option(GLFW_DOCUMENT_INTERNALS "Include internals in documentation" OFF)
add_subdirectory(${GLFW_DIR} binary_dir EXCLUDE_FROM_ALL)
include_directories(${GLFW_DIR}/include)

# Dear ImGui
set(IMGUI_DIR ext/imgui)
include_directories(${IMGUI_DIR} ${IMGUI_DIR}/backends ..)
# ImGuiColorTextEdit
set(IMGUI_COLOR_TEXT_EDIT_DIR ext/ImGuiColorTextEdit)
include_directories(${IMGUI_COLOR_TEXT_EDIT_DIR})

# Libraries
find_package(Vulkan REQUIRED)
find_package(Freetype REQUIRED)
find_package(CURL REQUIRED)
set(LIBRARIES "glfw;Vulkan::Vulkan" CURL::libcurl Freetype::Freetype)

if(WIN32)
  add_compile_definitions(NOMINMAX)
  list(APPEND LIBRARIES user32)
endif()

if(UNIX AND NOT APPLE)
  find_package(X11)
  if(X11_FOUND)
    include_directories(${X11_INCLUDE_DIR})
    list(APPEND LIBRARIES ${X11_LIBRARIES})
  endif()
endif()

# Use vulkan headers from glfw:
include_directories(${GLFW_DIR}/deps ${IMGUI_DIR}/misc/cpp/ ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/ui ${CMAKE_SOURCE_DIR}/src/app)

# Gather all source files (modified to trigger re-run for new files)
file(GLOB_RECURSE sources CONFIGURE_DEPENDS src/*.cpp src/**/*.cpp)
list(FILTER sources EXCLUDE REGEX "gtk_glarea_demo\\.cpp$")

add_executable(openrequest ${sources} ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp ${IMGUI_DIR}/imgui.cpp ${IMGUI_DIR}/imgui_draw.cpp ${IMGUI_DIR}/imgui_demo.cpp ${IMGUI_DIR}/imgui_tables.cpp ${IMGUI_DIR}/imgui_widgets.cpp ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp ${IMGUI_DIR}/misc/freetype/imgui_freetype.cpp ${IMGUI_COLOR_TEXT_EDIT_DIR}/TextEditor.cpp)
target_link_libraries(openrequest ${LIBRARIES})
target_compile_definitions(openrequest PRIVATE FONT_PATH="${CMAKE_SOURCE_DIR}/ext/imgui/misc/fonts/Roboto-Medium.ttf" MONO_FONT_PATH="${CMAKE_SOURCE_DIR}/ext/imgui/misc/fonts/Cousine-Regular.ttf" EDITOR_FONT_SIZE=16)




