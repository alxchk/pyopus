TARGET         ?= opus.so

CFLAGS         ?= -O2 -g -pipe -fPIC
LDFLAGS        ?= -Wl,-no-undefined

PYTHON_PKG     ?= python-2.7
OPUS_PKG       ?= $(shell pkg-config opus && echo opus)

CFLAGS_PYTHON  ?= $(shell pkg-config --cflags $(PYTHON_PKG))
LDFLAGS_PYTHON ?= $(shell pkg-config --libs $(PYTHON_PKG))

ifeq ($(OPUS_PKG),)
LIBOPUS        := ../opus/.libs/libopus.a
CFLAGS_OPUS    := -I../opus/include
else
CFLAGS_OPUS    ?= $(shell pkg-config --cflags $(OPUS_PKG))
LDFLAGS_OPUS   ?= $(shell pkg-config --libs $(OPUS_PKG))
LIBOPUS        :=
endif

all: $(TARGET)

../opus/.libs/libopus.a:
	-cd ../opus && git submodule init && \
		git submodule update && git clean -fdx
	cd ../opus && autoreconf -isf && ./configure \
		--enable-static --disable-shared --disable-float-api \
		--disable-hardening --disable-doc --disable-extra-programs \
		--disable-stack-protector --enable-fixed-point \
		--disable-maintainer-mode \
		CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" && \
		make

$(TARGET): pyopus.c $(LIBOPUS)
	$(CC) -fPIC -o $@ $< -shared $(CFLAGS) $(CFLAGS_PYTHON) $(LDFLAGS) \
	$(LDFLAGS_PYTHON) $(CFLAGS_OPUS) $(LDFLAGS_OPUS) $(LIBOPUS)

clean:
	rm -f $(TARGET)
