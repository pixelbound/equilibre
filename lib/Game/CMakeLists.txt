set(LIB_SOURCES
    Fragments.cpp
    Game.cpp
    PFSArchive.cpp
    SoundTrigger.cpp
    StreamReader.cpp
    WLDActor.cpp
    WLDData.cpp
    WLDModel.cpp
    WLDSkeleton.cpp
    Zone.cpp
)

set(LIB_HEADERS
    ../../include/EQuilibre/Game/WLDData.h
    ../../include/EQuilibre/Game/Fragments.h
    ../../include/EQuilibre/Game/Game.h
    ../../include/EQuilibre/Game/WLDModel.h
    ../../include/EQuilibre/Game/SoundTrigger.h
    ../../include/EQuilibre/Game/StreamReader.h
    ../../include/EQuilibre/Game/PFSArchive.h
    ../../include/EQuilibre/Game/WLDActor.h
    ../../include/EQuilibre/Game/Zone.h
    ../../include/EQuilibre/Game/WLDSkeleton.h
)

QT4_WRAP_CPP(LIB_MOC_SOURCES ${LIB_HEADERS})

source_group(moc FILES ${LIB_MOC_SOURCES})

add_definitions(-DBUILD_GAME_DLL)

add_library(EQuilibreGame SHARED
    ${LIB_SOURCES}
    ${LIB_MOC_SOURCES}
    ${LIB_RESOURCES}
    ${LIB_RESOURCE_FILES}
    ${LIB_HEADERS}
)

target_link_libraries(EQuilibreGame
    EQuilibreRender
    ${QT_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${SYSTEM_LIBRARIES}
)
