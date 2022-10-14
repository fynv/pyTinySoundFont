from .wasm_binding import ByteArray, S16ToF32, F32ToS16, MaxValueF32

from .SF2 import LoadSF2
from .SF2Presets import LoadPresets

# Output Modes
# Two channels with single left/right samples one after another
STEREO_INTERLEAVED = 0
# Two channels with all samples for the left channel first then right
STEREO_UNWEAVED = 1
# A single channel (stereo instruments are mixed into center)
MONO = 2

from .SF2Synth import SynthNote
from .SF2SynthRT import TinySoundFont




