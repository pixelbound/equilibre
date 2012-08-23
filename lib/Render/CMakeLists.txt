set(LIB_SOURCES
    dxt.c
    FrameStat.cpp
    Geometry.cpp
    LinearMath.cpp
    Material.cpp
    mipmap.c
    Platform.cpp
    RenderState.cpp
    RenderStateGL2.cpp
    Scene.cpp
    SceneViewport.cpp
    ShaderProgramGL2.cpp
    Vertex.cpp
)

set(LIB_HEADERS
    ../../include/OpenEQ/Render/SceneViewport.h
    ../../include/OpenEQ/Render/RenderState.h
    ../../include/OpenEQ/Render/RenderStateGL2.h
    ../../include/OpenEQ/Render/ShaderProgramGL2.h
    ../../include/OpenEQ/Render/Material.h
    ../../include/OpenEQ/Render/Vertex.h
    ../../include/OpenEQ/Render/Geometry.h
    ../../include/OpenEQ/Render/LinearMath.h
    ../../include/OpenEQ/Render/Scene.h
    ../../include/OpenEQ/Render/FrameStat.h
    ../../include/OpenEQ/Render/Platform.h
    ../../include/OpenEQ/Render/imath.h
    ../../include/OpenEQ/Render/dxt_tables.h
    ../../include/OpenEQ/Render/dds.h
    ../../include/OpenEQ/Render/dxt.h
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

add_library(OpenEQRender SHARED
    ${LIB_SOURCES}
    ${LIB_MOC_SOURCES}
    ${LIB_RESOURCES}
    ${LIB_RESOURCE_FILES}
    ${LIB_HEADERS}
)
target_link_libraries(OpenEQRender
    ${QT_LIBRARIES}
    ${GLEW_LIBRARIES}
)
