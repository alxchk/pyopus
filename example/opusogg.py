# -*- coding: utf-8 -*-

__all__ = [
    'OpusOggFile'
]

# libogg required

from struct import pack
from ctypes import CDLL, Structure
from ctypes import (
    c_char_p, c_char, byref, POINTER, c_uint32, c_int32,
    c_uint16, c_long, c_ulonglong, c_void_p,
    create_string_buffer, cast
)

LIBOGG = CDLL('libogg.so.0')

class OggPacket(Structure):
    _fields_ = [
        ('packet', c_char_p),
        ('bytes', c_long),
        ('b_o_s', c_long),
        ('e_o_s', c_long),
        ('granulepos', c_ulonglong),
        ('packetno', c_ulonglong),
    ]

class OggPage(Structure):
    _fields_ = [
        ('header', c_void_p),
        ('header_len', c_long),
        ('body', c_void_p),
        ('body_len', c_long)
    ]

_ogg_stream_init = LIBOGG.ogg_stream_init
_ogg_stream_init.argtypes = [c_void_p, c_int32]
_ogg_stream_init.restype = c_int32

_ogg_stream_clear = LIBOGG.ogg_stream_clear
_ogg_stream_clear.argtypes = [c_void_p]

_ogg_stream_destroy = LIBOGG.ogg_stream_destroy
_ogg_stream_destroy.argtypes = [c_void_p]

_ogg_stream_packetin = LIBOGG.ogg_stream_packetin
_ogg_stream_packetin.argtypes = [c_void_p, POINTER(OggPacket)]
_ogg_stream_packetin.restype = c_int32

_ogg_stream_pageout = LIBOGG.ogg_stream_pageout
_ogg_stream_pageout.argtypes = [c_void_p, POINTER(OggPage)]
_ogg_stream_pageout.restype = c_int32

_ogg_stream_flush = LIBOGG.ogg_stream_flush
_ogg_stream_flush.argtypes = [c_void_p, POINTER(OggPage)]
_ogg_stream_flush.restype = c_int32

class OpusOggFile(object):
    def __init__(self, fobj, channels, samplerate, serialno=0, tags={}):
        # sizeof(ogg_stream_state) == 408
        ctx = create_string_buffer(408)
        if _ogg_stream_init(ctx, serialno) != 0:
            _ogg_stream_destroy(ctx)
            raise MemoryError()

        self._ctx = ctx
        self._fobj = fobj
        self.channels = channels
        self.samplerate = samplerate
        self.version = 1
        self.preskip = 0
        self.gain = 0
        self.channel_mapping_family = 0
        self.serialno = serialno
        self.packetno = 0
        self.granulepos = 0
        self.closed = False
        self.tags = tags

        self._tmp_page = OggPage()
        self._tmp_packet = OggPacket()

        self._write_header()

    def _write_page(self):
        ph = cast(
            self._tmp_page.header,
            POINTER(c_char*self._tmp_page.header_len))

        self._fobj.write(ph.contents.raw)

        pb = cast(
            self._tmp_page.body,
            POINTER(c_char*self._tmp_page.body_len))

        self._fobj.write(pb.contents.raw)
        self._fobj.flush()

    def _write_data_complete(self):
        pageout = _ogg_stream_pageout(self._ctx, byref(self._tmp_page))
        if pageout:
            self._write_page()

    def _feed_data(self, data, bos, eos, granulepos):
        self._tmp_packet.packet = data
        self._tmp_packet.bytes = len(data)
        self._tmp_packet.b_o_s = bos
        self._tmp_packet.e_o_s = eos
        self._tmp_packet.granulepos = granulepos
        self._tmp_packet.packetno = self.packetno

        if _ogg_stream_packetin(self._ctx, byref(self._tmp_packet)) < 0:
            raise ValueError('ogg_stream_packetin internal error')

        self.packetno += 1

    def _write_header(self):
        header = 'OpusHead' + \
          pack('BBHIHB',
               self.version,
               self.channels,
               self.preskip,
               self.samplerate,
               self.gain,
               self.channel_mapping_family
            )

        self._feed_data(header, 1, 0, 0)
        if _ogg_stream_flush(self._ctx, byref(self._tmp_page)):
            self._write_page()

        vendor = self.tags.pop('vendor', 'opusogg.py')

        tags = 'OpusTags' + pack('<I', len(vendor)) + vendor

        user_tags = [
            pack('<I', len(x)) + x for x in [
                '{}={}'.format(k,v) for k,v in self.tags.iteritems()
            ]
        ]

        user_tags = pack('<I', len(user_tags)) + ''.join(user_tags)
        self._feed_data(tags + user_tags, 0, 0, 0)
        if _ogg_stream_flush(self._ctx, byref(self._tmp_page)):
            self._write_page()

    def write(self, data, samples, eof=False):
        if self.closed:
            raise EOFError('File already closed')

        self._feed_data(data, 0, int(eof), self.granulepos)
        self._write_data_complete()

        self.granulepos += samples

    def close(self):
        if self.closed:
            raise EOFError('File already closed')

        self.write('', 0, True)
        self.closed = True

        if self._ctx:
            _ogg_stream_destroy(self._ctx)
            self._ctx = None

        if self._fobj:
            self._fobj = None

    def __del__(self):
        if not self.closed:
            self.close()
