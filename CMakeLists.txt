cmake_minimum_required (VERSION 3.0)
project(pyTinySoundFont_ALL)

IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  SET(CMAKE_INSTALL_PREFIX  ../test CACHE PATH "Install path" FORCE)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

add_subdirectory(pyTinySoundFont/SF2Synth)
add_subdirectory(pyTinySoundFont)

install(FILES setup.py README.md DESTINATION .)
