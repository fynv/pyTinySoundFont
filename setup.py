from setuptools import setup
from codecs import open
import os

here = os.path.abspath(os.path.dirname(__file__))

with open(os.path.join(here, 'README.md'), encoding='utf-8') as f:
    long_description = f.read()

setup(
    name = 'pyTinySoundFont',
    version = '0.1.0',
    description = 'Python port of TinySoundFont',
    long_description=long_description,
    long_description_content_type='text/markdown',  
    url='https://github.com/fynv/pyTinySoundFont',
    license='MIT',
    author='Fei Yang',
    author_email='hyangfeih@gmail.com',
    keywords='synthesizer soundfont sf2',
    packages=['pyTinySoundFont'],
    package_data = { 'pyTinySoundFont': ['*.dll', '*.so']},
    install_requires = ['cffi'],    
)


