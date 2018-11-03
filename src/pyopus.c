#include <Python.h>
#include <opus.h>

#ifndef MAX_CHANNELS
#define MAX_CHANNELS 2
#endif

#ifndef MAX_FRAME_SIZE
#define MAX_FRAME_SIZE (6*960)
#endif

#ifndef MAX_PACKET_SIZE
#define MAX_PACKET_SIZE	(3*1276)
#endif

static char docstring[] = "Embedded opus decoder/encoder";
static PyObject *opus_ErrorObject = NULL;

typedef struct {
	PyObject_HEAD
	OpusEncoder *enc;
	int channels;

} opus_EncoderObject, *popus_EncoderObject;

typedef struct {
	PyObject_HEAD
	OpusDecoder *dec;
	int channels;
} opus_DecoderObject, *popus_DecoderObject;

static PyObject *
opus_EncoderObject_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    popus_EncoderObject self = (popus_EncoderObject)type->tp_alloc(type, 0);
	self->enc = NULL;
    return (PyObject *)self;
}

static int
PyObject_ToLong(PyObject *object, long *value) {
	long result;

	if (!object) {
		return 0;
	}

#if PY_MAJOR_VERSION < 3
	if (!PyInt_Check(object)) {
		if (!PyLong_Check(object)) {
			return 0;
		} else {
			result = PyLong_AsLong(object);
		}
	} else {
		result = PyInt_AsLong(object);
	}
#else
	result = PyLong_AsLong(object);
#endif

	if (result == -1 && PyErr_Occurred())
		return 0;

	if (value) {
		*value = result;
	}

	return 1;
}

static int
PyObject_ToInt(PyObject *object, int *value) {
	long result;
	if (!PyObject_ToLong(object, &result)) {
		return 0;
	}

	if (result < INT_MIN || result > INT_MAX) {
		return 0;
	}

	if (value)
		*value = (int) result;

	return 1;
}

static int
opus_EncoderObject_init(popus_EncoderObject self, PyObject *args, PyObject *kwds)
{
	int fs = -1;
	int channels = -1;
	int application = -1;
	int error;

    if (! PyArg_ParseTuple(args, "iii", &fs, &channels, &application)) {
		return -1;
	}

	if (channels < 1 || channels > MAX_CHANNELS) {
		PyErr_SetString(opus_ErrorObject, "Only 1 or 2 channels are supported");
		return -1;
	}

	self->enc = opus_encoder_create(fs, channels, application, &error);
	if (error != OPUS_OK) {
		PyErr_SetString(opus_ErrorObject, opus_strerror(error));
		return -1;
	}

	self->channels = channels;

	return 0;
}

static int
opus_DecoderObject_init(popus_DecoderObject self, PyObject *args, PyObject *kwds)
{
	int fs = -1;
	int channels = -1;
	int error;

    if (! PyArg_ParseTuple(args, "ii", &fs, &channels)) {
		return -1;
	}

	if (channels < 1 || channels > MAX_CHANNELS) {
		PyErr_SetString(opus_ErrorObject, "Only 1 or 2 channels are supported");
		return -1;
	}

	self->dec = opus_decoder_create(fs, channels, &error);
	if (error != OPUS_OK) {
		PyErr_SetString(opus_ErrorObject, opus_strerror(error));
		return -1;
	}

	self->channels = channels;

	return 0;
}

static PyObject *
opus_DecoderObject_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    popus_DecoderObject self = (popus_DecoderObject)type->tp_alloc(type, 0);
	self->dec = NULL;
    return (PyObject *)self;
}

#define OPUS_CTL(x) ((int) ((uintptr_t) x))

static int
opus_EncoderObject_set(PyObject *self, PyObject *val, void *data) {
    popus_EncoderObject selfobj = (popus_EncoderObject) selfobj;
	int value;
	int ret;

	int opus_ctl = OPUS_CTL(data);

	if (opus_ctl % 2 != 0) {
		PyErr_SetString(opus_ErrorObject, "Invalid CTL");
		return -1;
	}

	Py_INCREF(val);

	if (!PyObject_ToInt(val, &value)) {
		PyErr_SetString(opus_ErrorObject, "Argument should be integer");
		Py_DECREF(val);
		return -1;
	}

	ret = opus_encoder_ctl(selfobj->enc, opus_ctl, value);
	if (ret != OPUS_OK) {
		PyErr_SetString(opus_ErrorObject, opus_strerror(ret));
		Py_DECREF(val);
		return -1;
	}

	Py_DECREF(val);
	return 0;
}

