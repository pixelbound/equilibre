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

if(WIN32)
    set(SSPLAYER_SOURCES ${SSPLAYER_SOURCES} WindowsMidiOut.cpp)
    set(SSPLAYER_HEADERS ${SSPLAYER_HEADERS} WindowsMidiOut.h)
elseif(UNIX)
    set(SSPLAYER_SOURCES ${SSPLAYER_SOURCES} LinuxMidiOut.cpp)
    set(SSPLAYER_HEADERS ${SSPLAYER_HEADERS} LinuxMidiOut.h)
endif(WIN32)

include_directories(include)

add_executable(SSPlayer
    ${SSPLAYER_SOURCES}
    ${SSPLAYER_HEADERS}
)

if(WIN32)
    target_link_libraries(SSPlayer winmm.lib)
elseif(UNIX)
    target_link_libraries(SSPlayer pthread fluidsynth)
endif(WIN32)
