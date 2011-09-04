find_package(Qt4 4.6.0 COMPONENTS QtCore QtGui QtOpenGL REQUIRED)
find_package(ZLIB 1.0.0 REQUIRED)
include(${QT_USE_FILE})
include_directories(${ZLIB_INCLUDE_DIRS})

if( WIN32 )
    set(GLEW_LIBRARIES glew32)
else( WIN32 )
    set(GLEW_LIBRARIES GLEW)
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

QT4_WRAP_CPP(CHARVIEWER_MOC_SOURCES ${CHARVIEWER_HEADERS})
QT4_ADD_RESOURCES(CHARVIEWER_RESOURCES ${CHARVIEWER_RESOURCES})

include_directories( include )

add_executable(CharacterViewer
    ${CHARVIEWER_SOURCES}
    ${CHARVIEWER_MOC_SOURCES}
    ${CHARVIEWER_RESOURCES}
)
target_link_libraries(CharacterViewer ${QT_LIBRARIES} ${GLEW_LIBRARIES} ${ZLIB_LIBRARIES})
