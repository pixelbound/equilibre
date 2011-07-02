INCLUDEPATH += include
HEADERS = include/SceneViewport.h \
    include/RenderState.h include/RenderStateGL2.h include/ShaderProgramGL2.h \
    include/Material.h include/Vertex.h \
    include/Scene.h \
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
    include/CharacterViewerWindow.h \
    include/imath.h include/dxt_tables.h include/dds.h include/dxt.h

SOURCES = src/main.cpp src/SceneViewport.cpp \
    src/RenderState.cpp src/RenderStateGL2.cpp src/ShaderProgramGL2.cpp \
    src/Material.cpp src/Vertex.cpp \
    src/Scene.cpp \
    src/WLDData.cpp \
    src/WLDFragment.cpp \
    src/Fragments.cpp \
    src/WLDModel.cpp \
    src/StreamReader.cpp \
    src/PFSArchive.cpp \
    src/WLDActor.cpp \
    src/Zone.cpp \
    src/WLDSkeleton.cpp \
    src/CharacterViewerWindow.cpp \
    src/dxt.c src/mipmap.c \
    src/Platform.cpp

win32 {
    INCLUDEPATH += ../glew-1.5.4-mingw32/include ../zlib125-dll/include
    LIBS += -L../glew-1.5.4-mingw32/lib -lglew32 -L../zlib125-dll/lib -lzdll
}
else {
    LIBS += -lm -lz -lGLEW
}

TARGET = CharacterViewer
CONFIG += qt warn_on debug thread console
QT += opengl

OTHER_FILES += \
    fragment.glsl \
    vertex.glsl vertex_skinned_uniform.glsl vertex_skinned_texture.glsl

RESOURCES += \
    resources.qrc
