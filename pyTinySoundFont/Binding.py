import os
from cffi import FFI

ffi = FFI()
ffi.cdef("""
// general
void* PtrArrayCreate(unsigned long long size, const void** ptrs);
void PtrArrayDestroy(void* ptr_arr);

// F32Buf
void* F32BufCreate(unsigned long long size, float value);
void F32BufDestroy(void* ptr);
float* F32BufData(void* ptr);
int F32BufSize(void* ptr);	
void F32BufToS16(void* ptr, short* dst, float amplitude);
void F32BufFromS16(void* ptr, const short* data, unsigned long long size);
float F32BufMaxValue(void* ptr);
void F32BufMix(void* ptr, void* ptr_lst);
void F32BufZero(void* ptr);

// NoteState
void* NoteStateCreate(double sourceSamplePosition, double z1, double z2);
void NoteStateDestroy(void* ptr_arr);
double NoteStateSourceSamplePosition(void* ptr_arr);
double NoteStateZ1(void* ptr_arr);
double NoteStateZ2(void* ptr_arr);

// SynthCtrlPnt
void* SynthCtrlPntCreate(int looping, float gainMono, double pitchRatio, int active, double a0, double a1, double b1, double b2);
void SynthCtrlPntDestroy(void* ptr_arr);

// SynthCtrl
void* SynthCtrlCreate(int outputmode, unsigned loopStart, unsigned loopEnd, unsigned end,
    float panFactorLeft, float panFactorRight, unsigned effect_sample_block, void* lst_controlPnts);

void SynthCtrlDestroy(void* ptr_arr);

// Synth
void SynthWav(void* pin, void* pout, unsigned numSamples, void* pns, void* pctrl);
""")


if os.name == 'nt':
    fn_shared_lib = 'SF2Synth.dll'
elif os.name == "posix":
    fn_shared_lib = 'libSF2Synth.so'
    
path_shared_lib = os.path.dirname(__file__)+"/"+fn_shared_lib
Native = ffi.dlopen(path_shared_lib)


class ObjArray:
    def __init__(self, arr):
        self.m_arr = arr
        c_ptrs = [obj.m_cptr for obj in arr]
        self.m_cptr = Native.PtrArrayCreate(len(c_ptrs), c_ptrs)
            
    def __del__(self):
        Native.PtrArrayDestroy(self.m_cptr)


class F32Buf:
    def __init__(self, size, value = 0.0):
        self.m_cptr = Native.F32BufCreate(size, value)
        
    def __del__(self):
        Native.F32BufDestroy(self.m_cptr)
        
    def data(self):
        return Native.F32BufData(self.m_cptr)
        
    def size(self):
        return Native.F32BufSize(self.m_cptr)
        
    def to_s16(self, amplitude):
        _size = self.size()
        s16 = bytearray(_size*2)
        ptr_s16 = ffi.from_buffer('short[]', s16)
        Native.F32BufToS16(self.m_cptr, ptr_s16, amplitude)
        return bytes(s16)
        
    def zero(self):
        Native.F32BufZero(self.m_cptr);
        
    @classmethod
    def from_s16(cls, s16):
        f32 = cls(0)
        ptr_s16 = ffi.from_buffer('const short[]', s16)
        Native.F32BufFromS16(f32.m_cptr, ptr_s16, len(s16)//2)
        return f32
        
    def max_value(self):
        return Native.F32BufMaxValue(self.m_cptr)
            
    @classmethod
    def mix(cls, lst_bufs):
        f32 = cls()
        obj_lst = ObjArray(lst_bufs);
        Native.F32BufMix(f32.m_cptr, obj_lst.m_cptr)        
        return f32
        
class NoteState:
    def __init__(self, obj):
        lowpass= obj["lowPass"]
        self.m_cptr = Native.NoteStateCreate(obj["sourceSamplePosition"], lowpass["z1"], lowpass["z2"]);

    def __del__(self):
        Native.NoteStateDestroy(self.m_cptr)
        
    def source_sample_position(self):
        return Native.NoteStateSourceSamplePosition(self.m_cptr)
        
    def z1(self):
        return Native.NoteStateZ1(self.m_cptr)
        
    def z2(self):
        return Native.NoteStateZ2(self.m_cptr)
        
class SynthCtrlPnt:
    def __init__(self, obj):
        lowpass= obj["lowPass"]
        self.m_cptr = Native.SynthCtrlPntCreate(obj["looping"], obj["gainMono"], obj["pitchRatio"], lowpass["active"], lowpass["a0"], lowpass["a1"], lowpass["b1"], lowpass["b2"])

    def __del__(self):
        Native.SynthCtrlPntDestroy(self.m_cptr)
        
class SynthCtrl:
    def __init__(self, obj):
        ctrlpnts = ObjArray([SynthCtrlPnt(pnt) for pnt in obj["controlPnts"]])
        self.m_cptr = Native.SynthCtrlCreate(obj["outputmode"], obj["loopStart"], obj["loopEnd"], obj["end"], obj["panFactorLeft"], obj["panFactorRight"], obj["effect_sample_block"], ctrlpnts.m_cptr)
        
    def __del__(self):
        Native.SynthCtrlDestroy(self.m_cptr)
        
def Synth(inBuf, outBuf, numSamples, noteState, control):
    ns = NoteState(noteState)
    ctrl = SynthCtrl(control)
    Native.SynthWav(inBuf.m_cptr, outBuf.m_cptr, numSamples, ns.m_cptr, ctrl.m_cptr)
    noteState["sourceSamplePosition"] = ns.source_sample_position()
    noteState["lowPass"]["z1"] = ns.z1()
    noteState["lowPass"]["z2"] = ns.z2()
        
    
