cmake_minimum_required(VERSION 2.8)
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
    set(GLEW_LIBRARIES GLEW GL GLU)
endif( WIN32 )

set(CHARVIEWER_SOURCES
    src/main.cpp
    src/SceneViewport.cpp
    src/RenderState.cpp
    src/RenderStateGL2.cpp
    src/ShaderProgramGL2.cpp
    src/Material.cpp
    src/Vertex.cpp
    src/Scene.cpp
    src/WLDData.cpp
    src/WLDFragment.cpp
    src/Fragments.cpp
    src/WLDModel.cpp
    src/StreamReader.cpp
    src/PFSArchive.cpp
    src/WLDActor.cpp
    src/Zone.cpp
    src/WLDSkeleton.cpp
    src/CharacterViewerWindow.cpp
    src/dxt.c src/mipmap.c
    src/Platform.cpp
    src/ZoneViewerWindow.cpp
)

set(CHARVIEWER_HEADERS
    include/SceneViewport.h
    include/RenderState.h
    include/RenderStateGL2.h
    include/ShaderProgramGL2.h
    include/Material.h
    include/Vertex.h
    include/Scene.h
    include/Platform.h
    include/WLDData.h
    include/WLDFragment.h
    include/Fragments.h
    include/WLDModel.h
    include/StreamReader.h
    include/PFSArchive.h
    include/WLDActor.h
    include/Zone.h
    include/WLDSkeleton.h
    include/CharacterViewerWindow.h
    include/imath.h
    include/dxt_tables.h
    include/dds.h
    include/dxt.h
    include/ZoneViewerWindow.h
)

set(CHARVIEWER_RESOURCES
    resources.qrc
)

set(CHARVIEWER_RESOURCE_FILES
    fragment.glsl
    vertex.glsl
    vertex_skinned_texture.glsl
    vertex_skinned_uniform.glsl
)

QT4_WRAP_CPP(CHARVIEWER_MOC_SOURCES ${CHARVIEWER_HEADERS})
QT4_ADD_RESOURCES(CHARVIEWER_RESOURCES ${CHARVIEWER_RESOURCES})

include_directories(include)

if(WIN32)
    set(CHARVIEWER_HEADERS ${CHARVIEWER_HEADERS} win32/inttypes.h win32/stdint.h)
    include_directories(include/win32)
    add_definitions(/DM_PI=3.14159265358979323846)
endif(WIN32)

add_executable(CharacterViewer
    ${CHARVIEWER_SOURCES}
    ${CHARVIEWER_MOC_SOURCES}
    ${CHARVIEWER_RESOURCES}
    ${CHARVIEWER_RESOURCE_FILES}
    ${CHARVIEWER_HEADERS}
)
target_link_libraries(CharacterViewer ${QT_LIBRARIES} ${GLEW_LIBRARIES} ${ZLIB_LIBRARIES})
