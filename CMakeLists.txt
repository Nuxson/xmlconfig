cmake_minimum_required(VERSION 3.15)
project(xmlconfig)

set(CMAKE_CXX_STANDARD 17)

# Ищем GLFW вручную
find_path(GLFW_INCLUDE_DIR NAMES GLFW/glfw3.h 
    PATHS 
    "C:/Users/nxs/Documents/Developers/vcpkg/installed/x64-windows/include"
    "C:/Users/nxs/Documents/Developers/vcpkg/packages/glfw3_x64-windows/include"
)

find_library(GLFW_LIBRARY NAMES glfw3 glfw3dll
    PATHS
    "C:/Users/nxs/Documents/Developers/vcpkg/installed/x64-windows/lib"
    "C:/Users/nxs/Documents/Developers/vcpkg/packages/glfw3_x64-windows/lib"
)

if(GLFW_INCLUDE_DIR AND GLFW_LIBRARY)
    message("Found GLFW: ${GLFW_INCLUDE_DIR}")
else()
    message(FATAL_ERROR "GLFW not found!")
endif()

find_package(OpenGL REQUIRED)

# ImGui
add_library(imgui
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_demo.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_glfw.cpp
    third_party/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    third_party/imgui
    third_party/imgui/backends
    ${GLFW_INCLUDE_DIR}
)

target_link_libraries(imgui PUBLIC OpenGL::GL ${GLFW_LIBRARY})

# TinyXML2
add_library(tinyxml2
    third_party/tinyxml2/tinyxml2.cpp
)
target_include_directories(tinyxml2 PUBLIC third_party/tinyxml2)

# Основное приложение
add_executable(xmlconfig
    src/main.cpp
    src/xml_parser.cpp
    src/network_tools.cpp
    src/network_ui.cpp
    src/profiles.cpp
    src/shortcuts.cpp
    src/tray_win.cpp
)

target_include_directories(xmlconfig PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(xmlconfig
    imgui
    tinyxml2
    OpenGL::GL
    ${GLFW_LIBRARY}
)

if(MSVC)
    add_compile_options(/W4 /permissive-)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()