# -*- encoding: utf-8 -*-

from ctypes import (
    Structure, c_uint32, c_uint8, c_int32,
    CDLL, c_char_p, byref, POINTER, c_void_p, c_size_t,
    create_string_buffer, sizeof, c_ulong
)

PA_SAMPLE_U8 = 0
PA_SAMPLE_ALAW = 1
PA_SAMPLE_ULAW = 2
PA_SAMPLE_S16LE = 3
PA_SAMPLE_S16BE = 4
PA_SAMPLE_FLOAT32LE = 5
PA_SAMPLE_FLOAT32BE = 6
PA_SAMPLE_S32LE = 7
PA_SAMPLE_S32BE = 8
PA_SAMPLE_S24LE = 9
PA_SAMPLE_S24BE = 10
PA_SAMPLE_S24_32LE = 11
PA_SAMPLE_S24_32BE = 12
PA_SAMPLE_MAX = 13
PA_SAMPLE_INVALID = -1

PA_STREAM_NODIRECTION = 0
PA_STREAM_PLAYBACK = 1
PA_STREAM_RECORD = 2
PA_STREAM_UPLOAD = 3

PA_CHANNEL_POSITION_INVALID = -1
PA_CHANNEL_POSITION_MONO = 0,

PA_CHANNEL_POSITION_FRONT_LEFT = 1
PA_CHANNEL_POSITION_FRONT_RIGHT = 2
PA_CHANNEL_POSITION_FRONT_CENTER = 3

PA_CHANNEL_POSITION_LEFT = PA_CHANNEL_POSITION_FRONT_LEFT,
PA_CHANNEL_POSITION_RIGHT = PA_CHANNEL_POSITION_FRONT_RIGHT,
PA_CHANNEL_POSITION_CENTER = PA_CHANNEL_POSITION_FRONT_CENTER,

class PaSampleSpec(Structure):
    _fields_ = [
        ('format', c_int32),
        ('rate', c_uint32),
        ('channels', c_uint8)
    ]

class PaChannelMap(Structure):
    _fields_ = [
        ('channels', c_uint8),
        ('position', c_int32*32)
    ]

class PaBufferAttr(Structure):
    _fields_ = [
        ('maxlength', c_uint32),
        ('tlength', c_uint32),
        ('prebuf', c_uint32),
        ('minreq', c_uint32),
        ('fragsize', c_uint32),
    ]

libpulse = CDLL('libpulse-simple.so')

pa_simple_new = libpulse.pa_simple_new
pa_simple_new.argtypes = [
    c_char_p, c_char_p, c_int32, c_char_p, c_char_p,
    POINTER(PaSampleSpec), POINTER(PaChannelMap),
    POINTER(PaBufferAttr), POINTER(c_int32)
]
pa_simple_new.restype = c_void_p

pa_simple_read = libpulse.pa_simple_read
pa_simple_read.argtypes = [
    c_void_p, c_void_p, c_size_t, POINTER(c_int32)
]
pa_simple_read.restype = c_int32

pa_simple_free = libpulse.pa_simple_free
pa_simple_free.argtypes = [
    c_void_p
]

class PaRec(object):
    def __init__(self, server=None, name="parec", stream_name="stream", dev=None,
                 fmt=PA_SAMPLE_S16LE, rate=24000, channels=2):
        self.dev = dev
        self.name = name
        self.stream_name = stream_name
        self.server = server
        self.format = fmt
        self.rate = rate
        self.channels = channels

    def read(self, frame_size=120):
        spec = PaSampleSpec()
        spec.format = self.format
        spec.rate = self.rate
        spec.channels = self.channels

        error = c_int32(0)

        print self.server, self.name, PA_STREAM_RECORD, self.dev, self.stream_name, \
          byref(spec), None, None, byref(error)

        pa = pa_simple_new(
            self.server, self.name, PA_STREAM_RECORD, self.dev, self.stream_name,
            byref(spec), None, None, byref(error))

        if not pa:
            raise ValueError('Invalid something ({})'.format(error.value))

        print sizeof(spec)

        try:
            fs = self.channels*frame_size*2
            buf = create_string_buffer(fs)
            while pa_simple_read(pa, byref(buf), fs, byref(error)) >= 0:
                yield buf.raw

        finally:
            pa_simple_free(pa)
