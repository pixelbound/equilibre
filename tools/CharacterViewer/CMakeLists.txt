set(CHARVIEWER_SOURCES
    main.cpp
    CharacterViewerWindow.cpp
)

set(CHARVIEWER_HEADERS
    CharacterViewerWindow.h
)

QT4_WRAP_CPP(CHARVIEWER_MOC_SOURCES ${CHARVIEWER_HEADERS})

source_group(moc FILES ${CHARVIEWER_MOC_SOURCES})

add_executable(CharacterViewer
    ${CHARVIEWER_SOURCES}
    ${CHARVIEWER_MOC_SOURCES}
    ${CHARVIEWER_HEADERS}
)

target_link_libraries(CharacterViewer
    EQuilibreRender
    EQuilibreGame
    ${QT_LIBRARIES}
    ${SYSTEM_LIBRARIES}
)

if(USE_VTUNE_PROFILER)
    target_link_libraries(CharacterViewer ittnotify)
endif(USE_VTUNE_PROFILER)

