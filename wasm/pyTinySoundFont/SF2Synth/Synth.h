#ifndef _Synth_h
#define _Synth_h

#include <emscripten.h>

struct LowPassState
{
	double z1, z2;
};

struct NoteState
{
	double sourceSamplePosition;
	LowPassState lowPass;
};

struct LowPassCtrlPnt
{
	char active;
	double a0, a1, b1, b2;
};

struct SynthCtrlPnt
{
	char looping;
	float gainMono;
	double pitchRatio;

	LowPassCtrlPnt lowPass;	
};

enum OutputMode
{
	// Two channels with single left/right samples one after another
	STEREO_INTERLEAVED,
	// Two channels with all samples for the left channel first then right
	STEREO_UNWEAVED,
	// A single channel (stereo instruments are mixed into center)
	MONO,
};

struct SynthCtrl_Header
{
	OutputMode outputmode;
	unsigned loopStart, loopEnd;
	unsigned end;
	float panFactorLeft, panFactorRight;
	unsigned effect_sample_block;	
};


extern "C"
{
	void EMSCRIPTEN_KEEPALIVE S16ToF32(const short* s16bytes, float* f32bytes, unsigned len);
	void EMSCRIPTEN_KEEPALIVE F32ToS16(const float* f32bytes, short* s16bytes, unsigned len, float amplitude);
	float EMSCRIPTEN_KEEPALIVE MaxValueF32(const float* f32bytes, unsigned len);
	unsigned EMSCRIPTEN_KEEPALIVE MixF32(unsigned* offsets, unsigned numBufs);
	unsigned char* EMSCRIPTEN_KEEPALIVE SetNoteState(unsigned char* p_noteState, double sourceSamplePosition, double z1, double z2);
	unsigned char* EMSCRIPTEN_KEEPALIVE SetControlHeader(unsigned char* p_control, int outputmode, unsigned loopStart, unsigned loopEnd, unsigned end, float panFactorLeft, float panFactorRight, unsigned effect_sample_block);
	unsigned char* EMSCRIPTEN_KEEPALIVE SetControlPoint(unsigned char* p_ctrl_pnt, int looping, float gainMono, double pitchRatio, int active, double a0, double a1, double b1, double b2);
	void EMSCRIPTEN_KEEPALIVE Synth(const float* input, float* outputBuffer, unsigned numSamples, unsigned char* p_noteState, const unsigned char* p_control, unsigned num_ctrl_pnts);
}



#endif
