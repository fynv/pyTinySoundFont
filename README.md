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

## Use cases

### Realtime Synthesis

Pretty similar to the C version

```Python

	import pyTinySoundFont as tsf

	g_TinySoundFont = tsf.TinySoundFont('florestan-subset.sf2')

	g_TinySoundFont.NoteOn(0,48,1.0) # C2
	g_TinySoundFont.NoteOn(0,52,1.0) # E2

	# We don't have an output device here, just open a file to simulate
	with open('dmp.raw','wb') as f:
		buf =  bytes(512 * 4 * 2) # create a buffer of 512 samples
		for i in range(200): # render 200 times to the buffer when notes are on
			g_TinySoundFont.Render(buf, 512, False)
			f.write(buf)
		
		g_TinySoundFont.NoteOffAll()
	
		for i in range(10): # render another 10 times after notes are off 
			g_TinySoundFont.Render(buf, 512, False)
			f.write(buf)
	

```

### Non-Realtime Synthesis

The use case is that sometimes we just want to render some preprogrammed notes to a buffer as soon as possible, and we don't need immediate play-back. In that case, we can render 1 note each time then blend them together. Bellow example shows how to render a single note.


```Python

	import wave
	import pyTinySoundFont as tsf
	
	sf2= tsf.LoadSF2('florestan-subset.sf2')
	presets = tsf.LoadPresets(sf2)
	
	# Render C5, required length is set to 2 seconds
	# The actual returned buffer will be a little longer than 2 seconds
	# There will some extra samples after the loop is ended
	res=tsf.SynthNote(sf2[1], presets, 0, 60, 1.0, 44100*2)

	# Utility to convert float32 to short16
	wavS16=tsf.F32ToS16(res[1], 1.0)
	
	# Here we write the generated samples to a wav file
	# We can also program to mix the samples with other buffer
	with wave.open('out.wav', mode='wb') as wavFile:
		wavFile.setnchannels(2)
		wavFile.setsampwidth(2)
		wavFile.setframerate(44100)
		wavFile.setnframes(len(wavS16)//4)
		wavFile.writeframes(wavS16)

```
