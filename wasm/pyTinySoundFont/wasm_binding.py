import os
import ctypes
import struct
from wasmtime import Store, Module, Instance, Func, FuncType

path_wasm = os.path.dirname(__file__)+"/SF2Synth.wasm"

store = Store()
module = Module(store.engine, open(path_wasm, 'rb').read())
instance = Instance(store, module, [])
exports = instance.exports(store)
mem = exports["memory"]
raw_view = (ctypes.c_ubyte*mem.data_len(store)).from_address(ctypes.addressof(mem.data_ptr(store).contents))

class ByteArray:
    def __init__(self, size):
        self.size = size
        self.data = exports["alloc"](store, size)
    
    def __del__(self):
        exports["dealloc"](store, self.data)
        
    def zero(self):
        exports["zero"](store, self.data, self.size)
        
    def get_bytes(self, start = 0, length = -1):
        if length<0:
            length = self.size
        return bytes(raw_view[self.data + start: self.data + start + length])
        
    def set_bytes(self, data, start = 0, length = -1):        
        if length<0:
            length = self.size
        raw_view[self.data + start: self.data + start + length] = data
        
        
def S16ToF32(s16bytes):
    byte_len = len(s16bytes)
    length = byte_len // 2
    arr_s16 = ByteArray(byte_len)
    arr_s16.set_bytes(s16bytes)
    arr_f32 = ByteArray(length * 4)
    exports["S16ToF32"](store, arr_s16.data, arr_f32.data, length)
    return arr_f32
    
def F32ToS16(arr_f32, amplitude = 1.0):
    byte_len = arr_f32.size
    length = byte_len // 4
    arr_s16 = ByteArray(length*2)
    exports["F32ToS16"](store, arr_f32.data, arr_s16.data, length, amplitude)
    return arr_s16.get_bytes()

def MaxValueF32(arr_f32):
    byte_len = arr_f32.size
    length = byte_len // 4
    return exports["MaxValueF32"](store, arr_f32.data, length)
    
def MixF32(lst_bufs):
    numBufs = len(lst_bufs)
    p_f32bufs = ByteArray(numBufs*4)
    lengths = ByteArray(numBufs*4)
    maxlen = 0
    for i in range(numBufs):
        arr = lst_bufs[i]
        length = arr.size // 4
        p_f32bufs.set_bytes(struct.pack('I', arr.data), i*4, 4)
        lengths.set_bytes(struct.pack('I', length), i*4, 4)
        if length>maxlen:
            maxlen = length    
    f32Out = ByteArray(maxlen * 4)
    exports["MixF32"](store, p_f32bufs.data, lengths.data, f32Out.data, maxlen, numBufs)    
    return f32Out
        
def Synth(input_buf, output_buf, numSamples, note_state, control):
    arr_note_state = ByteArray(8*3)
    arr_note_state.set_bytes(struct.pack('d', note_state["sourceSamplePosition"]), 0, 8)
    arr_note_state.set_bytes(struct.pack('d', note_state["lowPass"]["z1"]), 8, 8)
    arr_note_state.set_bytes(struct.pack('d', note_state["lowPass"]["z2"]), 16, 8)
    size_header = exports["GetSizeControlHeader"](store)
    ctrl_header = ByteArray(size_header)
    exports["SetControlHeader"](store, ctrl_header.data, control["outputmode"], control["loopStart"], control["loopEnd"], control["end"], control["panFactorLeft"], control["panFactorRight"], control["effect_sample_block"])
    num_ctrl_pnts = len(control["controlPnts"])
    size_ctrl_pnt = exports["GetSizeCtrlPnt"](store)
    ctrl_pnts = ByteArray(size_ctrl_pnt * num_ctrl_pnts)
    for i in range(num_ctrl_pnts):
        ctrl_pnt = control["controlPnts"][i]
        lowpass= ctrl_pnt["lowPass"]
        exports["SetControlPoint"](store, ctrl_pnts.data + size_ctrl_pnt*i, ctrl_pnt["looping"], ctrl_pnt["gainMono"], ctrl_pnt["pitchRatio"], lowpass["active"], lowpass["a0"], lowpass["a1"], lowpass["b1"], lowpass["b2"])
    exports["SynthWav"](store, input_buf.data, output_buf.data, numSamples, arr_note_state.data, ctrl_header.data, ctrl_pnts.data, num_ctrl_pnts)
    note_state["sourceSamplePosition"] = struct.unpack('d',  arr_note_state.get_bytes(0, 8))[0]
    note_state["lowPass"]["z1"] = struct.unpack('d',  arr_note_state.get_bytes(8, 8))[0]
    note_state["lowPass"]["z2"] = struct.unpack('d',  arr_note_state.get_bytes(16, 8))[0]




