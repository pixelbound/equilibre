cmake_minimum_required(VERSION 2.8)

project(OpenEQ)

find_package(Qt4 4.6.0 COMPONENTS QtCore QtGui QtOpenGL REQUIRED)
find_package(ZLIB 1.0.0 REQUIRED)
include(${QT_USE_FILE})
include_directories(${ZLIB_INCLUDE_DIRS})

if( WIN32 )
    set(GLEW_LIB_PATH "" CACHE PATH "Path to the GLEW library directory.")
    set(GLEW_INCLUDE_PATH "" CACHE PATH "Path to the GLEW include directory.")
    if(NOT ${GLEW_LIB_PATH} STREQUAL "" AND EXISTS "${GLEW_LIB_PATH}/glew32.lib")
      set(GLEW_LIBRARIES "${GLEW_LIB_PATH}/glew32.lib" OpenGL32.lib)
    else()
      message(FATAL_ERROR "The path to the GLEW library directory must be set.")
    endif()
    if(NOT ${GLEW_INCLUDE_PATH} STREQUAL "" AND EXISTS ${GLEW_INCLUDE_PATH}/GL/glew.h)
      include_directories(${GLEW_INCLUDE_PATH})
    else()
      message(FATAL_ERROR "The path to the GLEW include directory must be set.")
    endif()
else( WIN32 )
    set(GLEW_LIBRARIES GLEW GL)
endif( WIN32 )

if(WIN32)
    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/OpenEQ/Platform/win32")
    add_definitions(/DM_PI=3.14159265358979323846)
endif(WIN32)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")

subdirs(lib)
subdirs(tools)
