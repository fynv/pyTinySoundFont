cmake_minimum_required (VERSION 3.0)
project(pyTinySoundFont)

set(PYTHON
Binding.py
__init__.py
SF2.py
SF2Presets.py
SF2Synth.py
SF2SynthRT.py
)

install(FILES ${PYTHON} DESTINATION pyTinySoundFont)
