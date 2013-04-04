subdirs(CharacterViewer)
subdirs(ZoneViewer)
subdirs(ssplayer)
if(UNIX)
    # Do not require SDL on Windows yet.
    subdirs(play_wav)
endif(UNIX)
