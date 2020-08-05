cmake_minimum_required(VERSION 3.0)
cmake_policy(VERSION 3.0)

project (OpenGL_VSM)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules")

if (WIN32)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
endif (WIN32)

link_directories(${CMAKE_SOURCE_DIR}/lib)

find_package(GLM REQUIRED)
message(STATUS "GLM included at ${GLM_INCLUDE_DIR}")

find_package(GLFW3 REQUIRED)
message(STATUS "Found GLFW3 in ${GLFW3_INCLUDE_DIR}")

if (WIN32)
    set(LIBS glfw3 opengl32)
elseif(APPLE)
    INCLUDE_DIRECTORIES(/System/Library/Frameworks)
    FIND_LIBRARY(COCOA_LIBRARY Cocoa)
    FIND_LIBRARY(OpenGL_LIBRARY OpenGL)
    FIND_LIBRARY(IOKit_LIBRARY IOKit)
    FIND_LIBRARY(CoreVideo_LIBRARY CoreVideo)
    MARK_AS_ADVANCED(COCOA_LIBRARY OpenGL_LIBRARY)
    set(APPLE_LIBS ${COCOA_LIBRARY} ${IOKit_LIBRARY} ${OpenGL_LIBRARY} ${CoreVideo_LIBRARY})
    set(APPLE_LIBS ${APPLE_LIBS} ${GLFW3_LIBRARY})
    set(LIBS ${LIBS} ${APPLE_LIBS})
endif (WIN32)

include_directories(${CMAKE_SOURCE_DIR}/includes)

#create static libraries
add_library(STB_IMAGE "src/stb_image.cpp")
set(LIBS ${LIBS} STB_IMAGE)

add_library(GLAD "src/glad.c")
set(LIBS ${LIBS} GLAD)

set(SOURCE "src/OpenGL_VSM.cpp")
set(NAME "OpenGL_VSM")
add_executable(${NAME} ${SOURCE})
target_link_libraries(${NAME} ${LIBS})

file(GLOB SHADERS
    "src/shaders/*.vert"
    "src/shaders/*.frag"
)

foreach(SHADER ${SHADERS})
    if (WIN32)
        file(COPY ${SHADER} DESTINATION ${CMAKE_SOURCE_DIR}/bin/Debug)
    elseif (APPLE)
        file(COPY ${SHADER} DESTINATION ${CMAKE_SOURCE_DIR}/build)
    endif (WIN32)
endforeach(SHADER)