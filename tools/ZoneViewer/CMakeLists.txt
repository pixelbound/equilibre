set(ZONEVIEWER_SOURCES
    main.cpp
    ZoneViewerWindow.cpp
)

set(ZONEVIEWER_HEADERS
    ZoneViewerWindow.h
)

QT4_WRAP_CPP(ZONEVIEWER_MOC_SOURCES ${ZONEVIEWER_HEADERS})

source_group(moc FILES ${ZONEVIEWER_MOC_SOURCES})

include_directories(include)

add_executable(ZoneViewer
    ${ZONEVIEWER_SOURCES}
    ${ZONEVIEWER_MOC_SOURCES}
    ${ZONEVIEWER_HEADERS}
)

target_link_libraries(ZoneViewer
    EQuilibreRender
    EQuilibreGame
    ${QT_LIBRARIES}
    ${SYSTEM_LIBRARIES}
)

if(USE_VTUNE_PROFILER)
    target_link_libraries(ZoneViewer ittnotify)
endif(USE_VTUNE_PROFILER)
