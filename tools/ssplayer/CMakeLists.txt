set(SSPLAYER_SOURCES
    randgen.cpp
    rsgcreate.cpp
    midiout.cpp
    xmidi.cpp
)

set(SSPLAYER_HEADERS
    randgen.h
    midiout.h
    xmidi.h
    databuf.h
)

include_directories(include)

add_executable(SSPlayer
    ${SSPLAYER_SOURCES}
    ${SSPLAYER_HEADERS}
)

if(WIN32)
    target_link_libraries(SSPlayer winmm.lib)
endif(WIN32)
