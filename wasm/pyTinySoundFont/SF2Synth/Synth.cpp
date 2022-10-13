#include "Synth.h"
#include <math.h>
#include <memory.h>

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

unsigned MixF32(unsigned* offsets, unsigned numBufs)
{
	unsigned maxLen = 0;
	for (unsigned i = 0; i < numBufs; i++)
	{
		unsigned len = offsets[i+1] - offsets[i];
		len /= sizeof(float);
		if (maxLen < len) maxLen = len;
	}
	float* f32Out = (float*)((unsigned char*)(offsets) + sizeof(unsigned)*(numBufs+1));
	memset(f32Out, 0, maxLen*sizeof(float));
	
	for (unsigned i = 0; i < numBufs; i++)
	{
		unsigned len = offsets[i+1] - offsets[i];
		len /= sizeof(float);
		float* f32bytes = (float*)(unsigned char*)(offsets[i]);
		for (unsigned j = 0; j < len; j++)
			f32Out[j] += f32bytes[j];
	}
	
	return maxLen;
}

unsigned char* SetNoteState(unsigned char* p_noteState, double sourceSamplePosition, double z1, double z2)
{
	NoteState* noteState = (NoteState*)p_noteState;
	*noteState = { sourceSamplePosition, {z1, z2}};
	return p_noteState + sizeof(NoteState);
}

unsigned char* SetControlHeader(unsigned char* p_control, int outputmode, unsigned loopStart, unsigned loopEnd, unsigned end, float panFactorLeft, float panFactorRight, unsigned effect_sample_block)
{
	SynthCtrl_Header* header = (SynthCtrl_Header*)p_control;
	*header = { (OutputMode)outputmode, loopStart,  loopEnd, end, panFactorLeft, panFactorRight, effect_sample_block};
	return p_control + sizeof(SynthCtrl_Header);
}

unsigned char* SetControlPoint(unsigned char* p_ctrl_pnt, int looping, float gainMono, double pitchRatio, int active, double a0, double a1, double b1, double b2)
{
	SynthCtrlPnt* ctrl_pnt = (SynthCtrlPnt*)p_ctrl_pnt;
	*ctrl_pnt = { (char)looping, gainMono, pitchRatio, { (char)active, a0, a1, b1, b2} };
	return p_ctrl_pnt + sizeof(SynthCtrlPnt);
}

