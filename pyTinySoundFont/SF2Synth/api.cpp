#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#define SF2_API __declspec(dllexport)
#else
#define SF2_API 
#endif

extern "C"
{
	// general
	SF2_API  void* PtrArrayCreate(unsigned long long size, const void** ptrs);
	SF2_API  void PtrArrayDestroy(void* ptr_arr);

	// F32Buf
	SF2_API  void* F32BufCreate(unsigned long long size, float value);
	SF2_API  void F32BufDestroy(void* ptr);
	SF2_API  float* F32BufData(void* ptr);
	SF2_API  int F32BufSize(void* ptr);
	SF2_API  void F32BufToS16(void* ptr, short* dst, float amplitude);
	SF2_API  void F32BufFromS16(void* ptr, const short* data, unsigned long long size);
	SF2_API  float F32BufMaxValue(void* ptr);
	SF2_API  void F32BufMix(void* ptr, void* ptr_lst);
	SF2_API  void F32BufZero(void* ptr);

	// NoteState
	SF2_API  void* NoteStateCreate(double sourceSamplePosition, double z1, double z2);
	SF2_API  void NoteStateDestroy(void* ptr_arr);
	SF2_API double NoteStateSourceSamplePosition(void* ptr_arr);
	SF2_API double NoteStateZ1(void* ptr_arr);
	SF2_API double NoteStateZ2(void* ptr_arr);

	// SynthCtrlPnt
	SF2_API void* SynthCtrlPntCreate(int looping, float gainMono, double pitchRatio, int active, double a0, double a1, double b1, double b2);
	SF2_API void SynthCtrlPntDestroy(void* ptr_arr);

	// SynthCtrl
	SF2_API void* SynthCtrlCreate(int outputmode, unsigned loopStart, unsigned loopEnd, unsigned end,
		float panFactorLeft, float panFactorRight, unsigned effect_sample_block, void* lst_controlPnts);

	SF2_API void SynthCtrlDestroy(void* ptr_arr);

	// Synth
	SF2_API void SynthWav(void* pin, void* pout, unsigned numSamples, void* pns, void* pctrl);
}


#include <vector>
#include <cmath>
#include <memory.h>

typedef std::vector<void*> PtrArray;
typedef std::vector<float> F32Buf;

#include "Synth.h"


// general
void* PtrArrayCreate(unsigned long long size, const void** ptrs)
{
	PtrArray* ret = new PtrArray(size);
	memcpy(ret->data(), ptrs, sizeof(void*)*size);
	return ret;
}

void PtrArrayDestroy(void* ptr_arr)
{
	PtrArray* arr = (PtrArray*)ptr_arr;
	delete arr;
}

// F32Buf
void* F32BufCreate(unsigned long long size, float value)
{
	return new F32Buf(size, value);
}

void F32BufDestroy(void* ptr)
{
	F32Buf* buf = (F32Buf*)ptr;
	delete buf;
}

float* F32BufData(void* ptr)
{
	F32Buf* buf = (F32Buf*)ptr;
	return buf->data();
}

int F32BufSize(void* ptr)
{
	F32Buf* buf = (F32Buf*)ptr;
	return (int)(buf->size());
}

void F32BufToS16(void* ptr, short* dst, float amplitude)
{
	F32Buf* buf = (F32Buf*)ptr;
	for (size_t i = 0; i < buf->size(); i++)
		dst[i] = (short)((*buf)[i] * 32767.0f*amplitude + 0.5f);
}

void F32BufFromS16(void* ptr, const short* data, unsigned long long size)
{
	F32Buf* buf = (F32Buf*)ptr;
	buf->resize(size);
	for (size_t i = 0; i < size; i++)
	{
		(*buf)[i] = (float)data[i] / 32767.0f;
	}
}

