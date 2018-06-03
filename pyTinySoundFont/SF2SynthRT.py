from .SF2 import LoadSF2
from .SF2Presets import LoadPresets
from .SF2Synth import *
from .PySF2Synth import ZeroBuf

def VoiceKill(v):
		v['region'] = None
		v['playingPreset'] = -1

def VoiceEnd(v, outSampleRate):
	VoiceEnvelopeNextsegment_Sustain(v['ampenv'], outSampleRate)
	VoiceEnvelopeNextsegment_Sustain(v['modenv'], outSampleRate)
	if v['region']['loop_mode']== TSF_LOOPMODE_SUSTAIN:
		# Continue playing, but stop looping.
		v['loopEnd']=v['loopStart']
		
def VoiceEndquick(v, outSampleRate):
	v['ampenv']['parameters']['release'] = 0.0
	VoiceEnvelopeNextsegment_Sustain(v['ampenv'], outSampleRate)
	v['modenv']['parameters']['release'] = 0.0
	VoiceEnvelopeNextsegment_Sustain(v['modenv'], outSampleRate)

def VoiceCalcpitchratio(v, pitchShift, outSampleRate):
	note = v['playingKey'] + v['region']['transpose'] + v['region']['tune'] / 100.0
	adjustedPitch = v['region']['pitch_keycenter'] + (note - v['region']['pitch_keycenter']) * (v['region']['pitch_keytrack'] / 100.0)
	if pitchShift!=0:
		adjustedPitch += pitchShift
	v['pitchInputTimecents'] = adjustedPitch * 100.0
	v['pitchOutputFactor'] = v['region']['sample_rate'] / (TimeCents2Sec(v['region']['pitch_keycenter'] * 100.0) * outSampleRate)


