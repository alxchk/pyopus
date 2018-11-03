#!/usr/bin/python

# Use pulseaudio to record something

import opus
from parec import PaRec

enc = opus.Encoder(24000, 2, opus.OPUS_APPLICATION_VOIP)
dec = opus.Decoder(24000, 2)
pulse = PaRec()

i = open('x.in.pcm', 'w+')
ed = open('x.opus', 'w+')
dd = open('x.out.pcm', 'w+')

frame_size = 3*960

for idx, d in enumerate(pulse.read(frame_size)):
    i.write(d)
    x = enc.encode(d, frame_size)
    ed.write(x)
    y = dec.decode(x, frame_size)
    dd.write(y)

    if idx > 5000:
        break

i.close()
ed.close()
dd.close()
