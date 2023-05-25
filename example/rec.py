#!/usr/bin/python

# Use pulseaudio to record something

import opus

from parec import PaRec
from opusogg import OpusOggFile

sample_rate = 24000
channels = 2
frame_size = 3*960

enc = opus.Encoder(sample_rate, channels, opus.OPUS_APPLICATION_VOIP)
dec = opus.Decoder(sample_rate, channels)

enc.packet_loss_perc = 0
enc.inband_fec = 0

pulse = PaRec()

i = open('x.in.pcm', 'w+b')
ed = open('x.opus', 'w+b')
dd = open('x.out.pcm', 'w+b')

ogg = OpusOggFile(ed, channels, sample_rate, tags={'TITLE':'example'})

try:
    for idx, d in enumerate(pulse.read(frame_size)):
        i.write(d)
        print(type(d), type(frame_size))
        x = enc.encode(d, frame_size)
        ogg.write(x, frame_size)
        y = dec.decode(x, frame_size)
        dd.write(y)
except KeyboardInterrupt:
    pass

finally:
    i.close()
    ogg.close()
    dd.close()
