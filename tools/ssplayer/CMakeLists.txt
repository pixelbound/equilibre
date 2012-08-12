set(SSPLAYER_SOURCES
    randgen.cpp
    rsgcreate.cpp
    win_midiout.cpp
    xmidi.cpp
)

set(SSPLAYER_HEADERS
    randgen.h
    win_midiout.h
    xmidi.h
    databuf.h
)

include_directories(include)

add_executable(SSPlayer
    ${SSPLAYER_SOURCES}
    ${SSPLAYER_HEADERS}
)

target_link_libraries(SSPlayer winmm.lib)