class TinySoundFont:
	def __init__(self, fn):
		sf2 = LoadSF2(fn)
		self.presets=LoadPresets(sf2)
		self.fontSamples=sf2[1]
		self.voices =[]
		self.voicePlayIndex=0
		self.outputmode = STEREO_INTERLEAVED
		self.outSampleRate = 44100.0
		self.globalGainDB = 0.0

	def SetOutput(self,  outputmode, samplerate, global_gain_db = 0.0):
		self.outputmode = outputmode
		self.samplerate = samplerate
		self.globalGainDB = global_gain_db

	def GetPresetCount(self):
		return len(self.presets)

	def GetPresetName(self, i):
		return self.presets[i]['presetName']

	def PrintPresets(self):
		for i in range(len(self.presets)):
			preset = self.presets[i]
			print ('%d : %s bank=%d number=%d' % (i, preset['presetName'], preset['bank'], preset['preset']))

	def GetPresetIndex(self, bank, preset_number):
		for i in range(len(self.presets)):
			preset = self.presets[i]
			if preset['preset'] == preset_number and preset['bank'] == bank:
				return i
		return -1

	def BankGetPresetName(self, bank, preset_number):
		return self.GetPresetName(self.GetPresetIndex(bank,preset_number))


	def VoiceRender(self, voice, outputBuffer, numSamples):
		region = voice['region']
		inputSamples = self.fontSamples

		updateModEnv = region['modEnvToPitch'] or region['modEnvToFilterFc']
		updateModLFO = voice['modlfo']['delta']!=0.0 and (region['modLfoToPitch']!=0 or region['modLfoToFilterFc']!=0 or region['modLfoToVolume']!=0)
		updateVibLFO = voice['viblfo']['delta']!= 0.0 and (region['vibLfoToPitch']!= 0)
		isLooping = voice['loopStart'] < voice['loopEnd']

		tmpLoopStart = voice['loopStart']
		tmpLoopEnd  = voice['loopEnd']
		tmpSampleEnd = region['end']
		tmpSourceSamplePosition = voice['ns']['sourceSamplePosition']

		tmpLowPass = voice['lowpass'].copy()

		dynamicLowpass = region['modLfoToFilterFc']!=0 or region['modEnvToFilterFc']!=0
		dynamicPitchRatio = region['modLfoToPitch']!=0 or region['modEnvToPitch']!=0 or region['vibLfoToPitch']!=0
		dynamicGain = region['modLfoToVolume']!=0

		tmpSampleRate=0
		tmpInitialFilterFc = 0
		tmpModLfoToFilterFc = 0
		tmpModEnvToFilterFc = 0
		if dynamicLowpass:
			tmpSampleRate = samplerate
			tmpInitialFilterFc = region['initialFilterFc']
			tmpModLfoToFilterFc = region['modLfoToFilterFc']
			tmpModEnvToFilterFc = region['modEnvToFilterFc']

		pitchRatio = TimeCents2Sec(voice['pitchInputTimecents'])*voice['pitchOutputFactor']
		tmpModLfoToPitch = 0
		tmpVibLfoToPitch = 0
		tmpModEnvToPitch = 0
		if dynamicPitchRatio:
			pitchRatio = 0
			tmpModLfoToPitch = region['modLfoToPitch']
			tmpVibLfoToPitch = region['vibLfoToPitch']
			tmpModEnvToPitch = region['modEnvToPitch']

		noteGain = DecibelsToGain(voice['noteGainDB'])
		tmpModLfoToVolume = 0
		if dynamicGain:
			noteGain = 0
			tmpModLfoToVolume = region['modLfoToVolume']*0.1

		control={
			'outputmode': self.outputmode,
			'loopStart': tmpLoopStart,
			'loopEnd': tmpLoopEnd,
			'end': tmpSampleEnd,
			'panFactorLeft': voice['panFactorLeft'],
			'panFactorRight': voice['panFactorRight'],
			'effect_sample_block': TSF_RENDER_EFFECTSAMPLEBLOCK,
			'controlPnts': []
		}

		remainingSamples=numSamples

		while remainingSamples>0:
			blockSamples = TSF_RENDER_EFFECTSAMPLEBLOCK
			if remainingSamples < TSF_RENDER_EFFECTSAMPLEBLOCK:
				blockSamples= remainingSamples
			remainingSamples -= blockSamples

			if dynamicLowpass:
				fres = tmpInitialFilterFc + voice['modlfo']['level'] * tmpModLfoToFilterFc + voice['modenv']['level'] * tmpModEnvToFilterFc
				tmpLowpass['active'] = fres <= 13500.0
				if tmpLowpass['active']:
					VoiceLowpassSetup(tmpLowpass, Cents2Hertz(fres)/tmpSampleRate)

			if dynamicPitchRatio:
				pitchRatio = TimeCents2Sec(voice['pitchInputTimecents'] + (voice['modlfo']['level']*tmpModLfoToPitch +voice['viblfo']['level']*tmpVibLfoToPitch + voice['modenv']['level']*tmpModEnvToPitch))*  voice['pitchOutputFactor']

			if dynamicGain:
				noteGain = DecibelsToGain(voice['noteGainDB']+ (voice['modlfo']['level']*tmpModLfoToVolume))

			gainMono = noteGain * voice['ampenv']['level']

			# Update EG.
			VoiceEnvelopeProcess(voice['ampenv'], blockSamples, self.samplerate)
			if updateModEnv:
				VoiceEnvelopeProcess(voice['modenv'], blockSamples, self.samplerate)

			# Update LFOs.
			if updateModLFO:
				VoiceLfoProcess(voice['modlfo'], blockSamples)
			if updateVibLFO:
				VoiceLfoProcess(voice['viblfo'], blockSamples)

			ctrlPnt = {
				'looping': isLooping,
				'gainMono': gainMono,
				'pitchRatio': pitchRatio,
				'lowPass': {
					'active': tmpLowPass['active'],
					'a0': tmpLowPass['a0'],
					'a1': tmpLowPass['a1'],
					'b1': tmpLowPass['b1'],
					'b2': tmpLowPass['b2'],
				}
			}

			control['controlPnts']+=[ctrlPnt]

			tmpSourceSamplePosition +=  pitchRatio*blockSamples

			while tmpSourceSamplePosition > tmpLoopEnd and isLooping: 
				tmpSourceSamplePosition -= (tmpLoopEnd - tmpLoopStart + 1.0)

			if tmpSourceSamplePosition > tmpSampleEnd or voice['ampenv']['segment'] == TSF_SEGMENT_DONE:
				VoiceKill(voice)
				break
		Synth(inputSamples, outputBuffer, numSamples, voice['ns'], control)

	def Reset(self):
		for voice in self.voices:
			if voice['playingPreset '] != -1 and (v['ampenv']['segment'] < TSF_SEGMENT_RELEASE or v['ampenv']['parameters']['release']!=0):
				VoiceEndquick(voice, self.outSampleRate)

	def NoteOn(self, preset_index, key, vel):
		midiVelocity = int(vel*127)
		if preset_index < 0 or preset_index >= len(self.presets):
			return

		if vel <= 0.0:
			self.NoteOff(preset_index, key)
			return

		# Play all matching regions.
		voicePlayIndex = self.voicePlayIndex
		self.voicePlayIndex+=1
		for region in self.presets[preset_index]['regions']:
			if key < region['lokey'] or key > region['hikey'] or midiVelocity < region['lovel'] or midiVelocity > region['hivel']:
				continue

			voice = None
			if region['group']!=0:
				for v in self.voices:
					if v['playingPreset'] == preset_index and v['region']['group'] == region['group']:
						VoiceEndquick(v, self.outSampleRate)
					elif v['playingPreset'] == -1 and voice==None:
						voice = v
			else:
				for v in self.voices:
					if v['playingPreset'] == -1:
						voice = v
						break

			if voice == None:
				voice = {
					'ns': {},
					'ampenv': {},
					'modenv': {},
					'lowpass': {},
					'modlfo':{},
					'viblfo':{}
				}
				self.voices+=[voice]

			voice['region'] = region
			voice['playingPreset'] = preset_index
			voice['playingKey'] = key
			voice['playIndex'] = voicePlayIndex
			voice['noteGainDB'] = self.globalGainDB - region['attenuation'] - GainToDecibels(1.0/vel)

			VoiceCalcpitchratio(voice, 0, self.outSampleRate)

			# The SFZ spec is silent about the pan curve, but a 3dB pan law seems common. This sqrt() curve matches what Dimension LE does; Alchemy Free seems closer to sin(adjustedPan * pi/2).
			voice['panFactorLeft'] = math.sqrt(0.5 - region['pan'])
			voice['panFactorRight'] = math.sqrt(0.5 + region['pan'])

			# Offset/end.
			voice['ns']['sourceSamplePosition'] = region['offset']

			# Loop.
			doLoop = region['loop_mode'] != TSF_LOOPMODE_NONE and region['loop_start'] < region['loop_end']
			voice['loopStart'] = 0
			voice['loopEnd'] = 0
			if doLoop:
				voice['loopStart'] = region['loop_start']
				voice['loopEnd'] = region['loop_end']

			# Setup envelopes.
			VoiceEnvelopeSetup(voice['ampenv'], region['ampenv'], key, midiVelocity, True, self.samplerate)
			VoiceEnvelopeSetup(voice['modenv'], region['modenv'], key, midiVelocity, False, self.samplerate)

			# Setup lowpass filter.
			filterQDB = region['initialFilterQ'] / 10.0
			voice['lowpass']['QInv'] = 1.0 / (10.0 ** (filterQDB / 20.0))
			voice['ns']['lowPass']={
				'z1': 0,
				'z2': 0,
			}
			voice['lowpass']['active'] =  region['initialFilterFc'] <= 13500
			if voice['lowpass']['active']:
				VoiceLowpassSetup(voice['lowpass'],Cents2Hertz(region['initialFilterFc']) / self.samplerate)

			# Setup LFO filters.
			VoiceLfoSetup(voice['modlfo'], region['delayModLFO'], region['freqModLFO'], self.samplerate)
			VoiceLfoSetup(voice['viblfo'], region['delayVibLFO'], region['freqVibLFO'], self.samplerate)


	def BankNoteOn(self, bank, preset_number, key, vel):
		preset_index = self.GetPresetIndex(bank, preset_number)
		if preset_index == -1:
			return 0
		self.NoteOn(preset_index,key,vel)
		return 1

	def NoteOff(self, preset_index, key):
		iMatchFirst = -1
		iMatchLast = -1
		for i in range(len(self.voices)):
			v = self.voices[i]
			# Find the first and last entry in the voices list with matching preset, key and look up the smallest play index
			if v['playingPreset'] != preset_index  or v['playingKey'] != key or v['ampenv']['segment'] >= TSF_SEGMENT_RELEASE:
				continue				
			elif iMatchFirst == -1 or v['playIndex']<self.voices[iMatchFirst]['playIndex']:
				iMatchFirst = i
				iMatchLast = i
			elif v['playIndex'] == self.voices[iMatchFirst]['playIndex']:
				iMatchLast = i
		if iMatchFirst == -1:
			return
		for i in range(iMatchFirst, iMatchLast+1):
			v = self.voices[i]
			# Stop all voices with matching preset, key and the smallest play index which was enumerated above
			if i != iMatchFirst and i != iMatchLast and (v['playIndex'] != self.voices[iMatchFirst]['playIndex'] or v['playingPreset'] != preset_index or v['playingKey'] != key or v['ampenv']['segment'] >= TSF_SEGMENT_RELEASE):
				continue
			VoiceEnd(v, self.outSampleRate)

	def BankNoteOff(self, bank, preset_number, key):
		preset_index = self.GetPresetIndex(bank, preset_number)
		if preset_index == -1:
			return 0
			NoteOff(preset_index, key)
		return 1

	def NoteOffAll(self):
		for v in self.voices:
			if v['playingPreset'] != -1 and v['ampenv']['segment'] < TSF_SEGMENT_RELEASE:
				VoiceEnd(v, self.outSampleRate)

	def Render(self, buf, samples, flag_mixing):
		if not flag_mixing:
			ZeroBuf(buf)

		for v in self.voices:
			if v['playingPreset'] !=-1:
				self.VoiceRender(v, buf, samples)

