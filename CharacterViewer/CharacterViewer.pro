INCLUDEPATH += include
HEADERS = include/SceneViewport.h \
    include/RenderState.h include/RenderStateGL2.h \
    include/Mesh.h include/Material.h include/Vertex.h \
    include/Scene.h include/MeshGL2.h \
    include/Platform.h \
    include/WLDData.h \
    include/WLDFragment.h \
    include/Fragments.h \
    include/WLDModel.h \
    include/StreamReader.h \
    include/PFSArchive.h \
    include/WLDActor.h \
    include/Zone.h \
    include/WLDSkeleton.h \
    include/CharacterViewerWindow.h

SOURCES = src/main.cpp src/SceneViewport.cpp \
    src/RenderState.cpp src/RenderStateGL2.cpp \
    src/Mesh.cpp src/Material.cpp src/Vertex.cpp \
    src/Scene.cpp src/MeshGL2.cpp \
    src/WLDData.cpp \
    src/WLDFragment.cpp \
    src/Fragments.cpp \
    src/WLDModel.cpp \
    src/StreamReader.cpp \
    src/PFSArchive.cpp \
    src/WLDActor.cpp \
    src/Zone.cpp \
    src/WLDSkeleton.cpp \
    src/CharacterViewerWindow.cpp
LIBS += -lm -lz

TARGET = CharacterViewer
CONFIG += qt warn_on debug thread
QT += opengl

OTHER_FILES += \
    fragment.glsl \
    vertex.glsl
