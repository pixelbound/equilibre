set(LIB_SOURCES
    Fragments.cpp
    PFSArchive.cpp
    StreamReader.cpp
    WLDActor.cpp
    WLDData.cpp
    WLDFragment.cpp
    WLDModel.cpp
    WLDSkeleton.cpp
    Zone.cpp
)

set(LIB_HEADERS
    ../../include/OpenEQ/Game/WLDData.h
    ../../include/OpenEQ/Game/WLDFragment.h
    ../../include/OpenEQ/Game/Fragments.h
    ../../include/OpenEQ/Game/WLDModel.h
    ../../include/OpenEQ/Game/StreamReader.h
    ../../include/OpenEQ/Game/PFSArchive.h
    ../../include/OpenEQ/Game/WLDActor.h
    ../../include/OpenEQ/Game/Zone.h
    ../../include/OpenEQ/Game/WLDSkeleton.h
)

QT4_WRAP_CPP(LIB_MOC_SOURCES ${LIB_HEADERS})

source_group(moc FILES ${LIB_MOC_SOURCES})

add_definitions(-DBUILD_GAME_DLL)

add_library(OpenEQGame SHARED
    ${LIB_SOURCES}
    ${LIB_MOC_SOURCES}
    ${LIB_RESOURCES}
    ${LIB_RESOURCE_FILES}
    ${LIB_HEADERS}
)

target_link_libraries(OpenEQGame
    OpenEQRender
    ${QT_LIBRARIES}
    ${ZLIB_LIBRARIES}
)
