# NEWTON_FOUND - Newton was found on the system
# NEWTON_INCLUDE_DIRS - Newton include directories
# NEWTON_LIBRARIES - Newton shared library

if (NEWTON_INCLUDE_DIRS AND NEWTON_LIBRARIES)
    set(NEWTON_FOUND TRUE)
else ()
    # Find the Newton Game Dynamics
    find_path(NEWTON_INCLUDE_DIRS Newton.h
              PATHS /usr/include /usr/local/include /opt/include
    )
    find_library(NEWTON_LIBRARIES Newton
                 PATHS /usr/lib /usr/lib64
    )
    
    # Determine whether Newton was found
    set(NEWTON_FOUND FALSE)
    if (NEWTON_INCLUDE_DIRS AND NEWTON_LIBRARIES)
        set(NEWTON_FOUND TRUE)
    endif()
endif ()

if(NEWTON_FOUND)
    message(STATUS "Found Newton: ${NEWTON_LIBRARIES}")
else()
    message(FATAL_ERROR "Could NOT find Newton")
endif()