static PyObject *
opus_EncoderObject_get(PyObject *self, void *data) {
    popus_EncoderObject selfobj = (popus_EncoderObject) self;
	int opus_ctl = OPUS_CTL(data) + 1;
	int value = 0;
	int ret;

	if (opus_ctl % 2 != 1) {
		PyErr_SetString(opus_ErrorObject, "Invalid CTL");
		return NULL;
	}

	ret = opus_encoder_ctl(selfobj->enc, opus_ctl, &value);
	if (ret != OPUS_OK) {
		PyErr_SetString(opus_ErrorObject, opus_strerror(ret));
		return NULL;
	}

	return PyLong_FromLong(value);
}

static int
opus_DecoderObject_set(PyObject *self, PyObject *val, void *data) {
    popus_DecoderObject selfobj = (popus_DecoderObject) self;
	int opus_ctl = OPUS_CTL(data);
	int value = 0;
	int ret;

	if (opus_ctl % 2 != 0) {
		PyErr_SetString(opus_ErrorObject, "Invalid CTL");
		return -1;
	}

	if (!PyObject_ToInt(val, &value)) {
		PyErr_SetString(opus_ErrorObject, "Argument should be integer");
		return -1;
	}

	ret = opus_decoder_ctl(selfobj->dec, opus_ctl, value);
	if (ret != OPUS_OK) {
		PyErr_SetString(opus_ErrorObject, opus_strerror(ret));
		return -1;
	}

	return 0;
}

static PyObject *
opus_DecoderObject_get(PyObject *self, void *data) {
    popus_DecoderObject selfobj = (popus_DecoderObject) self;
	int opus_ctl = OPUS_CTL(data) + 1;
    int value = 0;
	int ret;

	if (opus_ctl % 2 != 1) {
		PyErr_SetString(opus_ErrorObject, "Invalid CTL");
		return NULL;
	}

	ret = opus_decoder_ctl(selfobj->dec, opus_ctl, &value);
	if (ret != OPUS_OK) {
		PyErr_SetString(opus_ErrorObject, opus_strerror(ret));
		return NULL;
	}

	return PyLong_FromLong(value);
}

