set(SSPLAYER_SOURCES
    randgen.cpp
    rsgcreate.cpp
    MidiOut.cpp
    xmidi.cpp
)

set(SSPLAYER_HEADERS
    randgen.h
    MidiOut.h
    xmidi.h
    databuf.h
)

#if(WIN32)
    set(SSPLAYER_SOURCES ${SSPLAYER_SOURCES} WindowsMidiOut.cpp)
    set(SSPLAYER_HEADERS ${SSPLAYER_HEADERS} WindowsMidiOut.h)
#endif(WIN32)

include_directories(include)

add_executable(SSPlayer
    ${SSPLAYER_SOURCES}
    ${SSPLAYER_HEADERS}
)

if(WIN32)
    target_link_libraries(SSPlayer winmm.lib)
endif(WIN32)
