#include <emscripten.h>

extern "C"
{
	EMSCRIPTEN_KEEPALIVE void* alloc(unsigned long long size);
	EMSCRIPTEN_KEEPALIVE void dealloc(void* ptr);
	EMSCRIPTEN_KEEPALIVE void zero(void* ptr, unsigned long long size);
	
	
	EMSCRIPTEN_KEEPALIVE void S16ToF32(const short* s16bytes, float* f32bytes, unsigned len);
	EMSCRIPTEN_KEEPALIVE void F32ToS16(const float* f32bytes, short* s16bytes, unsigned len, float amplitude);
	EMSCRIPTEN_KEEPALIVE float MaxValueF32(const float* f32bytes, unsigned len);
	EMSCRIPTEN_KEEPALIVE void MixF32(unsigned* p_f32bufs, unsigned* lengths, float* f32Out, unsigned maxLen, unsigned numBufs);
	
	EMSCRIPTEN_KEEPALIVE unsigned GetSizeControlHeader();
	EMSCRIPTEN_KEEPALIVE unsigned GetSizeCtrlPnt();
	EMSCRIPTEN_KEEPALIVE void SetControlHeader(unsigned char* p_control, int outputmode, unsigned loopStart, unsigned loopEnd, unsigned end, float panFactorLeft, float panFactorRight, unsigned effect_sample_block);
	EMSCRIPTEN_KEEPALIVE void SetControlPoint(unsigned char* p_ctrl_pnt, int looping, float gainMono, double pitchRatio, int active, double a0, double a1, double b1, double b2);
	EMSCRIPTEN_KEEPALIVE void SynthWav(const float* input, float* outputBuffer, unsigned numSamples, unsigned char* p_noteState, const unsigned char* p_control_header, const unsigned char* p_ctrl_pnts, unsigned num_ctrl_pnts);
}


#include <math.h>
#include <memory.h>

#include "Synth.h"

void* alloc(unsigned long long size)
{
	return malloc(size);
}

void dealloc(void* ptr)
{
	free(ptr);
}

void zero(void* ptr, unsigned long long size)
{
	memset(ptr, 0, size);
}


void S16ToF32(const short* s16bytes, float* f32bytes, unsigned len)
{
	for (unsigned i = 0; i < len; i++)
		f32bytes[i] = (float)s16bytes[i] / 32767.0f;
}

void F32ToS16(const float* f32bytes, short* s16bytes, unsigned len, float amplitude)
{
	for (unsigned i = 0; i < len; i++)
		s16bytes[i] = (short)(f32bytes[i] * 32767.0f*amplitude + 0.5f);
}

float MaxValueF32(const float* f32bytes, unsigned len)
{
	float maxV = 0.0f;
	for (unsigned i = 0; i < len; i++)
	{
		float v = fabsf(f32bytes[i]);
		if (v > maxV) maxV = v;
	}
	return maxV;
}

void MixF32(unsigned* p_f32bufs, unsigned* lengths, float* f32Out, unsigned maxLen, unsigned numBufs)
{
	memset(f32Out, 0, maxLen*sizeof(float));
	for (unsigned i = 0; i < numBufs; i++)
	{
		unsigned len = lengths[i];
		float* f32bytes = (float*)(unsigned char*)p_f32bufs[i];
		for (unsigned j = 0; j < len; j++)
			f32Out[j] += f32bytes[j];
	}
}

unsigned GetSizeControlHeader()
{
	return (unsigned)sizeof(SynthCtrl_Header);
}

unsigned GetSizeCtrlPnt()
{
	return (unsigned)sizeof(SynthCtrlPnt);
}

void SetControlHeader(unsigned char* p_control, int outputmode, unsigned loopStart, unsigned loopEnd, unsigned end, float panFactorLeft, float panFactorRight, unsigned effect_sample_block)
{
	SynthCtrl_Header* header = (SynthCtrl_Header*)p_control;
	*header = { (OutputMode)outputmode, loopStart,  loopEnd, end, panFactorLeft, panFactorRight, effect_sample_block};
}

void SetControlPoint(unsigned char* p_ctrl_pnt, int looping, float gainMono, double pitchRatio, int active, double a0, double a1, double b1, double b2)
{
	SynthCtrlPnt* ctrl_pnt = (SynthCtrlPnt*)p_ctrl_pnt;
	*ctrl_pnt = { (char)looping, gainMono, pitchRatio, { (char)active, a0, a1, b1, b2} };	
}

void SynthWav(const float* input, float* outputBuffer, unsigned numSamples, unsigned char* p_noteState, const unsigned char* p_control_header, const unsigned char* p_ctrl_pnts, unsigned num_ctrl_pnts)
{
	Synth(input, outputBuffer, numSamples, *(NoteState*)p_noteState, *(const SynthCtrl_Header*)p_control_header, (const SynthCtrlPnt*)p_ctrl_pnts, num_ctrl_pnts);	
}




