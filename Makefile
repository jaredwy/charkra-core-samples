#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

IDIR=/Users/wyles/src/ChakraCore/lib/Jsrt
CC=clang++

LIBRARY_PATH=/Users/wyles/src/ChakraCore/out/Debug
LDIR=$(LIBRARY_PATH)/lib/libChakraCoreStatic.a

ICU4C_LIBRARY_PATH ?= /usr/local/opt/icu4c
CFLAGS= -g -std=c++14 -stdlib=libc++ -I$(IDIR)
FORCE_STARTS=-Wl,-force_load,
FORCE_ENDS=
LIBS=-framework CoreFoundation -framework Security -lm -ldl -Wno-c++11-compat-deprecated-writable-strings \
	-Wno-deprecated-declarations -Wno-unknown-warning-option -o sample.o
LDIR+=$(ICU4C_LIBRARY_PATH)/lib/libicudata.a \
	$(ICU4C_LIBRARY_PATH)/lib/libicuuc.a \
	$(ICU4C_LIBRARY_PATH)/lib/libicui18n.a

testmake:
	$(CC) sample.cpp JSHost.cpp $(CFLAGS) $(FORCE_STARTS) $(LDIR) $(FORCE_ENDS) $(LIBS)

.PHONY: clean

clean:
	rm sample.o