cmake_minimum_required (VERSION 2.6)

SET(GPU_PERF_GLES_MAJOR_VERSION 2)
SET(GPU_PERF_GLES_MINOR_VERSION 0)
SET(GPU_PERF_GLES_PATCH_VERSION 0)
SET(GPU_PERF_GLES_VERSION 200)
SET(GPU_PERF_GLSL_VERSION 100)

set(${PROJECT_NAME} GPU_Perf_GLES_2_0)
project(GPU_Perf_GLES_2_0)

option(ENABLE_ASAN "Enable Address Sanitizer for memory leak detect" OFF)

#使用文件夹路径统一设置库路径
if(DEFINED CUSTOM_EGL_PATH)
    add_definitions(-D_GLFW_EGL_LIBRARY="${CUSTOM_EGL_PATH}/libEGL.so")
    add_definitions(-D_GLFW_GLESV2_LIBRARY="${CUSTOM_EGL_PATH}/libGLESv2.so")
    add_definitions(-DPERF_EGL_PATH="${CUSTOM_EGL_PATH}/libEGL.so")
    add_definitions(-DPERF_GLES_PATH="${CUSTOM_EGL_PATH}/libGLESv2.so")
endif ()

#单独设置EGL库路径
if (DEFINED CUSTOM_EGL_FILE)
    add_definitions(-D_GLFW_EGL_LIBRARY="${CUSTOM_EGL_FILE}")
    add_definitions(-DPERF_EGL_PATH="${CUSTOM_EGL_FILE}")
endif ()

#单独设置GLES库路径
if (DEFINED CUSTOM_GLES_FILE)
    add_definitions(-D_GLFW_GLESV2_LIBRARY="${CUSTOM_GLES_FILE}")
    add_definitions(-DPERF_GLES_PATH="${CUSTOM_GLES_FILE}")
endif ()

set(USE_WAY 0)
#设置创建窗口的方式为EGL
if(DEFINED USE_XEGL)
    add_definitions(-DWINDOW_USE_XEGL=1)
    set(USE_WAY 1)
    message("use egl" ${USE_XEGL})
endif ()

#设置创建窗口的方式为GLFW
if(DEFINED USE_GLFW)
    add_definitions(-DWINDOW_USE_GLFW=1)
    set(USE_WAY 2)
    message("use glfw : " ${USE_GLFW})
endif ()

#设置默认创建窗口的方式为EGL
if(${USE_WAY} EQUAL 0)
    add_definitions(-DWINDOW_USE_GLFW=1)
    set(USE_WAY 2)
endif()

#设置自定义的头文件路径
if(DEFINED CUSTOM_INCLUDE_PATH)
    include_directories(${CUSTOM_INCLUDE_PATH})
endif()

#set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/lib)
EXECUTE_PROCESS(COMMAND uname -m
                 COMMAND tr -d '/r/n'
                 OUTPUT_VARIABLE ARCHITECTURE)

message(-- " " STATUS " Architecture: " ${ARCHITECTURE})
if(${ARCHITECTURE} MATCHES "X86|x86" )
    message(-- " " STATUS " Machine is X86 Architecture")
else()
    LINK_DIRECTORIES(/usr/local/lib)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Second, for multi-config builds (e.g. msvc)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/bin )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/lib )
    set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/lib )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

#SET(CMAKE_BUILD_TYPE "Release")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb -pg")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

link_directories(${CMAKE_SOURCE_DIR}/lib)
link_directories(${CMAKE_CURRENT_BINARY_DIR}/lib)

add_definitions(-DASSIMP_BUILD_SAMPLES=OFF)
add_definitions(-DASSIMP_BUILD_TESTS=OFF)
set(ASSIMP_BUILD_SAMPLES, OFF)
set(ASSIMP_BUILD_TESTS, OFF)
add_subdirectory(assimp_5_2_2)
add_subdirectory(freetype_2_12_1)
if(${USE_WAY} EQUAL 2)
    add_subdirectory(glfw_3_3_7)
endif()

set(RUN_DIR ${PROJECT_SOURCE_DIR}/bin)
include_directories(${CMAKE_SOURCE_DIR}/freetype_2_12_1/include)
include_directories(${CMAKE_SOURCE_DIR}/glfw_3_3_7/include)
include_directories(${CMAKE_SOURCE_DIR}/assimp_5_2_2/include)
include_directories(${CMAKE_BINARY_DIR}/freetype_2_12_1/include)
include_directories(${CMAKE_BINARY_DIR}/glfw_3_3_7/include)
include_directories(${CMAKE_BINARY_DIR}/assimp_5_2_2/include)

set(EXAMPLES
   #shader
   fill
   pixel
   depth
   material
   sprite
   skybox
   #fire
   textures
   lights
   #shadowmap
)

include_directories(include)

add_definitions(-DGLSL_PATH="${PROJECT_SOURCE_DIR}/media/")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_LINUX -std=c++11")

macro (add_linker_flags)
    string (REPLACE ";" " " _FLAGS "${ARGV}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${_FLAGS}")
    set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${_FLAGS}")
    set (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${_FLAGS}")
endmacro ()

# Enable Address Sanitizer
if (ENABLE_ASAN)
    add_compiler_flags (-fsanitize=address)
    add_linker_flags (-fsanitize=address)
endif ()

add_library(glad src/glpbcommon/glad.c)
add_library(ftgl
    src/ftgl/distance-field.c
    src/ftgl/edtaa3func.c
    src/ftgl/ftgl-utils.c
    src/ftgl/platform.c
    src/ftgl/texture-atlas.c
    src/ftgl/texture-font.c
    src/ftgl/utf8-utils.c
    src/ftgl/vector.c
    src/ftgl/vertex-attribute.c
    src/ftgl/vertex-buffer.c)

add_library(glpbcommon
    src/glpbcommon/Node.cpp
    src/glpbcommon/stb_image.cpp
    src/glpbcommon/Window.cpp
    src/glpbcommon/Log.cpp
    src/glpbcommon/Texture.cpp
    src/glpbcommon/Model.cpp
    src/glpbcommon/Mesh.cpp
    src/glpbcommon/GLSLProgram.cpp
    src/glpbcommon/TextRender.cpp
    src/glpbcommon/perfeglcontext.cpp
    src/glpbcommon/perfxwindow.cpp
    src/glpbcommon/perftimer.cpp
)

target_link_libraries(glpbcommon ftgl glad freetype)

foreach(EXAMPLE ${EXAMPLES})
  file(GLOB APP_SRC ${CMAKE_SOURCE_DIR}/src/${EXAMPLE}/*.cpp)
  set(EXAMPLE_SRC ${EXAMPLE_SRC} ${APP_SRC})
endforeach(EXAMPLE)

add_executable(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/src/main.cpp ${EXAMPLE_SRC})

target_link_libraries(${PROJECT_NAME} glpbcommon ftgl pthread freetype assimp glad m rt dl X11)

if(${USE_WAY} EQUAL 2)
    target_link_libraries(${PROJECT_NAME} glfw)
endif()

ADD_CUSTOM_TARGET(debug ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE:STRING=Debug ${PROJECT_SOURCE_DIR})
ADD_CUSTOM_TARGET(release ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE:STRING=Release ${PROJECT_SOURCE_DIR})
