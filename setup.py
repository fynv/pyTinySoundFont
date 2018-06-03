#!/usr/bin/python3

from setuptools import setup, Extension
from codecs import open
import os
here = os.path.abspath(os.path.dirname(__file__))
with open(os.path.join(here, 'README.md'), encoding='utf-8') as f:
    long_description = f.read()

extra_compile_args=[]
if os.name != 'nt':
	extra_compile_args = ['-std=c++11']

SF2Synth_Src=[
	'pyTinySoundFont/SF2Synth/SF2Synth_Module.cpp',
	'pyTinySoundFont/SF2Synth/Synth.cpp'
]

SF2Synth_IncludeDirs=[
	'pyTinySoundFont/SF2Synth'
]

module_SF2Synth = Extension(
	'pyTinySoundFont.PySF2Synth',
	sources = SF2Synth_Src,
	include_dirs = SF2Synth_IncludeDirs,
	extra_compile_args=extra_compile_args)

setup(
	name = 'pyTinySoundFont',
	version = '0.0.1',
	description = 'Python port of TinySoundFont',
	long_description=long_description,
	long_description_content_type='text/markdown',  
	url='https://github.com/fynv/pyTinySoundFont',
	author='Fei Yang',
	author_email='hyangfeih@gmail.com',
	keywords='synthesizer soundfont sf2',
	packages=['pyTinySoundFont'],
	ext_modules=[module_SF2Synth]
)

