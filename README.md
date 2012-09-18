Introduction
============
EQuilibre is a project to re-create a client of the 1999 MMORPG. At the moment, only a few tools like a zone viewer and character/monster viewer have been created. Eventually, these will be fashioned together into something than is more like a game.

Disclaimmer
-----------
No copyrighted file will be distributed with this project. The tools merely load the game assets that come with a copy of the game you must have.

Why?
----
Because it's fun! I started this project because I wanted to learn some game development and how game engines work.

Building
=======
One of the goals of this project is to create a multi-platform client and tools that can be used at least on Windows and Linux. The CMake build system is used to build this project on these platforms using the same build scripts.

Dependencies
------------

* CMake 2.8+
* Qt 4.6+
* ZLIB
* GLEW 
* fluidsynth (Linux only)
* OpenGL 2.0 drivers with several 3.0+ extensions

Building on Windows
-------------------
Microsoft Visual Stdio 2008/2010 (Expression editions should work fine too) is required to build EQuilibre on Windows. CMake is used to generate Visual Studio solution and project files. It can be downloaded at http://www.cmake.org/cmake/resources/software.html.

Other dependencies to install:

* Qt at http://qt-project.org/downloads (choose the online installer, make sure the Qt libraries match your version of Visual Studio).
* GLEW at http://glew.sourceforge.net/ (download Windows 32-bit binaries and unzip in the EQuilibre folder).
* ZLIB at http://prdownloads.sourceforge.net/libpng/zlib125-dll.zip (unzip in the EQuilibre folder).

Generating the Visual Studio solutions takes a little while but only needs to be done once.  First, start cmake-gui which can be found in the Start menu. Click on the Browse Source button and select the EQuilibre folder. Then, click on Browse build, make a build folder in the EQuilibre folder and select it. Finally, click on the Configure button.

A window will pop up asking which generator to use. Visual Studio 9 or 10 should be selected here, then click Finish. This may take a minute or take and will end up in an error pop-up.

Click on the Grouped check box to reduce the number of list entries. Then click on Ungrouped Entries and click on the value to the right of QT_MAKE_EXECUTABLE. Use the '...' button to the right and locate the qmake.exe file (mine is installed at: C:\QtSDK\Desktop\Qt\4.8.1\msvc2010\bin). Once this is done, click Configure again.

Next we need to locate the ZLIB library. Click on ZLIB_INCLIUDE_DIR-NOTFOUND and use the '...' button to select the ZLIB include folder (mine is at C:/Users/PiB/Documents/Projects/EQuilibre/zlib125-dll/include). Same with the library (C:/Users/PiB/Documents/Projects/EQuilibre/zlib125-dll/lib/zdll.lib) then click configure again.

Finally, we need to locate the GLEW library in the same way (C:/Users/PiB/Documents/Projects/EQuilibre/glew-1.9.0-win32/glew-1.9.0/include and C:/Users/PiB/Documents/Projects/EQuilibre/glew-1.9.0-win32/glew-1.9.0/lib) and click configure. After this last step there should not be any error remaining. Click Generate to create the Visual Studio solutions in your build foler.

In Visual Studio, set ZoneViewer or CharacterViewer as startup project and build the solution. To run it, you will need to copy several .dll files to the output folder (EQuilibre\build\bin\Debug).

You will need to copy:

* glew32.dll from EQuilibre\glew-1.9.0-win32\glew-1.9.0\bin
* zlib1.dll from EQuilibre\zlib125-dll
* QtCore4d.dll, QtGui4d.dll and QtOpenGL4d.dll from C:\QtSDK\Desktop\Qt\4.8.1\msvc2010\bin\

At this point you can finally hit F5 and run EQuilibre!

Building on Linux
-----------------
Install CMake 2.8+, Qt 4.6+, zlib, glew and fluidsynth using your distribution's package manager. Then, in a terminal:

    $ cd /path/to/EQuilibre
    $ make build
    $ cd build
    $ cmake-gui ..

Select the default project generator (Makefiles), click Finish, Configure and Generate. Close cmake-gui.

    $ make -j4
    $ bin/ZoneViewer
    $ bin/CharacterViewer

If you want to look at the source or develop it on Linux I suggest using Qt Creator which has native support for CMake projects (File -> Open File/Project and select the top-level CMakeLists.txt file).
