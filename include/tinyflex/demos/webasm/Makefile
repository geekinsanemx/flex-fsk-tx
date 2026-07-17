# tinyflex WebAssembly demo
# This is free and unencumbered software released into the public domain.

CC_JS   = emcc
CFLAGS += -Wall -Wextra -O3
CFLAGS_JS += $(CFLAGS)
CFLAGS_JS += -s EXPORTED_FUNCTIONS='["_encode_flex_message_str", "_get_encoded_data", "_get_encoded_size", "_get_last_error", "_malloc", "_free"]'
CFLAGS_JS += -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap", "HEAPU8"]'
CFLAGS_JS += -I$(CURDIR)/../..

all: flex_encoder.js

flex_encoder.js: flex_encoder.o
	$(CC_JS) $(CFLAGS_JS) flex_encoder.o -o flex_encoder.js

flex_encoder.o: flex_encoder.c $(CURDIR)/../../tinyflex.h
	$(CC_JS) $(CFLAGS_JS) flex_encoder.c -c

clean:
	rm -f flex_encoder.js flex_encoder.wasm *.o