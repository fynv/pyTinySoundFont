cmake_minimum_required (VERSION 3.0)

project(SF2Synth)


set(SOURCES
api.cpp
Synth.cpp
)

set(HEADERS 
Synth.h
)


if (WIN32) 
set (DEFINES  ${DEFINES}
-D"_CRT_SECURE_NO_DEPRECATE"  
-D"_SCL_SECURE_NO_DEPRECATE" 
)
else()
add_definitions(-std=c++0x)
add_compile_options(-fPIC)
endif()

add_definitions(${DEFINES})
add_library (SF2Synth SHARED  ${SOURCES} ${HEADERS})

if (WIN32) 
install(TARGETS SF2Synth RUNTIME DESTINATION pyTinySoundFont)
else()
install(TARGETS SF2Synth DESTINATION pyTinySoundFont)
endif()