void Synth(const float* input, float* outputBuffer, unsigned numSamples, unsigned char* p_noteState, const unsigned char* p_control, unsigned num_ctrl_pnts)
{
	NoteState& noteState = *(NoteState*)p_noteState;
	const SynthCtrl_Header& control_header = *(const SynthCtrl_Header*)p_control;
	const SynthCtrlPnt* p_ctrl_pnts = (const SynthCtrlPnt*)(p_control + sizeof(SynthCtrl_Header));
	
	float* outL = outputBuffer;
	float* outR = (control_header.outputmode == STEREO_UNWEAVED ? outL + numSamples : nullptr);

	unsigned tmpLoopStart = control_header.loopStart;
	unsigned tmpLoopEnd = control_header.loopEnd;
	unsigned tmpEnd = control_header.end;
	double tmpSourceSamplePosition = noteState.sourceSamplePosition;
	
	double tmpSampleEndDbl = (double)tmpEnd;
	double tmpLoopEndDbl = (double)tmpLoopEnd + 1.0;

	unsigned i_ctrl = 0;
	SynthCtrlPnt ctrlPnt;

	LowPassState lowPassState = noteState.lowPass;

	while (numSamples)
	{
		float gainLeft, gainRight;
		int blockSamples = (numSamples > control_header.effect_sample_block ? control_header.effect_sample_block : numSamples);
		numSamples -= blockSamples;

		if (i_ctrl<num_ctrl_pnts)
			ctrlPnt = p_ctrl_pnts[i_ctrl];
		else
			break;

		float gainMono = ctrlPnt.gainMono;
		double pitchRatio = ctrlPnt.pitchRatio;
		bool interpolation = pitchRatio <= 1.0f;

		gainLeft = gainMono *control_header.panFactorLeft;
		gainRight = gainMono  * control_header.panFactorRight;

		LowPassCtrlPnt lowPassCtrlPnt = ctrlPnt.lowPass;

		while (blockSamples-- && tmpSourceSamplePosition < tmpSampleEndDbl)
		{
			float val = 0.0f;
			if (interpolation)
			{
				int ipos1 = (int)floor(tmpSourceSamplePosition);
				float frac = (float)(tmpSourceSamplePosition - (double)ipos1);
				int ipos2 = ipos1 + 1;
				int ipos3 = ipos1 + 2;
				int ipos0 = ipos1 - 1;

				if (ipos1 > (int)tmpLoopEnd && ctrlPnt.looping)
				{
					ipos2 = tmpLoopStart;
					ipos3 = tmpLoopStart + 1;
				}
				if (ipos2 >= (int)tmpEnd) ipos2 = tmpEnd - 1;
				if (ipos3 >= (int)tmpEnd) ipos3 = tmpEnd - 1;
				if (ipos0 < 0) ipos0 = 0;

				float p0 = input[ipos0];
				float p1 = input[ipos1];
				float p2 = input[ipos2];
				float p3 = input[ipos3];
				
				float frac2 = frac * frac;
				float frac3 = frac2 * frac;				

				val = (-0.5f*p0 + 1.5f*p1 - 1.5f*p2 + 0.5f*p3)*frac3 +
					(p0 - 2.5f*p1 + 2.0f*p2 - 0.5f*p3)*frac2 +
					(-0.5f*p0 + 0.5f*p2)*frac + p1;
			}
			else
			{
				int ipos1 = (int)ceil(tmpSourceSamplePosition - 0.5* pitchRatio);
				int ipos2 = (int)floor(tmpSourceSamplePosition + 0.5* pitchRatio);
				int count = ipos2 - ipos1 + 1;
				for (int ipos = ipos1; ipos <= ipos2; ipos++)
				{
					int _ipos = ipos;
					if (_ipos < 0) _ipos = 0;
					if (_ipos > (int)tmpLoopEnd && ctrlPnt.looping)
					{
						_ipos += (int)tmpLoopStart - (int)tmpLoopEnd -1;
					}
					if (_ipos >= (int)tmpEnd)
					{
						_ipos = tmpEnd - 1;
					}
					val += input[_ipos];
				}
				val /= (float)count;
			}

			if (lowPassCtrlPnt.active)
			{
				double In = val;
				val = (float)(In * lowPassCtrlPnt.a0 + lowPassState.z1);
				lowPassState.z1 = In * lowPassCtrlPnt.a1 + lowPassState.z2 - lowPassCtrlPnt.b1 * val; 
				lowPassState.z2 = In * lowPassCtrlPnt.a0 - lowPassCtrlPnt.b2 * val; 
			}

			switch (control_header.outputmode)
			{
			case STEREO_INTERLEAVED:
				*outL++ += val * gainLeft;
				*outL++ += val * gainRight;
				break;
			case STEREO_UNWEAVED:
				*outL++ += val * gainLeft;
				*outR++ += val * gainRight;
				break;
			case MONO:
				*outL++ += val * gainMono;
				break;
			}
			// Next sample.
			tmpSourceSamplePosition += pitchRatio;
			if (tmpSourceSamplePosition >= tmpLoopEndDbl && ctrlPnt.looping) 
				tmpSourceSamplePosition -= (tmpLoopEnd - tmpLoopStart + 1.0f);

		}

		if (tmpSourceSamplePosition >= tmpSampleEndDbl)
			break;

		i_ctrl++;
	}

	noteState.sourceSamplePosition= tmpSourceSamplePosition;
	noteState.lowPass = lowPassState;
	
}
