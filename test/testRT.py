import pyTinySoundFont as tsf

g_TinySoundFont = tsf.TinySoundFont('florestan-subset.sf2')

g_TinySoundFont.PrintPresets()

g_TinySoundFont.SetOutput(tsf.STEREO_INTERLEAVED, 44100.0, 0.0)

g_TinySoundFont.NoteOn(0,48,1.0) # C2
g_TinySoundFont.NoteOn(0,52,1.0) # E2

with open('dmp.raw','wb') as f:
	buf =  tsf.F32Buf(512*2)
	for i in range(200):
		g_TinySoundFont.Render(buf, 512, False)
		f.write(buf.to_s16(1.0))
	
	g_TinySoundFont.NoteOffAll()

	for i in range(10):
		g_TinySoundFont.Render(buf, 512, False)
		f.write(buf.to_s16(1.0))