#define ENCODER_GET(name, CTL, doc)				 \
	{											 \
		name, opus_EncoderObject_get, NULL, doc, \
			(void *) (OPUS_GET_ ## CTL ## _REQUEST - 1)	\
	}

#define ENCODER_GETSET(name, CTL, doc)							\
	{															\
		name, opus_EncoderObject_get, opus_EncoderObject_set,	\
			doc, (void *) OPUS_SET_ ## CTL ## _REQUEST			\
	}

#define DECODER_GET(name, CTL, doc)				 \
	{											 \
		name, opus_DecoderObject_get, NULL, doc, \
			(void *) (OPUS_GET_ ## CTL ## _REQUEST - 1)	\
	}

#define DECODER_GETSET(name, CTL, doc)							\
	{															\
		name, opus_DecoderObject_get, opus_DecoderObject_set,	\
			doc, (void *) OPUS_SET_ ## CTL ## _REQUEST			\
	}

#define BANDWIDTH_DOCSTRING												\
	"Sets the encoder's bandpass to a specific value.\n"				\
	"This prevents the encoder from automatically selecting the bandpass based " \
	"on the available bitrate. If an application knows the bandpass of the input " \
	"audio it is providing, it should normally use OPUS_SET_MAX_BANDWIDTH " \
	"instead, which still gives the encoder the freedom to reduce the bandpass " \
	"when the bitrate becomes too low, for better overall quality."

#define SAMPLE_RATE_DOCSTRING					\
	"If set to 1, disables the use of phase inversion for intensity stereo " \
	"improving the quality of mono downmixes, but slightly reducing normal " \
	"stereo quality. Disabling phase inversion in the decoder does not comply " \
	"with RFC 6716, although it does not cause any interoperability issue and " \
	"is expected to become part of the Opus standard once RFC 6716 is updated "	\
	"by draft-ietf-codec-opus-update. "


#define PHASE_INVERSION_DISABLED_DOCSTRING \
	"Gets the encoder's configured phase inversion status."

static PyObject*
opus_EncoderObject_encode(PyObject *self, PyObject *args)
{
	unsigned char *inbuf = NULL;
	size_t inbuf_size = 0;
	int frame_size = 0;
	int ret = -1;
	int i = 0;
	unsigned char encoded_bytes[MAX_PACKET_SIZE];
	opus_int16 in[MAX_FRAME_SIZE*MAX_CHANNELS];
    size_t required_size = 0;
    popus_EncoderObject encobj = (popus_EncoderObject) self;

	if (!PyArg_ParseTuple(args, "s#i", &inbuf, &inbuf_size, &frame_size)) {
		PyErr_SetString(opus_ErrorObject, "Invalid encode arguments");
		return NULL;
	}

	if (frame_size > MAX_FRAME_SIZE) {
		PyErr_SetString(opus_ErrorObject, "Frame size is too big");
		return NULL;
	}

	required_size = frame_size*encobj->channels*sizeof(opus_int16);
	if (inbuf_size < required_size) {
		PyErr_Format(opus_ErrorObject, "Invalid size: %lu < %lu",
			inbuf_size, required_size);
		return NULL;
	}

    /* Convert from little-endian ordering. */
	for (i=0; i<encobj->channels*frame_size; i++)
		in[i] = inbuf[2*i+1]<<8|inbuf[2*i];

	ret = opus_encode(encobj->enc, in,
		frame_size, encoded_bytes, sizeof(encoded_bytes));

	if (ret < 0) {
		PyErr_SetString(opus_ErrorObject, opus_strerror(ret));
		return NULL;
	}

	return PyBytes_FromStringAndSize(encoded_bytes, ret);
}

static PyObject*
opus_DecoderObject_decode(PyObject *self, PyObject *args)
{
	unsigned char *inbuf = NULL;
	size_t inbuf_size = 0;
	int frame_size = 0;
	int ret = -1;
	int i = 0;
	opus_int16 out[MAX_FRAME_SIZE*MAX_CHANNELS];
	unsigned char pcm_bytes[MAX_FRAME_SIZE*MAX_CHANNELS*sizeof(opus_int16)];

	popus_DecoderObject decobj = (popus_DecoderObject) self;

	if (!PyArg_ParseTuple(args, "s#i", &inbuf, &inbuf_size, &frame_size)) {
		return NULL;
	}

	if (frame_size > MAX_FRAME_SIZE) {
		PyErr_SetString(opus_ErrorObject, "Invalid frame size");
		return NULL;
	}

	ret = opus_decode(decobj->dec, inbuf, inbuf_size, out, frame_size, 0);

	if (ret < 0) {
		PyErr_SetString(opus_ErrorObject, opus_strerror(ret));
		return NULL;
	}

	for(i=0; i<decobj->channels*frame_size; i++) {
		pcm_bytes[2*i] = out[i] & 0xFF;
		pcm_bytes[2*i+1] = (out[i]>>8) & 0xFF;
	}

	return PyBytes_FromStringAndSize(
		pcm_bytes, frame_size*decobj->channels*sizeof(opus_int16));
}

static void
opus_EncoderObject_dealloc(PyObject *obj)
{
    popus_EncoderObject self = (popus_EncoderObject)obj;
	opus_encoder_destroy(self->enc);
	self->enc = NULL;
	self->channels = 0;
}

static void
opus_DecoderObject_dealloc(PyObject *obj)
{
    popus_DecoderObject self = (popus_DecoderObject)obj;
	opus_decoder_destroy(self->dec);
	self->dec = NULL;
	self->channels = 0;
}

static PyMethodDef
opus_EncoderObject_methods[] = {
	{
		"encode", opus_EncoderObject_encode, METH_VARARGS,
		"Encodes an Opus frame.\n"
		"1st arg - Input signal (interleaved if 2 channels). length is frame_size*channels*sizeof(opus_int16)\n"
		"2nd arg - Number of samples per channel in the input signal.\n"
		"          This must be an Opus frame size for the encoder's sampling rate.\n"
		"          For example, at 48 kHz the permitted values are 120, 240, 480, 960, 1920, 2880.\n"
	},
	{NULL}
};

static PyMethodDef
opus_DecoderObject_methods[] = {
	{
		"decode", opus_DecoderObject_decode, METH_VARARGS,
		"1st arg - Input payload. Use a NULL pointer to indicate packet loss\n"
		"2nd arg - Number of sample per channel in the input stream\n"
	},
	{NULL}
};

static PyGetSetDef
opus_EncoderObject_getset[] = {
	ENCODER_GETSET(
		"complexity", COMPLEXITY,
		"Configures the encoder's computational complexity.\n"
		"The supported range is 0-10 inclusive with 10 representing "
		"the highest complexity."
	),

	ENCODER_GETSET(
		"bitrate", BITRATE,
		"Configures the bitrate in the encoder.\n"
		"Rates from 500 to 512000 bits per second are meaningful, as well as the "
		"special values OPUS_AUTO and OPUS_BITRATE_MAX.\n"
		"The value OPUS_BITRATE_MAX can be used to cause the codec to use as much "
		"rate as it can, which is useful for controlling the rate by adjusting the "
		"output buffer size."
	),

	ENCODER_GETSET(
		"vbr", VBR,
		"Enables or disables constrained VBR in the encoder.\n"
		"This setting is ignored when the encoder is in CBR mode."
	),

	ENCODER_GETSET(
		"vbr_constraint", VBR_CONSTRAINT,
		"Determine if constrained VBR is enabled in the encoder."
	),

	ENCODER_GETSET(
		"force_channels", FORCE_CHANNELS,
		"Configures mono/stereo forcing in the encoder.\n"
		"This can force the encoder to produce packets encoded as either mono or "
		"stereo, regardless of the format of the input audio. This is useful when "
		"the caller knows that the input signal is currently a mono source embedded "
		"in a stereo stream."
	),

	ENCODER_GETSET(
		"max_bandwidth", MAX_BANDWIDTH,
		"Configures the maximum bandpass that the encoder will select automatically.\n"
		"Applications should normally use this instead of OPUS_SET_BANDWIDTH. "
		"This allows the application to set an upper bound based on the type of input it is "
		"providing, but still gives the encoder the freedom to reduce the bandpass"
		"when the bitrate becomes too low, for better overall quality."
	),

	ENCODER_GETSET(
		"bandwidth", BANDWIDTH, BANDWIDTH_DOCSTRING
	),

	ENCODER_GETSET(
		"signal", SIGNAL,
		"Configures the type of signal being encoded.\n"
		"This is a hint which helps the encoder's mode selection."
	),

	ENCODER_GETSET(
		"application", APPLICATION,
		"Configures the encoder's intended application."
	),

	ENCODER_GET(
		"lookahead", LOOKAHEAD,
		"Gets the total samples of delay added by the entire codec.\n"
		"This can be queried by the encoder and then the provided number "
		"of samples can be skipped on from the start of the decoder's "
		"output to provide time aligned input and output. From the perspective "
		"of a decoding application the real data begins this many samples late."
	),

	ENCODER_GETSET(
		"inband_fec", INBAND_FEC,
		"Gets encoder's configured use of inband forward error correction."
	),

	ENCODER_GETSET(
		"packet_loss_perc", PACKET_LOSS_PERC,
		"Configures the encoder's expected packet loss percentage.\n"
		"Higher values trigger progressively more loss resistant behavior in the encoder "
		"at the expense of quality at a given bitrate in the absence of packet loss, but "
		"greater quality under loss."
	),

	ENCODER_GETSET(
		"dtx", DTX,
		"Gets encoder's configured use of discontinuous transmission."
	),

	ENCODER_GETSET(
		"lsb_depth", LSB_DEPTH,
		"Gets the encoder's configured signal depth."
	),

	ENCODER_GETSET(
		"expert_frame_duration", EXPERT_FRAME_DURATION,
		"Configures the encoder's use of variable duration frames."
	),

	ENCODER_GETSET(
		"prediction_disabled", PREDICTION_DISABLED,
		"Gets the encoder's configured prediction status."
	),

	ENCODER_GET("sample_rate", SAMPLE_RATE, SAMPLE_RATE_DOCSTRING),

	ENCODER_GETSET(
		"phase_inversion_disabled", PHASE_INVERSION_DISABLED,
		PHASE_INVERSION_DISABLED_DOCSTRING
	),

	{NULL}
};

static PyGetSetDef
opus_DecoderObject_getset[] = {
	DECODER_GET("sample_rate", SAMPLE_RATE, SAMPLE_RATE_DOCSTRING),

	DECODER_GETSET(
		"phase_inversion_disabled", PHASE_INVERSION_DISABLED,
		PHASE_INVERSION_DISABLED_DOCSTRING
	),

	DECODER_GET("bandwidth", BANDWIDTH, BANDWIDTH_DOCSTRING),

	DECODER_GET(
		"pitch", PITCH,
		"Gets the pitch of the last decoded frame, if available."
	),

	DECODER_GET(
		"last_packet_duration", LAST_PACKET_DURATION,
		"Gets the duration (in samples) of the last packet successfully decoded or concealed."
	),

	DECODER_GETSET(
		"gain", GAIN,
		"Configures decoder gain adjustment.\n"
		"Scales the decoded output by a factor specified in Q8 dB units. "
		"This has a maximum range of -32768 to 32767 inclusive, and returns "
		"OPUS_BAD_ARG otherwise. The default is zero indicating no adjustment."
	),

	{NULL}
};

static PyTypeObject
opus_EncoderObjectType = {
    PyVarObject_HEAD_INIT(NULL, 0)
	"opus.Encoder",
	sizeof(opus_EncoderObject),
	0,
	opus_EncoderObject_dealloc,
    0,                             /* tp_print */
    0,                             /* tp_getattr */
    0,                             /* tp_setattr */
    0,                             /* tp_compare */
    0,                             /* tp_repr */
    0,                             /* tp_as_number */
    0,                             /* tp_as_sequence */
    0,                             /* tp_as_mapping */
    0,                             /* tp_hash */
    0,                             /* tp_call */
    0,                             /* tp_str */
    0,                             /* tp_getattro */
    0,                             /* tp_setattro */
    0,                             /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,            /* tp_flags */
	"Encoder(fs, channels, application)\n"
	"fs: Sampling rate of input signal (Hz) This must be one "
	     "of 8000, 12000, 16000, 24000, or 48000 (OPUS_FS_XXX).\n"
	"channels: Number of channels (1 or 2) in input signal.\n"
	"application: Coding mode (OPUS_APPLICATION_XXX)\n"
	, /* tp_doc */
    0,                             /* tp_traverse */
    0,                             /* tp_clear */
    0,                             /* tp_richcompare */
    0,                             /* tp_weaklistoffset */
    0,                             /* tp_iter */
    0,                             /* tp_iternext */
    opus_EncoderObject_methods,    /* tp_methods */
    0,                             /* tp_members */
    opus_EncoderObject_getset,     /* tp_getset */
    0,                             /* tp_base */
    0,                             /* tp_dict */
    0,                             /* tp_descr_get */
    0,                             /* tp_descr_set */
    0,                             /* tp_dictoffset */
    (initproc)opus_EncoderObject_init, /* tp_init */
    0,                             /* tp_alloc */
    opus_EncoderObject_new,         /* tp_new */
};

static PyTypeObject
opus_DecoderObjectType = {
    PyVarObject_HEAD_INIT(NULL, 0)
	"opus.Decoder",
	sizeof(opus_DecoderObject),
	0,
	opus_DecoderObject_dealloc,
    0,                             /* tp_print */
    0,                             /* tp_getattr */
    0,                             /* tp_setattr */
    0,                             /* tp_compare */
    0,                             /* tp_repr */
    0,                             /* tp_as_number */
    0,                             /* tp_as_sequence */
    0,                             /* tp_as_mapping */
    0,                             /* tp_hash */
    0,                             /* tp_call */
    0,                             /* tp_str */
    0,                             /* tp_getattro */
    0,                             /* tp_setattro */
    0,                             /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,            /* tp_flags */
	"Decoder(fs, channels, application)\n"
	"fs: Sampling rate of input signal (Hz) This must be one "
	     "of 8000, 12000, 16000, 24000, or 48000 (OPUS_FS_XXX).\n"
	"channels: Number of channels (1 or 2) in input signal.\n"
	"application: Coding mode (OPUS_APPLICATION_XXX)\n"
	, /* tp_doc */
    0,                             /* tp_traverse */
    0,                             /* tp_clear */
    0,                             /* tp_richcompare */
    0,                             /* tp_weaklistoffset */
    0,                             /* tp_iter */
    0,                             /* tp_iternext */
    opus_DecoderObject_methods,    /* tp_methods */
    0,                             /* tp_members */
    opus_DecoderObject_getset,     /* tp_getset */
    0,                             /* tp_base */
    0,                             /* tp_dict */
    0,                             /* tp_descr_get */
    0,                             /* tp_descr_set */
    0,                             /* tp_dictoffset */
    (initproc)opus_DecoderObject_init, /* tp_init */
    0,                             /* tp_alloc */
    opus_DecoderObject_new,         /* tp_new */
};

static PyMethodDef
opus_methods[] = {
	{NULL}
};

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef opus_module = {
        PyModuleDef_HEAD_INIT,
        "opus",
        NULL,
        0,
        opus_methods,
};

#define INITERROR return NULL

PyMODINIT_FUNC
PyInit_opus(void)
#else

#define INITERROR return

DL_EXPORT(void) initopus(void)
#endif
{
    PyObject *module = NULL;

	if (PyType_Ready(&opus_EncoderObjectType) < 0)
        INITERROR;

	if (PyType_Ready(&opus_DecoderObjectType) < 0)
        INITERROR;

#if PY_MAJOR_VERSION >= 3
	module = PyModule_Create(&opus_module);
#else
	module = Py_InitModule("opus", opus_methods);
#endif

	PyModule_AddIntMacro(module, OPUS_AUTO);
	PyModule_AddIntMacro(module, OPUS_BITRATE_MAX);
	PyModule_AddIntMacro(module, OPUS_APPLICATION_VOIP);
	PyModule_AddIntMacro(module, OPUS_APPLICATION_AUDIO);
	PyModule_AddIntMacro(module, OPUS_APPLICATION_RESTRICTED_LOWDELAY);
	PyModule_AddIntMacro(module, OPUS_SIGNAL_VOICE);
	PyModule_AddIntMacro(module, OPUS_SIGNAL_MUSIC);
	PyModule_AddIntMacro(module, OPUS_BANDWIDTH_NARROWBAND);
	PyModule_AddIntMacro(module, OPUS_BANDWIDTH_MEDIUMBAND);
	PyModule_AddIntMacro(module, OPUS_BANDWIDTH_WIDEBAND);
	PyModule_AddIntMacro(module, OPUS_BANDWIDTH_SUPERWIDEBAND);
	PyModule_AddIntMacro(module, OPUS_BANDWIDTH_FULLBAND);

	PyModule_AddIntConstant(module, "OPUS_FS_8000", 8000);
	PyModule_AddIntConstant(module, "OPUS_FS_12000", 12000);
	PyModule_AddIntConstant(module, "OPUS_FS_16000", 16000);
	PyModule_AddIntConstant(module, "OPUS_FS_24000", 24000);
	PyModule_AddIntConstant(module, "OPUS_FS_48000", 48000);

	PyModule_AddIntMacro(module, OPUS_FRAMESIZE_ARG);
	PyModule_AddIntMacro(module, OPUS_FRAMESIZE_2_5_MS);
	PyModule_AddIntMacro(module, OPUS_FRAMESIZE_5_MS);
	PyModule_AddIntMacro(module, OPUS_FRAMESIZE_10_MS);
	PyModule_AddIntMacro(module, OPUS_FRAMESIZE_20_MS);
	PyModule_AddIntMacro(module, OPUS_FRAMESIZE_40_MS);
	PyModule_AddIntMacro(module, OPUS_FRAMESIZE_60_MS);
	PyModule_AddIntMacro(module, OPUS_FRAMESIZE_80_MS);
	PyModule_AddIntMacro(module, OPUS_FRAMESIZE_100_MS);
	PyModule_AddIntMacro(module, OPUS_FRAMESIZE_120_MS);

	PyModule_AddStringConstant(
		module, "version", opus_get_version_string());

	opus_ErrorObject = PyErr_NewException("opus.Error", NULL, NULL);

    Py_INCREF(opus_ErrorObject);

    PyModule_AddObject(module, "Error", opus_ErrorObject);

	Py_INCREF(&opus_EncoderObjectType);
	Py_INCREF(&opus_DecoderObjectType);

	PyModule_AddObject(module, "Encoder", (PyObject *)&opus_EncoderObjectType);
	PyModule_AddObject(module, "Decoder", (PyObject *)&opus_DecoderObjectType);

#if PY_MAJOR_VERSION >= 3
    return module;
#endif
}
