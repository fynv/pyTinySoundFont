#!/usr/bin/python3

import wave
import pyTinySoundFont as tsf

sf2= tsf.LoadSF2('florestan-subset.sf2')
presets = tsf.LoadPresets(sf2)

res=tsf.SynthNote(sf2[1], presets, 0, 60, 1.0, 44100*2)
wavS16=tsf.F32ToS16(res[1], 1.0)

with wave.open('out.wav', mode='wb') as wavFile:
	wavFile.setnchannels(2)
	wavFile.setsampwidth(2)
	wavFile.setframerate(44100)
	wavFile.setnframes(len(wavS16)//4)
	wavFile.writeframes(wavS16)


