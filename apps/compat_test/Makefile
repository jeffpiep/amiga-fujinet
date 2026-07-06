include ../../make/amiga.mk

INCLUDES = -I$(NIO_INC) -I$(COMPAT_INC)
LIBS     = $(COMPAT_LIB) $(NIO_ALIB)

TARGET = compat_test
SRCS   = $(COMPAT)/test/compat_test.c

APP_NAME         = compat_test
APP_BINARY       = $(TARGET)
EMU_PASS_PATTERN = fujibus: receive:
EMU_TIMEOUT      = 90

all: $(TARGET)

include ../../emu/template/emu.mk

$(TARGET): $(SRCS) $(COMPAT_LIB)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(SRCS) $(LIBS)

clean: emu-clean
	rm -f $(TARGET)

.PHONY: all clean