float F32BufMaxValue(void* ptr)
{
	F32Buf* buf = (F32Buf*)ptr;

	float maxV = 0.0f;
	for (size_t i = 0; i < buf->size(); i++)
	{
		float v = fabsf((*buf)[i]);
		if (v > maxV) maxV = v;
	}

	return maxV;
}

void F32BufMix(void* ptr, void* ptr_lst)
{
	F32Buf* target_buf = (F32Buf*)ptr;
	PtrArray* list = (PtrArray*)ptr_lst;

	unsigned numBufs = (unsigned)list->size();
	size_t maxLen = 0;
	for (unsigned i = 0; i < numBufs; i++)
	{
		F32Buf* buf = (F32Buf*)(*list)[i];
		size_t len = buf->size();

		if (maxLen < len)
			maxLen = (unsigned)len;
	}

	target_buf->resize(maxLen);
	float* f32Out = target_buf->data();
	memset(f32Out, 0, maxLen * sizeof(float));

	for (unsigned i = 0; i < numBufs; i++)
	{
		F32Buf* buf = (F32Buf*)(*list)[i];
		size_t len = buf->size();
		const float* f32In = buf->data();

		for (unsigned j = 0; j < len; j++)
			f32Out[j] += f32In[j];
	}
}

void F32BufZero(void* ptr)
{
	F32Buf* buf = (F32Buf*)ptr;
	memset(buf->data(), 0, sizeof(float)*buf->size());
}

void* NoteStateCreate(double sourceSamplePosition, double z1, double z2)
{
	return new NoteState({ sourceSamplePosition, {z1,z2} });
}

void NoteStateDestroy(void* ptr_arr)
{
	NoteState* arr = (NoteState*)ptr_arr;
	delete arr;
}

double NoteStateSourceSamplePosition(void* ptr_arr)
{
	NoteState* arr = (NoteState*)ptr_arr;
	return arr->sourceSamplePosition;
}

double NoteStateZ1(void* ptr_arr)
{
	NoteState* arr = (NoteState*)ptr_arr;
	return arr->lowPass.z1;
}

double NoteStateZ2(void* ptr_arr)
{
	NoteState* arr = (NoteState*)ptr_arr;
	return arr->lowPass.z2;
}


void* SynthCtrlPntCreate(int looping, float gainMono, double pitchRatio,
	int active, double a0, double a1, double b1, double b2)
{
	return new SynthCtrlPnt({ (char)looping, gainMono, pitchRatio, { (char)active, a0, a1, b1, b2} });
}

void SynthCtrlPntDestroy(void* ptr_arr)
{
	SynthCtrlPnt* arr = (SynthCtrlPnt*)ptr_arr;
	delete arr;
}

void* SynthCtrlCreate(int outputmode, unsigned loopStart, unsigned loopEnd, unsigned end,
	float panFactorLeft, float panFactorRight, unsigned effect_sample_block, void* lst_controlPnts)
{
	SynthCtrl* newCtrl = new SynthCtrl({ (OutputMode)outputmode, loopStart,  loopEnd, end, panFactorLeft, panFactorRight, effect_sample_block});
	PtrArray* arr = (PtrArray*)lst_controlPnts;
	for (size_t i = 0; i < arr->size(); i++)
	{
		SynthCtrlPnt* pnt = (SynthCtrlPnt*)((*arr)[i]);
		newCtrl->controlPnts.push_back(*pnt);
	}
	return newCtrl;
}


void SynthCtrlDestroy(void* ptr_arr)
{
	SynthCtrl* arr = (SynthCtrl*)ptr_arr;
	delete arr;
}


void SynthWav(void* pin, void* pout, unsigned numSamples, void* pns, void* pctrl)
{
	F32Buf* inBuf = (F32Buf*)pin;
	F32Buf* outBuf = (F32Buf*)pout;
	NoteState* ns = (NoteState*)pns;
	SynthCtrl* control = (SynthCtrl*)pctrl;
	Synth(inBuf->data(), outBuf->data(), numSamples, *ns, *control);
}
