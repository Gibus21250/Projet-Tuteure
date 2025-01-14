cmake_minimum_required(VERSION 3.30)
project(IFSOpenGL)
set(CMAKE_CXX_STANDARD 20)

set(SHADER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SHADER_BUILD_DIR ${CMAKE_BINARY_DIR}/shaders)

add_subdirectory(libs/glfw)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

add_executable(IFSOpenGL main.cpp shader.cpp shader.hpp)



# Copier les shaders
add_custom_command(TARGET IFSOpenGL POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${SHADER_SOURCE_DIR} ${SHADER_BUILD_DIR}
)

include_directories(
        libs/glm
        libs/glfw/include)

target_link_libraries(IFSOpenGL PRIVATE glfw ${GLEW_LIBRARIES} OpenGL)
