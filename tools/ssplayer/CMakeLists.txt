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
