pyTinySoundFont
================

This is Python porting of TinySoundFont(https://github.com/schellingb/TinySoundFont)

We are still relying on C++ for some sample level tasks, so a building process is needed (see below).

The "class TinySoundFont" interface defined in pyTinySoundFont/SF2SynthRT.py provides most of the original functions of TinySoundFont, with the following limitations:

* Loading SF2 from memory is not implemented
* "Higher level channel based functions" are not ported yet.
* Real-time playback is not part of the project, which needs a separate solution.

See test/testRT.py for a use case.

The "SynthNote()" defined in pyTinySoundFont/SF2Synth.py provides a simple interface for single note synthesis. See test/test.py for a use case.


## Building with CMake

Prerequisites:

* CMake 3.0+
* Python3

You can simply run CMake to generate makefiles/project files for your system and build. 
You can set CMAKE_INSTALL_PREFIX to /test so that the test scripts can find pyTinySoundFont.

## Building with Setuptools

	$ python3 setup.py build	
	$ python3 setup.py install
