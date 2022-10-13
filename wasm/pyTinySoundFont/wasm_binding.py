import os
from wasmer import engine, Store, Module, Instance
from wasmer_compiler_cranelift import Compiler

path_wasm = os.path.dirname(__file__)+"/Synth.wasm"

store = Store(engine.JIT(Compiler))
module = Module(store, open(path_wasm, 'rb').read())
instance = Instance(module)
mem = instance.exports.memory
raw_view = mem.uint8_view(0)

def S16ToF32(s16bytes):
    p_s16bytes = 0
    byte_len = len(s16bytes)
    length = byte_len // 2
    p_f32bytes = byte_len    
    raw_view[0:byte_len] = s16bytes;    
    instance.exports.S16ToF32(p_s16bytes, p_f32bytes, length)    
    return bytes(raw_view[byte_len:byte_len+length*4])
    
def F32ToS16(f32bytes, amplitude = 1.0):
    p_f32bytes = 0
    byte_len = len(f32bytes)
    length = byte_len // 4
    p_s16bytes = byte_len
    raw_view[0:byte_len] = f32bytes;
    instance.exports.F32ToS16(p_f32bytes, p_s16bytes, length, amplitude)
    return bytes(raw_view[byte_len:byte_len+length*2])

def MaxValueF32(f32bytes):
    byte_len = len(f32bytes)
    length = byte_len // 4
    raw_view[0:byte_len] = f32bytes;
    return instance.exports.MaxValueF32(0, length)
    
def MixF32(lst_bufs):
    offsets = []
    offset = 0
    for buf in lst_bufs:
        offsets += [offset]
        offset_next = offset + len(buf)
        raw_view[offset:offset_next] = buf        
        offset = offset_next        
    offsets += [offset]
    num_bufs = len(lst_bufs)
    
    view_offsets = mem.uint32_view(offset//4)
    view_offsets[0:num_bufs+1] = offsets;    
    
    max_len = instance.exports.MixF32(offset, num_bufs)
    offset += (num_bufs+1)*4
    return bytes(raw_view[offset: offset+ max_len*4])
    
def Synth(input_buf, output_buf, numSamples, note_state, control):
    offset = 0
    p_note_state = offset
    view_note_state = mem.float64_view(0)        
    view_note_state[0] = note_state["sourceSamplePosition"]
    view_note_state[1] = note_state["lowPass"]["z1"]
    view_note_state[2] = note_state["lowPass"]["z2"]    
    offset += 8*3    
    p_control = offset
    offset = instance.exports.SetControlHeader(p_control, control["outputmode"], control["loopStart"], control["loopEnd"], control["end"], control["panFactorLeft"], control["panFactorRight"], control["effect_sample_block"])    
    
    num_ctrl_pnts = len(control["controlPnts"])    
    for ctrl_pnt in control["controlPnts"]:
        p_ctrl_pnt = offset
        lowpass= ctrl_pnt["lowPass"]
        offset = instance.exports.SetControlPoint(p_ctrl_pnt, ctrl_pnt["looping"], ctrl_pnt["gainMono"], ctrl_pnt["pitchRatio"], lowpass["active"], lowpass["a0"], lowpass["a1"], lowpass["b1"], lowpass["b2"])                
        
    p_input = offset
    size_input = len(input_buf)
    raw_view[p_input: p_input+size_input] = input_buf    
    offset += size_input    
    p_output = offset
    size_output = len(output_buf)
    raw_view[p_output: p_output+size_output] = output_buf        
    instance.exports.Synth(p_input, p_output, numSamples, p_note_state, p_control, num_ctrl_pnts)    
    note_state["sourceSamplePosition"] = view_note_state[0]
    note_state["lowPass"]["z1"] = view_note_state[1]
    note_state["lowPass"]["z2"] = view_note_state[2]      
    output_buf[0:size_output] = raw_view[p_output: p_output+size_output]   
