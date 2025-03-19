cmake_minimum_required(VERSION 3.20)
project(IFSOpenGL)
set(CMAKE_CXX_STANDARD 20)

set(SHADER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SHADER_BUILD_DIR ${CMAKE_BINARY_DIR}/shaders)

set(CUSTOM_GLEW_PATH "J:/Developpement/SDK/glew-2.2.0")

option(CUSTOM_GLEW_PATH "Chemin vers les bibliothèques GLEW personnalisées" "")

add_subdirectory(libs/glfw)

find_package(OpenGL REQUIRED)
find_package(GLEW QUIET)

if (NOT GLEW_FOUND)
    if (CUSTOM_GLEW_PATH)
        message(STATUS "GLEW non trouvé, utilisation du chemin personnalisé : ${CUSTOM_GLEW_PATH}")

        # Ajouter le chemin des headers et des bibliothèques
        include_directories(${CUSTOM_GLEW_PATH}/include)
        set(GLEW_LIBRARIES ${CUSTOM_GLEW_PATH}/lib/Release/x64/glew32.lib)
    else()
        message(FATAL_ERROR "GLEW non trouvé et aucun chemin personnalisé spécifié. Utilisez -DCUSTOM_GLEW_PATH pour définir un chemin.")
    endif()
endif()

include_directories(
        libs/glm
        libs/glfw/include)

if (WIN32)
    link_libraries(opengl32)
else ()
    link_libraries(GL)
endif()

link_libraries(glfw ${GLEW_LIBRARIES})

add_executable(IFSOpenGL main.cpp
        shader.cpp
        shader.hpp
        Automaton.cpp
        Automaton.h)

add_executable(test
        test.cpp
        Automaton.cpp
        Automaton.h
)

add_executable(testgpu
        testgpu.cpp
        shader.cpp
        shader.hpp
        Automaton.cpp
        Automaton.h
)

# Copier les shaders
#add_custom_command(TARGET IFSOpenGL POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy_directory ${SHADER_SOURCE_DIR} ${SHADER_BUILD_DIR}
#)