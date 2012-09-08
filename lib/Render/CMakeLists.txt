set(LIB_SOURCES
    dxt.c
    FrameStat.cpp
    Geometry.cpp
    LinearMath.cpp
    Material.cpp
    mipmap.c
    Platform.cpp
    RenderStateGL2.cpp
    Scene.cpp
    SceneViewport.cpp
    ShaderProgramGL2.cpp
    Vertex.cpp
)

set(LIB_HEADERS
    ../../include/EQuilibre/Render/SceneViewport.h
    ../../include/EQuilibre/Render/RenderState.h
    ../../include/EQuilibre/Render/ShaderProgramGL2.h
    ../../include/EQuilibre/Render/Material.h
    ../../include/EQuilibre/Render/Vertex.h
    ../../include/EQuilibre/Render/Geometry.h
    ../../include/EQuilibre/Render/LinearMath.h
    ../../include/EQuilibre/Render/Scene.h
    ../../include/EQuilibre/Render/FrameStat.h
    ../../include/EQuilibre/Render/Platform.h
    ../../include/EQuilibre/Render/imath.h
    ../../include/EQuilibre/Render/dxt_tables.h
    ../../include/EQuilibre/Render/dds.h
    ../../include/EQuilibre/Render/dxt.h
)

set(LIB_RESOURCES
    resources.qrc
)

set(LIB_RESOURCE_FILES
    fragment.glsl
    vertex.glsl
    vertex_skinned_texture.glsl
    vertex_skinned_uniform.glsl
)

QT4_WRAP_CPP(LIB_MOC_SOURCES ${LIB_HEADERS})
QT4_ADD_RESOURCES(LIB_RESOURCES ${LIB_RESOURCES})

source_group(shaders FILES ${LIB_RESOURCE_FILES})
source_group(moc FILES ${LIB_MOC_SOURCES})

add_definitions(-DBUILD_RENDER_DLL)

add_library(EQuilibreRender SHARED
    ${LIB_SOURCES}
    ${LIB_MOC_SOURCES}
    ${LIB_RESOURCES}
    ${LIB_RESOURCE_FILES}
    ${LIB_HEADERS}
)
target_link_libraries(EQuilibreRender
    ${QT_LIBRARIES}
    ${GLEW_LIBRARIES}
    ${SYSTEM_LIBRARIES}
)
